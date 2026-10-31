# Gera o PackMedic.mod mínimo (sem registros) a partir do cabeçalho do KenshiAchievements.mod,
# trocando só a descrição. O FCS regrava o arquivo no upload da Workshop.
import struct
import sys

src, dst, desc = sys.argv[1], sys.argv[2], sys.argv[3].encode("utf-8")
data = open(src, "rb").read()
old_len = struct.unpack_from("<i", data, 16)[0]
tail = data[20 + old_len:]
header_len = struct.unpack_from("<i", data, 4)[0] + len(desc) - old_len
out = data[:4] + struct.pack("<i", header_len) + data[8:16] + struct.pack("<i", len(desc)) + desc + tail
open(dst, "wb").write(out)
print(dst, len(out), "bytes")
