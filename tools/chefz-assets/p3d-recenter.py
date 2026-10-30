"""Verschiebt alle LODs einer MLOD-P3D um die Mitte der LOD0-Huelle (Vorschau-Fix 30.10.2026)."""
import struct, sys
def lod_ranges(b):
    n = struct.unpack("<I", b[8:12])[0]; i = 12; out = []
    for _ in range(n):
        hs, ver, npts, nnorm, nfaces, flags = struct.unpack("<6I", b[i+4:i+28])
        pstart = i + 28; i = pstart + npts * 16 + nnorm * 12
        for _ in range(nfaces):
            i += 72
            for _ in range(2): i = b.index(b"\0", i) + 1
        i += 4
        while True:
            e = b.index(b"\0", i+1); name = b[i+1:e].decode(); i = e + 1
            ln = struct.unpack("<I", b[i:i+4])[0]; i += 4 + ln
            if name == "#EndOfFile#": break
        res = struct.unpack("<f", b[i:i+4])[0]; i += 4
        out.append((res, pstart, npts))
    assert i == len(b)
    return out
for path in sys.argv[1:]:
    b = bytearray(open(path, "rb").read())
    lods = lod_ranges(b)
    res0, p0, n0 = lods[0]
    pts = [struct.unpack_from("<3f", b, p0 + k*16) for k in range(n0)]
    ctr = [(min(p[a] for p in pts) + max(p[a] for p in pts)) / 2 for a in range(3)]
    for res, ps, n in lods:
        for k in range(n):
            x, y, z = struct.unpack_from("<3f", b, ps + k*16)
            struct.pack_into("<3f", b, ps + k*16, x - ctr[0], y - ctr[1], z - ctr[2])
    open(path, "wb").write(b)
    lod_ranges(bytes(b))
    print(path.split("/")[-1], "verschoben um", [round(-c, 4) for c in ctr])
