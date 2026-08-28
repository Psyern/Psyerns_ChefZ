@echo off
REM ---------------------------------------------------------------------------
REM Startet den Testserver DME-Test mit @ChefZ.
REM
REM MUSS IN SITZUNG 1 (Konsole) LAUFEN. Der DayZ-Server richtet beim Start ein
REM Fenster ein und bleibt in einer getrennten RDP-Sitzung ohne aktiven Desktop
REM stehen - er schreibt dann nur den RPT-Kopf und tut danach nichts mehr, ohne
REM Fehlermeldung. Nachgewiesen am 28.08.2026: derselbe Server bleibt dort auch
REM ganz OHNE Mods stehen, es liegt also nicht am Mod.
REM
REM Die Kommandozeile ist unveraendert die, die das Deployment am 28.08. um
REM 02:40 selbst benutzt hat (aus dem Absturzprotokoll uebernommen), ergaenzt um
REM nichts. Port 2602, Profile in .\profiles.
REM
REM Danach liegen die Protokolle in
REM   D:\Agent\deployments\DME-Test\profiles\script_*.log   (Skriptfehler)
REM   D:\Agent\deployments\DME-Test\profiles\DayZServer_x64_*.RPT
REM ---------------------------------------------------------------------------
cd /d "D:\Agent\deployments\DME-Test"
start "" /D "D:\Agent\deployments\DME-Test" "D:\Agent\deployments\DME-Test\DayZServer_x64.exe" -config=serverDZ.cfg -port=2602 -profiles=profiles -adminlog -netlog -freezeCheck -dologs "-serverMod=@2464526692;" "-mod=@3689057982;@2536780687;@2931560672;@2918418331;@2276010135;@2572331007;@2116157322;@1564026768;@2545327648;@1559212036;@3571685323;@3649957186;@3649958757;@3649957536;@3649959707;@1646187754;@3164839000;@2651195301;@1832448183;@1710977250;@1932611410;@2170927235;@3690289718;@3354681846;@2471347750;@3616635518;@3759357431;@3786175534;@3783149286;@3623510671;@3627848296;@3646233886;@3780383282;@3786176249;@ChefZ;"
