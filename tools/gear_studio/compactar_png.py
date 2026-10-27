#!/usr/bin/env python3
"""Recompacta PNGs no máximo sem mudar nenhum pixel: abre o fluxo de dados da
imagem e grava de novo com compressão nível 9. Uso: compactar_png.py arquivos..."""
import struct
import sys
import zlib


def chunks(data):
    pos = 8
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        yield kind, data[pos + 8:pos + 8 + length]
        pos += 12 + length


def chunk(kind, body):
    return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)


def compact(path):
    data = open(path, "rb").read()
    out, idat = [], b""
    for kind, body in chunks(data):
        if kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            out.append(chunk(b"IDAT", zlib.compress(zlib.decompress(idat), 9)))
            out.append(chunk(kind, body))
        else:
            out.append(chunk(kind, body))
    new = data[:8] + b"".join(out)
    if len(new) < len(data):
        open(path, "wb").write(new)
    return len(data), min(len(new), len(data))


if __name__ == "__main__":
    before = after = 0
    for p in sys.argv[1:]:
        b, a = compact(p)
        before += b
        after += a
    print(f"{before / 1e6:.1f} MB -> {after / 1e6:.1f} MB")
