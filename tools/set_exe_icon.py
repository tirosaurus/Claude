"""Sustituye el icono incrustado en el .exe de Windows exportado por Godot
(sin rcedit): reescribe cada RT_ICON con un PNG del mismo tamaño.

Uso: python3 tools/set_exe_icon.py builds/windows/Vaelmoor.exe game/icon.png
"""
import io
import struct
import sys
import pefile
from PIL import Image


def main(exe, icon_png):
    pe = pefile.PE(exe)
    src = Image.open(icon_png).convert("RGBA")
    rt = {e.id: e for e in pe.DIRECTORY_ENTRY_RESOURCE.entries}
    # tamaños por id desde RT_GROUP_ICON
    group = rt[14].directory.entries[0].directory.entries[0].data.struct
    gdata = bytearray(pe.get_data(group.OffsetToData, group.Size))
    count = struct.unpack_from("<H", gdata, 4)[0]
    dims = {}
    for i in range(count):
        off = 6 + i * 14
        w, h = gdata[off], gdata[off + 1]
        rid = struct.unpack_from("<H", gdata, off + 12)[0]
        dims[rid] = (w or 256, h or 256, off)
    done = 0
    for entry in rt[3].directory.entries:
        rid = entry.id
        d = entry.directory.entries[0].data.struct
        if rid not in dims:
            continue
        w, h, goff = dims[rid]
        buf = io.BytesIO()
        src.resize((w, h), Image.NEAREST).save(buf, "PNG", optimize=True)
        png = buf.getvalue()
        if len(png) > d.Size:
            print(f"  icono {w}x{h}: no cabe ({len(png)} > {d.Size}), se deja")
            continue
        off = pe.get_offset_from_rva(d.OffsetToData)
        pe.__data__ = pe.__data__[:off] + png + b"\0" * (d.Size - len(png)) + pe.__data__[off + d.Size:] \
            if isinstance(pe.__data__, bytes) else pe.__data__
        pe.set_bytes_at_offset(off, png + b"\0" * (d.Size - len(png)))
        d.Size = len(png)
        struct.pack_into("<I", gdata, goff + 8, len(png))
        struct.pack_into("<HH", gdata, goff + 4, 1, 32)
        done += 1
    goffset = pe.get_offset_from_rva(group.OffsetToData)
    pe.set_bytes_at_offset(goffset, bytes(gdata))
    pe.write(exe + ".tmp")
    import os
    os.replace(exe + ".tmp", exe)
    print(f"iconos sustituidos: {done}/{len(dims)}")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
