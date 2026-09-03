#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ChefZ Enforce-Script Audit
==========================

Statisches Audit ueber alle Addons in Psyerns_ChefZ_Core gegen die Hard Rules
des enforce-script-Skills (Wiki: DAYZ_Enforce-Script-main, Safe-AI-CodingPrompt,
Tips-Common-Pitfalls) sowie Struktur-Checks fuer config.cpp, $PREFIX$,
stringtable.csv, JSON-Configs und Inputs.xml.

Aufruf:
    python chefz_audit.py [--root <Pfad zu Psyerns_ChefZ_Core>]
                          [--vanilla <Pfad zu 'scripts - 1.29'>]
                          [--out <Report.md>] [--json <Findings.json>]

Exit-Code: 0 = keine P0/P1-Findings, 1 = P0/P1 vorhanden, 2 = Laufzeitfehler.

Prioritaeten (aus DME_129_Audit_Prompt.md):
    P0  Compile-Fehler (Ternary, Multi-Var, delete, auto/var, ref an falscher
        Stelle, modded class mit Vererbung, Redeklaration im Nested-Scope,
        Layer-Verstoss, Klammer-Ungleichgewicht, fehlende Dateien in files[])
    P1  Segfault/Crash (leere #ifdef, komplexe Array-Zuweisung)
    P2  1.29 Breaking (GetGame(), IsClient()/IsServer())
    P3  Silent Failures (RPC < 10000, fehlende Registrierung, fehlende
        STR_-Keys, kaputte stringtable, ungueltiges JSON, GetObjectsAtPosition)
    P4  Best Practice / Style (Tabs, Member-Prefix in modded class,
        override ohne super, mehrzeilige Aufrufe)
"""

import argparse
import csv
import io
import json
import os
import re
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict, OrderedDict
from datetime import datetime

# ---------------------------------------------------------------------------
# Konfiguration
# ---------------------------------------------------------------------------

DEFAULT_ROOT = r"C:\Users\Administrator\Desktop\Psyerns_ChefZ\Psyerns_ChefZ_Core"
DEFAULT_VANILLA = r"C:\Users\Administrator\Desktop\Mod Repositories\scripts - 1.29"

LAYER_ORDER = {"1_Core": 1, "2_GameLib": 2, "3_Game": 3, "4_World": 4, "5_Mission": 5}

# Member-Prefix, der in modded classes verlangt wird (Hard Rule: Mod-Prefix).
MOD_MEMBER_PREFIX_RE = re.compile(r"^(m_|s_)?(ChefZ|CHEFZ)", re.I)

# Enforce-Schluesselwoerter, die wie ein Typ aussehen koennen, aber keiner sind.
NOT_A_TYPE = {
    "return", "if", "else", "for", "while", "foreach", "switch", "case",
    "break", "continue", "new", "delete", "class", "modded", "override",
    "static", "const", "private", "protected", "proto", "native", "typedef",
    "enum", "true", "false", "null", "this", "super", "in", "out", "inout",
    "ref", "autoptr", "notnull", "default", "typename", "owned", "volatile",
    "extern", "event", "sealed", "local", "reference", "external",
}

# Bekannte Vanilla-Klassen, die sicher hoeheren Layern angehoeren, falls die
# Vanilla-Quelle nicht verfuegbar ist (Fallback).
FALLBACK_VANILLA_LAYERS = {
    "ItemBase": 4, "PlayerBase": 4, "EntityAI": 4, "Edible_Base": 4,
    "Inventory_Base": 4, "ActionBase": 4, "Cooking": 4, "FireplaceBase": 4,
    "ActionConstructor": 4, "PluginRecipesManagerBase": 4, "RecipeBase": 4,
    "Man": 4, "DayZPlayer": 4, "CargoBase": 4, "Hologram": 4,
    "MissionServer": 5, "MissionGameplay": 5, "MissionBase": 5,
    "UIScriptedMenu": 5, "MissionBaseWorld": 5,
}

# ---------------------------------------------------------------------------
# Hilfsstrukturen
# ---------------------------------------------------------------------------


class Finding:
    __slots__ = ("prio", "rule", "file", "line", "msg", "code")

    def __init__(self, prio, rule, file, line, msg, code=""):
        self.prio = prio
        self.rule = rule
        self.file = file
        self.line = line
        self.msg = msg
        self.code = code.strip()

    def as_dict(self):
        return OrderedDict(
            prio=self.prio, rule=self.rule, file=self.file, line=self.line,
            msg=self.msg, code=self.code,
        )


class Audit:
    def __init__(self, root, vanilla):
        self.root = os.path.abspath(root)
        self.vanilla = vanilla
        self.findings = []
        self.stats = OrderedDict()
        self.addons_dir = os.path.join(self.root, "Addons")
        self.class_layers = {}       # ChefZ-Klasse -> Layer-Nummer
        self.class_files = {}        # ChefZ-Klasse -> Datei
        self.vanilla_layers = {}     # Vanilla-Klasse -> Layer-Nummer
        self.vanilla_classes = {}    # Vanilla-Klasse -> [base, {methoden}]
        self.chefz_classes = {}      # ChefZ-Klasse -> [base, is_modded, {methoden}, datei]
        self.stringtable_keys = set()
        self.cfg_classes = set()     # alle in config.cpp deklarierten Klassen
        self.cfg_patches = set()     # CfgPatches-Klassennamen

    # -- helpers ------------------------------------------------------------

    def rel(self, path):
        return os.path.relpath(path, self.root).replace("\\", "/")

    def add(self, prio, rule, file, line, msg, code=""):
        self.findings.append(Finding(prio, rule, self.rel(file), line, msg, code))

    def walk(self, ext):
        for dirpath, _dirs, files in os.walk(self.addons_dir):
            for f in files:
                if f.lower().endswith(ext):
                    yield os.path.join(dirpath, f)

    @staticmethod
    def read(path):
        with open(path, "rb") as fh:
            raw = fh.read()
        for enc in ("utf-8-sig", "utf-8", "cp1252", "latin-1"):
            try:
                return raw.decode(enc)
            except UnicodeDecodeError:
                continue
        return raw.decode("latin-1", errors="replace")

    @staticmethod
    def layer_of(path):
        parts = path.replace("\\", "/").split("/")
        for p in parts:
            if p in LAYER_ORDER:
                return LAYER_ORDER[p]
        return None

    # -- Quelltext-Vorverarbeitung -------------------------------------------

    @staticmethod
    def strip_code(text):
        """Ersetzt Kommentare und String-Literale durch Platzhalter gleicher
        Laenge (Zeilenstruktur bleibt erhalten). Rueckgabe: (clean, in_comment_flags)
        in_comment_flags[i] = True wenn Zeile i komplett in einem Kommentar liegt."""
        out = []
        i = 0
        n = len(text)
        in_block = False
        in_line = False
        in_str = False
        while i < n:
            c = text[i]
            nxt = text[i + 1] if i + 1 < n else ""
            if in_block:
                if c == "*" and nxt == "/":
                    out.append("  ")
                    i += 2
                    in_block = False
                    continue
                out.append("\n" if c == "\n" else " ")
                i += 1
                continue
            if in_line:
                if c == "\n":
                    in_line = False
                    out.append("\n")
                else:
                    out.append(" ")
                i += 1
                continue
            if in_str:
                if c == "\\":
                    out.append("  ")
                    i += 2
                    continue
                if c == '"':
                    in_str = False
                    out.append('"')
                elif c == "\n":
                    out.append("\n")
                    in_str = False  # Enforce: kein mehrzeiliges Literal
                else:
                    out.append("_")
                i += 1
                continue
            if c == "/" and nxt == "*":
                in_block = True
                out.append("  ")
                i += 2
                continue
            if c == "/" and nxt == "/":
                in_line = True
                out.append("  ")
                i += 2
                continue
            if c == '"':
                in_str = True
                out.append('"')
                i += 1
                continue
            out.append(c)
            i += 1
        return "".join(out)

    @staticmethod
    def strip_comments_only(text):
        """Entfernt nur Kommentare, laesst String-Literale stehen (fuer config.cpp)."""
        out = []
        i = 0
        n = len(text)
        in_block = in_line = in_str = False
        while i < n:
            c = text[i]
            nxt = text[i + 1] if i + 1 < n else ""
            if in_block:
                if c == "*" and nxt == "/":
                    in_block = False
                    out.append("  ")
                    i += 2
                    continue
                out.append("\n" if c == "\n" else " ")
                i += 1
                continue
            if in_line:
                if c == "\n":
                    in_line = False
                    out.append("\n")
                else:
                    out.append(" ")
                i += 1
                continue
            if in_str:
                out.append(c)
                if c == "\\":
                    out.append(nxt)
                    i += 2
                    continue
                if c == '"' or c == "\n":
                    in_str = False
                i += 1
                continue
            if c == "/" and nxt == "*":
                in_block = True
                out.append("  ")
                i += 2
                continue
            if c == "/" and nxt == "/":
                in_line = True
                out.append("  ")
                i += 2
                continue
            if c == '"':
                in_str = True
            out.append(c)
            i += 1
        return "".join(out)

    @staticmethod
    def strip_templates(line):
        prev = None
        while prev != line:
            prev = line
            line = re.sub(r"<[^<>]*>", "", line)
        return line

    @staticmethod
    def extract_block(text, header_re):
        """Liefert den Inhalt des ersten {...}-Blocks nach header_re (klammer-balanciert)."""
        m = re.search(header_re, text)
        if not m:
            return None, -1
        i = text.find("{", m.end())
        if i < 0:
            return None, -1
        depth = 0
        for j in range(i, len(text)):
            if text[j] == "{":
                depth += 1
            elif text[j] == "}":
                depth -= 1
                if depth == 0:
                    return text[i + 1:j], m.start()
        return None, -1

    # -- Vanilla-Klassenkarte -----------------------------------------------

    CLASS_RE = re.compile(r"^\s*(?:modded\s+)?class\s+([A-Za-z_]\w*)", re.M)
    CLASS_FULL_RE = re.compile(r"^\s*(?P<modded>modded\s+)?class\s+(?P<name>[A-Za-z_]\w*)(?:\s*<[^>]*>)?\s*(?:(?::|\bextends\b)\s*(?P<base>[A-Za-z_]\w*))?", re.M)

    def collect_class_methods(self, clean):
        """Liefert {klasse: (base, is_modded, {methoden})} fuer einen bereinigten Quelltext."""
        result = {}
        lines = clean.split("\n")
        brace = 0
        cur = None
        cur_depth = 0
        for l in lines:
            cm = self.CLASS_FULL_RE.match(l)
            if cm and brace == 0:
                cur = cm.group("name")
                cur_depth = brace
                base = cm.group("base")
                entry = result.get(cur)
                if entry is None:
                    result[cur] = [base, bool(cm.group("modded")), set()]
                elif base and not entry[0]:
                    entry[0] = base
            if cur is not None and brace == cur_depth + 1:
                fm = self.FUNC_DECL_START_RE.match(l)
                if fm and not re.match(r"^\s*(if|for|while|switch|foreach|return|else)\b", l):
                    name = fm.group("name")
                    if name != cur and fm.group("ret") not in NOT_A_TYPE:
                        result[cur][2].add(name)
                # Konstruktor/Destruktor: 'void ClassName(' bzw. 'void ~ClassName('
            for ch in l:
                if ch == "{":
                    brace += 1
                elif ch == "}":
                    brace -= 1
                    if cur is not None and brace <= cur_depth:
                        cur = None
        return result

    def build_vanilla_map(self):
        if not self.vanilla or not os.path.isdir(self.vanilla):
            self.vanilla_layers = dict(FALLBACK_VANILLA_LAYERS)
            self.stats["vanilla_map"] = "FALLBACK (Vanilla-Quelle nicht gefunden)"
            return
        count = 0
        for layer, num in LAYER_ORDER.items():
            base = os.path.join(self.vanilla, layer)
            if not os.path.isdir(base):
                continue
            for dirpath, _d, files in os.walk(base):
                for f in files:
                    if not f.endswith(".c"):
                        continue
                    text = self.read(os.path.join(dirpath, f))
                    clean = self.strip_code(text)
                    for m in self.CLASS_RE.finditer(clean):
                        name = m.group(1)
                        # Erste (niedrigste) Definition gewinnt: modded class in
                        # hoeherem Layer aendert die Heimat nicht.
                        if name not in self.vanilla_layers or num < self.vanilla_layers[name]:
                            self.vanilla_layers[name] = num
                        count += 1
                    for cls, (base, _modded, methods) in self.collect_class_methods(clean).items():
                        entry = self.vanilla_classes.setdefault(cls, [None, set()])
                        if base and not entry[0]:
                            entry[0] = base
                        entry[1].update(methods)
        self.stats["vanilla_map"] = "%d Klassen aus %s" % (len(self.vanilla_layers), self.vanilla)

    def build_chefz_map(self):
        for path in self.walk(".c"):
            layer = self.layer_of(path)
            clean = self.strip_code(self.read(path))
            for cls, (base, modded, methods) in self.collect_class_methods(clean).items():
                entry = self.chefz_classes.setdefault(cls, [None, modded, set(), self.rel(path)])
                if base and not entry[0]:
                    entry[0] = base
                entry[2].update(methods)
            for m in self.CLASS_RE.finditer(clean):
                name = m.group(1)
                line_start = clean.rfind("\n", 0, m.start()) + 1
                is_modded = clean[line_start:m.end()].lstrip().startswith("modded")
                if is_modded:
                    continue
                if layer is None:
                    continue
                if name in self.class_layers and self.class_layers[name] != layer:
                    self.add("P0", "duplicate-class", path, clean.count("\n", 0, m.start()) + 1,
                             "Klasse '%s' ist mehrfach definiert (auch in %s)" % (name, self.class_files[name]))
                self.class_layers.setdefault(name, layer)
                self.class_files.setdefault(name, self.rel(path))

    # -- Script-Checks -------------------------------------------------------

    DECL_RE = re.compile(
        r"^\s*(?:(?:static|const|private|protected|autoptr|ref|out|inout|notnull|owned)\s+)*"
        r"(?P<type>[A-Za-z_][\w:]*)(?P<tmpl><[^;=]*?>)?\s+(?P<name>[A-Za-z_]\w*)\s*(?P<rest>\[[^\]]*\]\s*)?(?:=|;|,)"
    )
    FUNC_DECL_RE = re.compile(
        r"^\s*(?:(?:override|static|private|protected|proto|native|event|sealed|external)\s+)*"
        r"(?:ref\s+)?(?P<ret>[A-Za-z_][\w:]*(?:<[^>]*>)?)\s+(?P<name>[A-Za-z_]\w*)\s*\((?P<params>[^)]*)\)\s*(?:;|\{|$)"
    )
    FUNC_DECL_START_RE = re.compile(
        r"^\s*(?:(?:override|static|private|protected|proto|native|event|sealed|external)\s+)*"
        r"(?:ref\s+)?(?P<ret>[A-Za-z_][\w:]*(?:<[^>]*>)?)\s+(?P<name>[A-Za-z_]\w*)\s*\("
    )

    def check_script(self, path):
        text = self.read(path)
        clean = self.strip_code(text)
        raw_lines = text.split("\n")
        lines = clean.split("\n")
        layer = self.layer_of(path)
        rel = self.rel(path)

        # Tabs vs. Spaces (P4) - eine Meldung je Datei
        space_lines = sum(1 for l in raw_lines if l.startswith(" ") and l.strip())
        tab_lines = sum(1 for l in raw_lines if l.startswith("\t"))
        if space_lines and not tab_lines:
            self.add("P4", "indent-spaces", path, 1,
                     "Einrueckung mit Leerzeichen (%d Zeilen), Hard Rule verlangt Tabs" % space_lines)
        elif space_lines and tab_lines:
            self.add("P4", "indent-mixed", path, 1,
                     "Gemischte Einrueckung: %d Tab-Zeilen, %d Space-Zeilen" % (tab_lines, space_lines))

        # Klammer-Balance (P0)
        depth_brace = 0
        depth_paren = 0
        for l in lines:
            depth_brace += l.count("{") - l.count("}")
            depth_paren += l.count("(") - l.count(")")
        if depth_brace != 0:
            self.add("P0", "brace-balance", path, len(lines), "Geschweifte Klammern unausgeglichen (Delta %+d)" % depth_brace)
        if depth_paren != 0:
            self.add("P0", "paren-balance", path, len(lines), "Runde Klammern unausgeglichen (Delta %+d)" % depth_paren)

        # Praeprozessor: leere #ifdef/#endif (P1)
        pp_stack = []
        for i, l in enumerate(lines):
            s = l.strip()
            if s.startswith("#ifdef") or s.startswith("#ifndef"):
                pp_stack.append((i, True))
            elif s.startswith("#else"):
                if pp_stack:
                    idx, empty = pp_stack[-1]
                    if empty:
                        self.add("P1", "empty-ifdef", path, idx + 1, "Leerer Praeprozessor-Block vor #else (Segfault-Risiko)", raw_lines[idx])
                    pp_stack[-1] = (i, True)
            elif s.startswith("#endif"):
                if pp_stack:
                    idx, empty = pp_stack.pop()
                    if empty:
                        self.add("P1", "empty-ifdef", path, idx + 1, "Leerer #ifdef/#endif-Block (Segfault-Risiko)", raw_lines[idx])
            elif s and pp_stack:
                idx, _e = pp_stack[-1]
                pp_stack[-1] = (idx, False)

        # Zeilenweise Regeln
        in_class = None            # Name der aktuellen Klasse
        class_is_modded = False
        class_depth = 0
        brace = 0
        func_scopes = None         # Liste von Sets (Scope-Stack) innerhalb einer Funktion
        func_depth = 0
        func_name = None
        func_is_override = False
        func_has_super = False
        func_start = 0
        pending_decl_line = None   # mehrzeilige Funktionsdeklaration
        paren_depth = 0
        paren_open_line = None
        paren_open_text = None

        for i, l in enumerate(lines):
            raw = raw_lines[i] if i < len(raw_lines) else ""
            s = l.strip()
            ln = i + 1

            # --- Klassenkontext
            cm = re.match(r"^\s*(modded\s+)?class\s+([A-Za-z_]\w*)\s*((?::|\bextends\b)\s*([A-Za-z_]\w*))?", l)
            if cm and brace == 0:
                in_class = cm.group(2)
                class_is_modded = bool(cm.group(1))
                class_depth = brace
                if class_is_modded and cm.group(3):
                    self.add("P0", "modded-inherit", path, ln, "modded class darf keine Vererbung ': %s' tragen" % cm.group(4), raw)

            # --- verbotene Syntax (P0)
            if "GetGame()" in l:
                self.add("P2", "GetGame", path, ln, "GetGame() ist ab 1.29 deprecated - g_Game mit Null-Check verwenden", raw)
            if re.search(r"\.IsClient\s*\(\)|\.IsServer\s*\(\)", l):
                self.add("P2", "IsClient-IsServer", path, ln, "IsClient()/IsServer() gelten als unzuverlaessig - IsDedicatedServer() bzw. !IsDedicatedServer()", raw)
            if re.search(r"^\s*delete\s+\w", l):
                self.add("P0", "delete", path, ln, "'delete' ist verboten - Referenz auf null setzen", raw)
            if re.search(r"^\s*(auto|var)\s+[A-Za-z_]\w*\s*=", l):
                self.add("P0", "auto-var", path, ln, "'auto'/'var' werden von Enforce nicht unterstuetzt", raw)
            if "?." in l or "??" in l:
                self.add("P0", "optional-chaining", path, ln, "'?.' / '??' werden von Enforce nicht unterstuetzt", raw)
            if "=>" in l:
                self.add("P0", "lambda", path, ln, "Lambda/Arrow-Syntax wird von Enforce nicht unterstuetzt", raw)
            if "?" in l:
                # Ternary: '?' ausserhalb von Strings (bereits maskiert) mit ':' dahinter
                q = l.find("?")
                if ":" in l[q:]:
                    self.add("P0", "ternary", path, ln, "Ternary-Operator wird von Enforce nicht unterstuetzt - if/else", raw)
            if "GetObjectsAtPosition" in l:
                self.add("P3", "GetObjectsAtPosition", path, ln, "GetObjectsAtPosition* ist teuer - Wiki raet zu Triggern/statischen Listen (pruefen, ob Aufruf selten ist)", raw)

            # --- Multi-Var-Deklaration (P0): 'int a, b;' - Template-Kommas entfernen
            no_tmpl = self.strip_templates(l)
            if "(" not in no_tmpl and not re.search(r"=\s*\{", no_tmpl) and re.match(
                    r"^\s*(?:(?:static|const|private|protected|ref|autoptr)\s+)*[A-Za-z_][\w:]*\s+[A-Za-z_]\w*\s*(?:\[[^\]]*\])?\s*(?:=[^,;]*)?,\s*[A-Za-z_]\w*", no_tmpl):
                if not re.match(r"^\s*(return|case)\b", no_tmpl):
                    self.add("P0", "multi-decl", path, ln, "Mehrere Variablen in einer Deklaration", raw)

            # --- ref an falscher Stelle (P0)
            fdm = self.FUNC_DECL_START_RE.match(l)
            if re.match(r"^\s*(?:(?:override|static|private|protected|proto|native)\s+)*ref\s+[A-Za-z_]", l) and fdm:
                self.add("P0", "ref-return", path, ln, "'ref' im Rueckgabetyp ist verboten", raw)
            if fdm and re.search(r"[(,]\s*(?:out\s+|inout\s+|notnull\s+)?ref\s+[A-Za-z_]", no_tmpl):
                self.add("P0", "ref-param", path, ln, "'ref' in Parametern ist verboten", raw)

            # --- Funktions-Erkennung fuer Scope-Tracking
            if in_class is not None and brace == class_depth + 1 and func_scopes is None:
                if fdm and not s.endswith(";") and not re.match(r"^\s*(if|for|while|switch|foreach|return)\b", l):
                    func_name = fdm.group("name")
                    func_is_override = bool(re.match(r"^\s*override\b", l))
                    func_has_super = False
                    func_start = ln
                    params = set()
                    pm = re.search(r"\((.*)\)", l)
                    if pm:
                        for p in pm.group(1).split(","):
                            p = p.strip()
                            m2 = re.search(r"([A-Za-z_]\w*)\s*(=.*)?$", p)
                            if m2 and p:
                                params.add(m2.group(1))
                    func_scopes = [params]
                    func_depth = brace
                    if "{" in l and "}" not in l:
                        func_scopes.append(set())

            # --- innerhalb einer Funktion: Deklarationen + super-Aufruf
            if func_scopes is not None and brace > func_depth:
                if re.search(r"\bsuper\s*\.\s*%s\s*\(" % re.escape(func_name or "\x00"), l):
                    func_has_super = True
                # Lokale ref-Deklaration
                if re.match(r"^\s*ref\s+[A-Za-z_]", l):
                    self.add("P0", "ref-local", path, ln, "'ref' auf lokaler Variable ist verboten", raw)
                # Deklarationen (auch in for(...) )
                decl_targets = []
                dm = self.DECL_RE.match(no_tmpl)
                if dm and dm.group("type") not in NOT_A_TYPE and "(" not in no_tmpl.split("=")[0]:
                    decl_targets.append(dm.group("name"))
                for fm in re.finditer(r"\bfor\s*\(\s*([A-Za-z_][\w:]*)\s+([A-Za-z_]\w*)\s*=", no_tmpl):
                    if fm.group(1) not in NOT_A_TYPE:
                        decl_targets.append(fm.group(2))
                for fm in re.finditer(r"\bforeach\s*\(([^)]*)\)", no_tmpl):
                    for part in fm.group(1).split(":")[0].split(","):
                        m3 = re.search(r"([A-Za-z_]\w*)\s*$", part.strip())
                        if m3:
                            decl_targets.append(m3.group(1))
                # Cast-Deklarationen: 'Type x; if (Class.CastTo(x, y))' sind normale Decls, ok
                for name in decl_targets:
                    for depth_idx, scope in enumerate(func_scopes[:-1]):
                        if name in scope:
                            self.add("P0", "redeclare-nested", path, ln,
                                     "Variable '%s' wird im verschachtelten Scope erneut deklariert (bereits in aeusserem Scope/Parameter)" % name, raw)
                            break
                    func_scopes[-1].add(name)

                # Komplexe Array-Zuweisung (P1)
                am = re.match(r"^\s*[A-Za-z_][\w.]*\s*\[[^\]]+\]\s*=\s*(.+);\s*$", l)
                if am:
                    rhs = am.group(1)
                    if re.search(r"(<=|>=|==|!=|<|>|&&|\|\|)", rhs) and "(" in rhs:
                        self.add("P1", "array-complex-assign", path, ln, "Komplexer Ausdruck direkt in Array-Index-Zuweisung (Segfault-Risiko) - Zwischenvariable", raw)

            # --- Mehrzeilige Funktionsaufrufe (P4) - Klammer-Tiefe ueber Zeilen
            opened = l.count("(") - l.count(")")
            if paren_depth == 0 and opened > 0:
                paren_open_line = ln
                paren_open_text = raw
            paren_depth += opened
            if paren_depth < 0:
                paren_depth = 0
            if paren_depth == 0 and paren_open_line is not None and paren_open_line != ln:
                # Eine Deklaration mit mehrzeiliger Signatur ist laut Safe-Prompt erlaubt
                is_decl = bool(self.FUNC_DECL_START_RE.match(lines[paren_open_line - 1])) and lines[paren_open_line - 1].rstrip().endswith((",", "("))
                is_ctrl = bool(re.match(r"^\s*(if|for|while|switch|foreach)\b", lines[paren_open_line - 1]))
                if not is_decl:
                    what = "Bedingung" if is_ctrl else "Funktionsaufruf"
                    self.add("P4", "multiline-call", path, paren_open_line,
                             "%s ueber %d Zeilen gebrochen - Hard Rule: eine Zeile oder Variablen extrahieren" % (what, ln - paren_open_line + 1), paren_open_text)
                paren_open_line = None

            # --- Member-Prefix in modded class (P4)
            if in_class is not None and class_is_modded and brace == class_depth + 1 and func_scopes is None:
                mm = self.DECL_RE.match(no_tmpl)
                if mm and mm.group("type") not in NOT_A_TYPE and "(" not in no_tmpl:
                    mname = mm.group("name")
                    if not MOD_MEMBER_PREFIX_RE.match(mname):
                        self.add("P4", "modded-member-prefix", path, ln,
                                 "Member '%s' in modded class %s ohne Mod-Prefix (m_ChefZ...)" % (mname, in_class), raw)

            # --- Brace-Tracking
            for ch in l:
                if ch == "{":
                    brace += 1
                    if func_scopes is not None and not (ln == func_start):
                        func_scopes.append(set())
                elif ch == "}":
                    brace -= 1
                    if func_scopes is not None:
                        if len(func_scopes) > 1:
                            func_scopes.pop()
                        if brace <= func_depth:
                            # Funktion beendet
                            if class_is_modded and func_is_override and not func_has_super:
                                self.add("P4", "override-no-super", path, func_start,
                                         "override %s() in modded class %s ruft super.%s() nicht auf" % (func_name, in_class, func_name))
                            func_scopes = None
                            func_name = None
                    if in_class is not None and brace <= class_depth:
                        in_class = None
                        class_is_modded = False

        # Layer-Verstoesse (P0): Referenz auf Klassen hoeherer Schichten.
        # Nur TYP-Positionen zaehlen: Deklaration 'X name', statischer Zugriff
        # 'X.' (nicht hinter '.'), 'new X', ': X', Template-Argument '<X>'.
        if layer is not None:
            words = set(re.findall(r"\b[A-Za-z_]\w*\b", clean))
            for w in words:
                target = None
                src = None
                if w in self.class_layers and self.class_layers[w] > layer:
                    target = self.class_layers[w]
                    src = "ChefZ (%s)" % self.class_files[w]
                elif w in self.vanilla_layers and self.vanilla_layers[w] > layer and w not in self.class_layers:
                    target = self.vanilla_layers[w]
                    src = "Vanilla 1.29"
                if not target:
                    continue
                W = re.escape(w)
                type_pos = re.compile(
                    r"(?<![\w.])%s(?:\s+[A-Za-z_]\w*\s*[;=,)\[]|\s*\.\s*[A-Za-z_]|\s*[>,](?=[^;(]*>))" % W
                    + r"|\bnew\s+%s\b|:\s*%s\b|<\s*%s\b" % (W, W, W)
                )
                for i, l in enumerate(lines):
                    if type_pos.search(l):
                        self.add("P0", "layer-violation", path, i + 1,
                                 "Layer %d referenziert '%s' aus Layer %d (%s)" % (layer, w, target, src), raw_lines[i])
                        break

        # RPC-Kennungen (P3)
        for i, l in enumerate(lines):
            m = re.search(r"\b(?:RPC|Rpc)\w*\s*=\s*(-?\d+)\b|\b\w*(?:RPC|Rpc)\w*\s*=\s*(-?\d+)\s*[;,]", l)
            if m:
                val = int(m.group(1) or m.group(2))
                if val < 10000:
                    self.add("P3", "rpc-range", path, i + 1, "RPC-Kennung %d liegt unter 10000 (Vanilla-Bereich)" % val, raw_lines[i])
        for m in re.finditer(r"enum\s+\w*(?:RPC|Rpc)\w*\s*\{([^}]*)\}", clean):
            body = m.group(1)
            first = re.search(r"=\s*(-?\d+)", body)
            if first is None:
                self.add("P3", "rpc-range", path, clean.count("\n", 0, m.start()) + 1, "RPC-Enum ohne expliziten Startwert (beginnt bei 0 - Vanilla-Kollision)")
            elif int(first.group(1)) < 10000:
                self.add("P3", "rpc-range", path, clean.count("\n", 0, m.start()) + 1, "RPC-Enum beginnt bei %s (< 10000)" % first.group(1))

        # STR_-Keys aus Skripten sammeln (fuer Stringtable-Abgleich)
        for m in re.finditer(r'"#?(STR_[A-Za-z0-9_]+)"', text):
            self.used_str_keys.add((m.group(1), rel))

    # -- config.cpp ------------------------------------------------------------

    def cpp_strip(self, text):
        return self.strip_code(text)

    def check_config_cpp(self, addon_dir):
        path = os.path.join(addon_dir, "config.cpp")
        addon = os.path.basename(addon_dir)
        prefix_path = os.path.join(addon_dir, "$PREFIX$")
        prefix = None
        if os.path.isfile(prefix_path):
            prefix = self.read(prefix_path).strip().replace("\\", "/")
        else:
            self.add("P3", "prefix-missing", addon_dir, 1, "Addon %s hat keine $PREFIX$-Datei" % addon)
        if not os.path.isfile(path):
            self.add("P3", "config-missing", addon_dir, 1, "Addon %s hat keine config.cpp" % addon)
            return
        raw_text = self.read(path)
        clean = self.cpp_strip(raw_text)
        # 'text' = ohne Kommentare, MIT Strings - fuer alle String-Extraktionen.
        text = self.strip_comments_only(raw_text)
        raw_lines = raw_text.split("\n")

        # Klammerbalance
        if clean.count("{") != clean.count("}"):
            self.add("P0", "cfg-brace-balance", path, len(raw_lines), "config.cpp: geschweifte Klammern unausgeglichen")

        # CfgPatches
        body, _pos = self.extract_block(text, r"class\s+CfgPatches\b")
        if body is None:
            self.add("P0", "cfg-no-cfgpatches", path, 1, "config.cpp ohne CfgPatches")
        else:
            for cm in re.finditer(r"class\s+(\w+)", body):
                self.cfg_patches.add(cm.group(1))
            rv = re.search(r"requiredVersion\s*=\s*([\d.]+)", body)
            if not rv:
                self.add("P3", "cfg-requiredVersion", path, 1, "CfgPatches ohne requiredVersion")
            ra = re.search(r"requiredAddons\[\]\s*=\s*\{([^}]*)\}", body, re.S)
            if ra:
                for a in re.findall(r'"([^"]+)"', ra.group(1)):
                    if a.startswith("ChefZ_"):
                        self.required_chefz.append((addon, a, path))
            units = re.search(r"units\[\]\s*=\s*\{([^}]*)\}", body, re.S)
            if units:
                for u in re.findall(r'"([^"]+)"', units.group(1)):
                    self.cfg_units.append((u, path))

        # CfgMods: dir, files[], inputs
        mm = re.search(r"class\s+CfgMods\s*\{(.*)", clean, re.S)
        if mm:
            dm = re.search(r'dir\s*=\s*"([^"]*)"', text)
            if dm and prefix and dm.group(1).replace("\\", "/") != prefix:
                self.add("P3", "cfg-dir-prefix", path, text[:dm.start()].count("\n") + 1,
                         "CfgMods dir='%s' weicht von $PREFIX$='%s' ab" % (dm.group(1), prefix))
            for fm in re.finditer(r'files\[\]\s*=\s*\{([^}]*)\}', text, re.S):
                for pth in re.findall(r'"([^"]+)"', fm.group(1)):
                    self.check_runtime_path(path, text[:fm.start()].count("\n") + 1, pth, addon, prefix, must_be_dir=True)
            im = re.search(r'inputs\s*=\s*"([^"]+)"', text)
            if im:
                self.check_runtime_path(path, text[:im.start()].count("\n") + 1, im.group(1), addon, prefix, must_be_dir=False)
            # dependencies[] vs. vorhandene ScriptModule
            dep = re.search(r'dependencies\[\]\s*=\s*\{([^}]*)\}', text)
            mods = set(re.findall(r'class\s+(\w+ScriptModule)', clean))
            if dep:
                deps = set(re.findall(r'"([^"]+)"', dep.group(1)))
                want = {"engineScriptModule": "Core", "gameLibScriptModule": "GameLib", "gameScriptModule": "Game", "worldScriptModule": "World", "missionScriptModule": "Mission"}
                for m_, d_ in want.items():
                    if m_ in mods and d_ not in deps:
                        self.add("P3", "cfg-dependencies", path, text[:dep.start()].count("\n") + 1,
                                 "%s vorhanden, aber '%s' fehlt in dependencies[]" % (m_, d_))
        elif prefix and os.path.isdir(os.path.join(addon_dir, "Scripts")):
            self.add("P3", "cfg-no-cfgmods", path, 1, "Addon %s hat Scripts/, aber keinen CfgMods-Eintrag - Skripte werden nie geladen" % addon)

        # Alle deklarierten Klassen (fuer Basisklassen-Check) + Modellpfade
        for cm in re.finditer(r"class\s+(\w+)\s*(?::\s*(\w+))?\s*\{", clean):
            self.cfg_classes.add(cm.group(1))
            if cm.group(2):
                self.cfg_bases.append((cm.group(1), cm.group(2), path))
        for cm in re.finditer(r"class\s+(\w+)\s*;", clean):
            self.cfg_externs.append((cm.group(1), path, clean[:cm.start()].count("\n") + 1))
        for mm_ in re.finditer(r'model\s*=\s*"([^"]+)"', text):
            self.check_model_path(path, text[:mm_.start()].count("\n") + 1, mm_.group(1))
        for mm_ in re.finditer(r'hiddenSelectionsTextures\[\]\s*=\s*\{([^}]*)\}', text):
            for t in re.findall(r'"([^"]+)"', mm_.group(1)):
                self.check_model_path(path, text[:mm_.start()].count("\n") + 1, t)
        for m in re.finditer(r'"#(STR_[A-Za-z0-9_]+)"', text):
            self.used_str_keys.add((m.group(1), self.rel(path)))

        # CfgChefZ dataFiles[] -> Pfade pruefen
        for fm in re.finditer(r'dataFiles\[\]\s*=\s*\{([^}]*)\}', text, re.S):
            for pth in re.findall(r'"([^"]+)"', fm.group(1)):
                self.check_runtime_path(path, text[:fm.start()].count("\n") + 1, pth, addon, prefix, must_be_dir=None)

    def resolve_runtime_path(self, pth):
        """Loest einen Laufzeitpfad (PBO-Prefix/...) auf den Repo-Pfad auf."""
        p = pth.replace("\\", "/").lstrip("/")
        for addon_dir in self.addon_dirs:
            pf = os.path.join(addon_dir, "$PREFIX$")
            if not os.path.isfile(pf):
                continue
            prefix = self.read(pf).strip().replace("\\", "/").rstrip("/")
            if p.lower() == prefix.lower() or p.lower().startswith(prefix.lower() + "/"):
                rest = p[len(prefix):].lstrip("/")
                return os.path.join(addon_dir, rest.replace("/", os.sep)), prefix
        return None, None

    def check_runtime_path(self, cfg, line, pth, addon, prefix, must_be_dir):
        p = pth.replace("\\", "/")
        if prefix and not (p.lower() == prefix.lower() or p.lower().startswith(prefix.lower() + "/")):
            self.add("P0", "cfg-path-prefix", cfg, line,
                     "Pfad '%s' beginnt nicht mit dem PBO-Prefix '%s' - DayZ ueberspringt das Modul still" % (pth, prefix))
        target, _pf = self.resolve_runtime_path(p)
        if target is None:
            self.add("P0", "cfg-path-unresolved", cfg, line, "Pfad '%s' laesst sich auf keinen Addon-Prefix aufloesen" % pth)
            return
        if not os.path.exists(target):
            self.add("P0", "cfg-path-missing", cfg, line, "Pfad '%s' existiert nicht (%s)" % (pth, self.rel(target)))
            return
        if must_be_dir is True and not os.path.isdir(target):
            self.add("P0", "cfg-path-notdir", cfg, line, "files[]-Eintrag '%s' ist kein Verzeichnis" % pth)
        if must_be_dir is True and os.path.isdir(target):
            has_c = any(f.endswith(".c") for _d, _s, fs in os.walk(target) for f in fs)
            if not has_c:
                self.add("P3", "cfg-path-empty", cfg, line, "files[]-Verzeichnis '%s' enthaelt keine .c-Datei" % pth)

    def check_model_path(self, cfg, line, pth):
        p = pth.replace("\\", "/").lstrip("/")
        low = p.lower()
        if low.startswith("dz/"):
            if self.vanilla and os.path.isdir(self.vanilla):
                # Vanilla-Daten liegen nicht als Quelle vor - nur Formcheck
                pass
            return
        target, _pf = self.resolve_runtime_path(p)
        if target is None:
            self.add("P3", "model-path-unresolved", cfg, line, "Modell-/Texturpfad '%s' passt zu keinem $PREFIX$ im Repo" % pth)
            return
        if not os.path.isfile(target):
            # Case-insensitive Suche (Windows toleriert, Linux-Server nicht)
            d = os.path.dirname(target)
            base = os.path.basename(target).lower()
            if os.path.isdir(d) and any(f.lower() == base for f in os.listdir(d)):
                self.add("P4", "model-path-case", cfg, line, "Pfad '%s' stimmt nur ohne Gross/Kleinschreibung ueberein" % pth)
            else:
                self.add("P3", "model-path-missing", cfg, line, "Modell-/Texturdatei '%s' fehlt (%s)" % (pth, self.rel(target)))

    # -- stringtable.csv -------------------------------------------------------

    STRINGTABLE_HEADER = ["Language", "original", "english", "czech", "german", "russian", "polish", "hungarian", "italian", "spanish", "french", "chinese", "japanese", "portuguese", "chinesesimp"]

    def check_stringtable(self, path):
        text = self.read(path)
        reader = csv.reader(io.StringIO(text))
        rows = list(reader)
        if not rows:
            self.add("P3", "stringtable-empty", path, 1, "stringtable.csv ist leer")
            return
        header = [h.strip() for h in rows[0]]
        # Erwartet: 15 Spalten + Trailing-Komma (= 16 Felder, letztes leer)
        core = [h for h in header if h != ""]
        if core != self.STRINGTABLE_HEADER:
            self.add("P3", "stringtable-header", path, 1,
                     "Header weicht vom 15-Spalten-Layout ab (%d Spalten: %s)" % (len(core), ",".join(core[:6]) + "..."))
        expected = len(header)
        for i, row in enumerate(rows[1:], start=2):
            if not any(c.strip() for c in row):
                continue
            if len(row) != expected:
                self.add("P3", "stringtable-columns", path, i, "Zeile hat %d Felder, Header %d" % (len(row), expected), ",".join(row)[:120])
            key = row[0].strip() if row else ""
            if key:
                if key in self.stringtable_keys_where:
                    self.add("P4", "stringtable-dup", path, i, "Key '%s' bereits in %s" % (key, self.stringtable_keys_where[key]))
                self.stringtable_keys.add(key)
                self.stringtable_keys_where.setdefault(key, "%s:%d" % (self.rel(path), i))
                if len(row) >= 3 and not row[2].strip():
                    self.add("P4", "stringtable-empty-english", path, i, "Key '%s' ohne englischen Text" % key)

    # -- JSON / XML --------------------------------------------------------------

    def check_json(self, path):
        text = self.read(path)
        try:
            json.loads(text)
        except json.JSONDecodeError as e:
            self.add("P3", "json-invalid", path, e.lineno, "Ungueltiges JSON: %s" % e.msg)

    def check_xml(self, path):
        try:
            ET.parse(path)
        except ET.ParseError as e:
            self.add("P3", "xml-invalid", path, getattr(e, "position", (1, 0))[0], "Ungueltiges XML: %s" % e)
            return
        if os.path.basename(path).lower() == "inputs.xml":
            tree = ET.parse(path)
            root = tree.getroot()
            declared = set()
            for inp in root.iter("input"):
                n = inp.get("name")
                if n:
                    declared.add(n)
                loc = inp.get("loc")
                if loc:
                    self.used_str_keys.add((loc, self.rel(path)))
            for srt in root.iter("sorting"):
                loc = srt.get("loc")
                if loc:
                    self.used_str_keys.add((loc, self.rel(path)))
            self.declared_inputs.update(declared)

    # -- Cross-Checks ------------------------------------------------------------

    def cross_checks(self):
        # requiredAddons auf ChefZ_* muessen als CfgPatches existieren
        for addon, req, cfg in self.required_chefz:
            if req not in self.cfg_patches:
                ln = 1
                for i, l in enumerate(self.read(cfg).splitlines()):
                    if "requiredAddons" in l and req in l:
                        ln = i + 1
                        break
                self.add("P0", "cfg-requiredAddon-missing", cfg, ln,
                         "%s verlangt requiredAddons '%s', aber kein Addon im Repo deklariert diese CfgPatches-Klasse" % (addon, req))
        # units[] muessen irgendwo als Config-Klasse existieren
        for u, cfg in self.cfg_units:
            if u not in self.cfg_classes:
                self.add("P3", "cfg-unit-undefined", cfg, 1, "units[] nennt '%s', aber keine config.cpp definiert diese Klasse" % u)
        # Basisklassen von Config-Klassen: entweder im Repo definiert, extern deklariert (class X;) oder Vanilla
        vanilla_cfg = set()
        vcfg = os.path.join(self.vanilla, "config.cpp") if self.vanilla else None
        if vcfg and os.path.isfile(vcfg):
            vanilla_cfg = set(re.findall(r"class\s+(\w+)", self.read(vcfg)))
        for cls, base, cfg in self.cfg_bases:
            if base in self.cfg_classes:
                continue
            if base in vanilla_cfg or base in self.vanilla_layers:
                continue
            if base in {c for c, _p, _l in self.cfg_externs}:
                # extern deklariert, aber nirgends im Repo definiert -> nur Warnung, wenn nicht ChefZ_
                if base.startswith("ChefZ_"):
                    self.add("P0", "cfg-base-undefined", cfg, 1,
                             "Config-Klasse '%s' erbt von '%s', die nur extern deklariert, aber in keiner config.cpp des Repos definiert ist (VM: 'nonexistent function on EntityAIType')" % (cls, base))
                continue
            self.add("P3", "cfg-base-unknown", cfg, 1, "Config-Klasse '%s' erbt von unbekannter Basis '%s'" % (cls, base))
        # Skriptklassen fuer Config-Klassen: jede CfgVehicles-Klasse mit scope=2 sollte in Scripts existieren
        # (nicht zwingend - Vanilla-Klassen ohne Script sind moeglich) -> nur Info fuer ChefZ_-Klassen
        all_script_classes = set(self.class_layers.keys())
        for cls, base, cfg in self.cfg_bases:
            if cls.startswith("ChefZ_") and cls not in all_script_classes and cls not in self.vanilla_layers:
                self.info_no_script.append((cls, base, self.rel(cfg)))
        # STR_-Keys
        for key, where in sorted(self.used_str_keys):
            if key not in self.stringtable_keys:
                self.add("P3", "str-key-missing", os.path.join(self.root, where), 1, "STR-Key '%s' fehlt in jeder stringtable.csv - UI zeigt den rohen Key" % key)
        # Inputs: GetInputByName("...") muss in Inputs.xml deklariert sein
        for name, where, line in self.used_inputs:
            if name not in self.declared_inputs:
                self.add("P3", "input-undeclared", os.path.join(self.root, where), line, "GetInputByName('%s') ohne Deklaration in einer Inputs.xml" % name)
        # Actions: jede Klasse, die von ActionBase/Action* erbt, muss registriert sein
        for cls, where, line in self.action_classes:
            if cls not in self.registered_actions:
                self.add("P3", "action-unregistered", os.path.join(self.root, where), line,
                         "Action '%s' wird in keinem RegisterActions()/AddAction() registriert" % cls)

    def ancestor_methods(self, cls, is_modded):
        """Alle Methodennamen der Vorfahrenkette (ChefZ- und Vanilla-Klassen).
        Fuer modded classes zaehlt die Originalklasse selbst als Vorfahr."""
        names = set()
        seen = set()
        if is_modded:
            cur = cls
        else:
            entry = self.chefz_classes.get(cls)
            cur = entry[0] if entry else None
            if cur is None and cls in self.vanilla_classes:
                cur = self.vanilla_classes[cls][0]
        steps = 0
        while cur and cur not in seen and steps < 40:
            seen.add(cur)
            steps += 1
            nxt = None
            if cur in self.vanilla_classes:
                names.update(self.vanilla_classes[cur][1])
                nxt = self.vanilla_classes[cur][0]
            if cur in self.chefz_classes and not self.chefz_classes[cur][1]:
                names.update(self.chefz_classes[cur][2])
                if self.chefz_classes[cur][0]:
                    nxt = self.chefz_classes[cur][0]
            elif cur in self.chefz_classes and self.chefz_classes[cur][1] and not (is_modded and cur == cls):
                # modded class in ChefZ: ihre Methoden gehoeren zur Vanilla-Klasse -
                # ausser es ist die geprueft Klasse selbst (sonst "ueberschreibt" sie sich).
                names.update(self.chefz_classes[cur][2])
            if nxt is None and cur not in self.vanilla_classes and cur not in self.chefz_classes:
                break
            cur = nxt
        return names, seen

    def check_overrides(self):
        """override ohne Basisfunktion (P0) und Basisfunktion ohne override (P4)."""
        for path in self.walk(".c"):
            clean = self.strip_code(self.read(path))
            raw_lines = self.read(path).split("\n")
            lines = clean.split("\n")
            brace = 0
            cur = None
            cur_depth = 0
            cur_modded = False
            anc = set()
            chain = set()
            for i, l in enumerate(lines):
                cm = self.CLASS_FULL_RE.match(l)
                if cm and brace == 0:
                    cur = cm.group("name")
                    cur_depth = brace
                    cur_modded = bool(cm.group("modded"))
                    anc, chain = self.ancestor_methods(cur, cur_modded)
                if cur is not None and brace == cur_depth + 1:
                    fm = self.FUNC_DECL_START_RE.match(l)
                    if fm and not re.match(r"^\s*(if|for|while|switch|foreach|return|else)\b", l) and fm.group("ret") not in NOT_A_TYPE:
                        name = fm.group("name")
                        is_override = bool(re.match(r"^\s*(?:\w+\s+)*override\b", l))
                        is_ctor = name == cur or name.startswith("~")
                        if is_override and name not in anc and not is_ctor:
                            base_known = bool(chain & (set(self.vanilla_classes) | set(self.chefz_classes)))
                            if base_known:
                                self.add("P0", "override-nothing", path, i + 1,
                                         "override %s() in %s: keine Basisklasse der Kette (%s) deklariert diese Methode" % (name, cur, ", ".join(sorted(chain))[:120]), raw_lines[i])
                            else:
                                self.add("P3", "override-unknown-base", path, i + 1,
                                         "override %s() in %s: Basiskette nicht aufloesbar" % (name, cur), raw_lines[i])
                        elif not is_override and name in anc and not is_ctor and not re.match(r"^\s*static\b", l):
                            self.add("P4", "missing-override", path, i + 1,
                                     "%s() in %s ueberschreibt eine Basismethode ohne 'override' (RPT: FIX-ME Overriding function)" % (name, cur), raw_lines[i])
                for ch in l:
                    if ch == "{":
                        brace += 1
                    elif ch == "}":
                        brace -= 1
                        if cur is not None and brace <= cur_depth:
                            cur = None

    def scan_actions_and_inputs(self):
        action_bases = {"ActionBase", "ActionContinuousBase", "ActionSingleUseBase", "ActionInteractBase", "ActionContinuousBaseCB"}
        for path in self.walk(".c"):
            clean = self.strip_code(self.read(path))
            rel = self.rel(path)
            for m in re.finditer(r"^\s*class\s+(\w+)\s*(?::|\bextends\b)\s*(\w+)", clean, re.M):
                base = m.group(2)
                is_cb_or_data = base.endswith("CB") or "ReciveData" in base or base.endswith("ActionData") or base == "ActionData"
                if not is_cb_or_data and (base in action_bases or (base.startswith("Action") and base in self.vanilla_layers and base not in {"ActionData", "ActionTarget", "ActionConstructor", "ActionManagerBase", "ActionManagerClient", "ActionManagerServer", "ActionInput"})):
                    self.action_classes.append((m.group(1), rel, clean.count("\n", 0, m.start()) + 1))
                if base in {c for c, _r, _l in self.action_classes} and base.startswith("ChefZ_"):
                    self.action_classes.append((m.group(1), rel, clean.count("\n", 0, m.start()) + 1))
            for m in re.finditer(r"\b(?:AddAction|RegisterAction|actions\.Insert|actions\.RegisterAction|AT_\w+\s*=)\s*\(?\s*(\w+)", clean):
                self.registered_actions.add(m.group(1))
            for m in re.finditer(r"\bRegisterAction\s*\(\s*(\w+)", clean):
                self.registered_actions.add(m.group(1))
            for m in re.finditer(r"\bAddAction\s*\(\s*(\w+)", clean):
                self.registered_actions.add(m.group(1))
            for m in re.finditer(r'GetInputByName\s*\(\s*"([^"]+)"', self.read(path)):
                self.used_inputs.append((m.group(1), rel, self.read(path)[:m.start()].count("\n") + 1))

    # -- Lauf ------------------------------------------------------------------

    def run(self):
        self.addon_dirs = sorted(
            os.path.join(self.addons_dir, d) for d in os.listdir(self.addons_dir)
            if os.path.isdir(os.path.join(self.addons_dir, d))
        )
        self.used_str_keys = set()
        self.stringtable_keys_where = {}
        self.required_chefz = []
        self.cfg_units = []
        self.cfg_bases = []
        self.cfg_externs = []
        self.declared_inputs = set()
        self.used_inputs = []
        self.action_classes = []
        self.registered_actions = set()
        self.info_no_script = []

        self.build_vanilla_map()
        self.build_chefz_map()

        c_files = list(self.walk(".c"))
        total_lines = 0
        for path in c_files:
            total_lines += self.read(path).count("\n") + 1
            self.check_script(path)
        self.scan_actions_and_inputs()
        self.check_overrides()

        for addon_dir in self.addon_dirs:
            self.check_config_cpp(addon_dir)
        for path in self.walk("stringtable.csv"):
            self.check_stringtable(path)
        json_files = list(self.walk(".json"))
        for path in json_files:
            self.check_json(path)
        xml_files = list(self.walk(".xml"))
        for path in xml_files:
            self.check_xml(path)

        self.cross_checks()

        self.stats["addons"] = len(self.addon_dirs)
        self.stats["script_files"] = len(c_files)
        self.stats["script_lines"] = total_lines
        self.stats["chefz_classes"] = len(self.class_layers)
        self.stats["json_files"] = len(json_files)
        self.stats["xml_files"] = len(xml_files)
        self.stats["stringtable_keys"] = len(self.stringtable_keys)
        self.stats["action_classes"] = len(self.action_classes)

    # -- Report ----------------------------------------------------------------

    def report(self, out_md, out_json):
        order = {"P0": 0, "P1": 1, "P2": 2, "P3": 3, "P4": 4}
        fs = sorted(self.findings, key=lambda f: (order[f.prio], f.rule, f.file, f.line))
        by_prio = defaultdict(list)
        by_rule = defaultdict(int)
        for f in fs:
            by_prio[f.prio].append(f)
            by_rule[(f.prio, f.rule)] += 1

        lines = []
        w = lines.append
        w("# ChefZ Enforce-Script Audit")
        w("")
        w("Stand: %s  " % datetime.now().strftime("%Y-%m-%d %H:%M"))
        w("Wurzel: `%s`  " % self.root)
        w("Regelbasis: enforce-script Hard Rules, Safe-AI-CodingPrompt.md, Tips-Common-Pitfalls.md, DME_129_Audit_Prompt.md  ")
        w("Vanilla-Klassenkarte: %s" % self.stats.get("vanilla_map"))
        w("")
        w("## Umfang")
        w("")
        w("| Kennzahl | Wert |")
        w("|---|---|")
        for k, v in self.stats.items():
            if k == "vanilla_map":
                continue
            w("| %s | %s |" % (k, v))
        w("")
        w("## Ergebnis nach Prioritaet")
        w("")
        w("| Prio | Bedeutung | Anzahl |")
        w("|---|---|---|")
        meaning = {"P0": "Compile-Fehler / Modul wird nicht geladen", "P1": "Segfault / Crash-Risiko", "P2": "DayZ 1.29 Breaking", "P3": "Silent Failure", "P4": "Best Practice / Style"}
        for p in ["P0", "P1", "P2", "P3", "P4"]:
            w("| %s | %s | %d |" % (p, meaning[p], len(by_prio.get(p, []))))
        w("")
        w("## Ergebnis nach Regel")
        w("")
        w("| Prio | Regel | Anzahl |")
        w("|---|---|---|")
        for (p, r), n in sorted(by_rule.items(), key=lambda kv: (order[kv[0][0]], -kv[1])):
            w("| %s | %s | %d |" % (p, r, n))
        w("")
        for p in ["P0", "P1", "P2", "P3", "P4"]:
            items = by_prio.get(p, [])
            if not items:
                continue
            w("## %s - %s (%d)" % (p, meaning[p], len(items)))
            w("")
            # Style-Regeln, die massenhaft auftreten, kompakt je Datei
            compact_rules = {"indent-spaces", "indent-mixed", "multiline-call"}
            grouped = defaultdict(list)
            for f in items:
                grouped[f.rule].append(f)
            for rule, group in grouped.items():
                w("### %s (%d)" % (rule, len(group)))
                w("")
                if rule in compact_rules and len(group) > 40:
                    per_file = defaultdict(int)
                    for f in group:
                        per_file[f.file] += 1
                    w("| Datei | Fundstellen |")
                    w("|---|---|")
                    for file, n in sorted(per_file.items(), key=lambda kv: -kv[1]):
                        w("| %s | %d |" % (file, n))
                    w("")
                    continue
                for f in group:
                    w("- `%s:%d` %s" % (f.file, f.line, f.msg))
                    if f.code:
                        w("  ```c")
                        w("  " + f.code[:200])
                        w("  ```")
                w("")
        if self.info_no_script:
            w("## Info - Config-Klassen ohne eigene Skriptklasse")
            w("")
            w("Nicht zwingend ein Fehler (die Basisklasse liefert das Verhalten), aber pruefenswert, wenn ChefZ-Logik erwartet wird.")
            w("")
            for cls, base, cfg in sorted(self.info_no_script):
                w("- `%s` : %s (%s)" % (cls, base, cfg))
            w("")
        with open(out_md, "w", encoding="utf-8") as fh:
            fh.write("\n".join(lines))
        if out_json:
            with open(out_json, "w", encoding="utf-8") as fh:
                json.dump([f.as_dict() for f in fs], fh, indent=1, ensure_ascii=False)
        return by_prio


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", default=DEFAULT_ROOT)
    ap.add_argument("--vanilla", default=DEFAULT_VANILLA)
    ap.add_argument("--out", default=None)
    ap.add_argument("--json", default=None)
    args = ap.parse_args()

    out = args.out or os.path.join(args.root, "Audit", "AUDIT_REPORT.md")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    try:
        audit = Audit(args.root, args.vanilla)
        audit.run()
        by_prio = audit.report(out, args.json)
    except Exception as e:  # noqa
        import traceback
        traceback.print_exc()
        return 2
    print("Audit geschrieben: %s" % out)
    for p in ["P0", "P1", "P2", "P3", "P4"]:
        print("  %s: %d" % (p, len(by_prio.get(p, []))))
    if by_prio.get("P0") or by_prio.get("P1"):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
