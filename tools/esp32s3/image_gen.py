import sys
OFFSET_START = 0x0
# OFFSET_BOOTLOADER = 0x1000
OFFSET_BOOTLOADER = 0x0
OFFSET_PARTITIONS = 0x8000
OFFSET_OTA_DATA_INITIAL = 0xd000
OFFSET_APPLICATION = 0x10000
# OFFSET_END = 0x3dffff
OFFSET_END = 0x190000
files_in = [
    ('bootloader', OFFSET_BOOTLOADER, sys.argv[1]),
    ('partitions', OFFSET_PARTITIONS, sys.argv[2]),
    ('ota_data_initial', OFFSET_OTA_DATA_INITIAL, sys.argv[3]),
    ('application', OFFSET_APPLICATION, sys.argv[4]),
]
file_out = sys.argv[5]

# cur_offset = OFFSET_BOOTLOADER
with open(file_out, 'wb') as fout:
    print(" ")
    cur_offset = OFFSET_START
    print('%-14s%-10s%-12s' % ("partition", "offset", "size")) 
    for name, offset, file_in in files_in:
#        assert offset >= cur_offset
        fout.write(b'\xff' * (offset - cur_offset))
        cur_offset = offset
        with open(file_in, 'rb') as fin:
            data = fin.read()
            fout.write(data)
            cur_offset += len(data)
            print('%-14s(0x)%-6x(0d)%-6d' % (name, offset, len(data))) 
    offset = OFFSET_END
    fout.write(b'\xff' * (offset - cur_offset))
    print('%-14s(0x)%-6x(0d)%-6d' % ('target_QIO', 0, OFFSET_END))
    print("package success.")