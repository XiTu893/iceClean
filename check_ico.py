import struct

with open(r'F:\project\iceClean\src\gui\resources\app.ico', 'rb') as f:
    src_ico = f.read()
print(f'Source app.ico: {len(src_ico):,} bytes')
icon_count = struct.unpack_from('<H', src_ico, 4)[0]
print(f'  Icon count: {icon_count}')

for i in range(icon_count):
    off = 6 + i * 16
    w = src_ico[off] if src_ico[off] else 256
    h = src_ico[off+1] if src_ico[off+1] else 256
    size = struct.unpack_from('<I', src_ico, off+8)[0]
    data_off = struct.unpack_from('<I', src_ico, off+12)[0]
    print(f'  Entry {i}: {w}x{h} size={size} data_off={data_off}')
