"""Legt stehende Fische hin (30.10.2026): (x,y,z) -> (y,z,x) fuer Punkte und Normalen
aller LODs, danach Mitte der LOD0-Huelle auf den Ursprung, Land-Contact-Punkte an die
unteren Ecken der neuen Huelle."""
import struct, sys
def parse(b):
    n = struct.unpack("<I", b[8:12])[0]; i = 12; out = []
    for _ in range(n):
        hs, ver, npts, nnorm, nfaces, flags = struct.unpack("<6I", b[i+4:i+28])
        ps = i + 28; ns = ps + npts * 16; i = ns + nnorm * 12
        for _ in range(nfaces):
            i += 72
            for _ in range(2): i = b.index(b"\0", i) + 1
        i += 4; sels = {}
        while True:
            e = b.index(b"\0", i+1); name = b[i+1:e].decode(); i = e + 1
            ln = struct.unpack("<I", b[i:i+4])[0]; i += 4
            if not name.startswith("#"): sels[name] = bytes(b[i:i+npts])
            i += ln
            if name == "#EndOfFile#": break
        res = struct.unpack("<f", b[i:i+4])[0]; i += 4
        out.append(dict(res=res, ps=ps, n=npts, ns=ns, nn=nnorm, sels=sels))
    assert i == len(b)
    return out
rot = lambda v: (v[1], v[2], v[0])
for path in sys.argv[1:]:
    b = bytearray(open(path, "rb").read()); lods = parse(b)
    for l in lods:
        for k in range(l["n"]):
            o = l["ps"] + k*16; struct.pack_into("<3f", b, o, *rot(struct.unpack_from("<3f", b, o)))
        for k in range(l["nn"]):
            o = l["ns"] + k*12; struct.pack_into("<3f", b, o, *rot(struct.unpack_from("<3f", b, o)))
    l0 = lods[0]
    pts = [struct.unpack_from("<3f", b, l0["ps"] + k*16) for k in range(l0["n"])]
    mn = [min(p[a] for p in pts) for a in range(3)]; mx = [max(p[a] for p in pts) for a in range(3)]
    ctr = [(mn[a] + mx[a]) / 2 for a in range(3)]
    for l in lods:
        for k in range(l["n"]):
            o = l["ps"] + k*16; x, y, z = struct.unpack_from("<3f", b, o)
            struct.pack_into("<3f", b, o, x - ctr[0], y - ctr[1], z - ctr[2])
    hx = (mx[0] - mn[0]) / 2; hy = (mx[1] - mn[1]) / 2; hz = (mx[2] - mn[2]) / 2
    corners = {"lc_front_left": (-hx, -hy, hz), "lc_front_right": (hx, -hy, hz), "lc_rear_left": (-hx, -hy, -hz), "lc_rear_right": (hx, -hy, -hz)}
    for l in lods:
        if "lc_front_left" not in l["sels"]: continue
        for name, c in corners.items():
            w = l["sels"].get(name)
            for k in range(l["n"]):
                if w and w[k]: struct.pack_into("<3f", b, l["ps"] + k*16, *c)
    open(path, "wb").write(b); parse(bytes(b))
    print(path.split("/")[-1], "Huelle", [round(2*h, 3) for h in (hx, hy, hz)])
