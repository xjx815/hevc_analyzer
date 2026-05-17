#!/usr/bin/env python3
"""Generate a minimal valid HEIF test file with HEVC content."""
import struct, sys

def box(typ, data=b''):
    if isinstance(typ, str): typ = typ.encode()
    return struct.pack('>I', 8 + len(data)) + typ + data

def fullbox(typ, version, flags, data=b''):
    if isinstance(typ, str): typ = typ.encode()
    return box(typ, struct.pack('>I', (version << 24) | flags) + data)

# Dummy NAL units
vps  = bytes.fromhex('40010c01ffff0160000003000003000003000003009d0000')
sps  = bytes.fromhex('420101010160000003000003000003000003009d0000')
pps  = bytes.fromhex('4401a55808')
idr  = bytes.fromhex('2601af00000300000300')
trail = bytes.fromhex('0201af081a0000030000')

# Build hvcC
hvcC = bytearray()
hvcC.append(1)  # configurationVersion
hvcC.extend(bytes.fromhex('01600000000000000000005df000fcfcf8f8000003'))
hvcC.append(3)  # numOfArrays
for naltype, nalu in [(32, vps), (33, sps), (34, pps)]:
    hvcC.append(naltype & 0x3F)
    hvcC.extend(struct.pack('>H', 1))
    hvcC.extend(struct.pack('>H', len(nalu)))
    hvcC.extend(nalu)
hvcC = bytes(hvcC)

# mdat payload
mdat_payload = b''
for nalu in [idr, trail, trail, trail]:
    mdat_payload += struct.pack('>I', len(nalu)) + nalu

# hvc1 sample entry
hvc1_data = bytearray()
hvc1_data.extend(bytes(6))
hvc1_data.extend(struct.pack('>H', 1))
hvc1_data.extend(bytes(4))
hvc1_data.extend(bytes(12))
hvc1_data.extend(struct.pack('>HH', 512, 512))
hvc1_data.extend(struct.pack('>II', 0x00480000, 0x00480000))
hvc1_data.extend(bytes(4))
hvc1_data.extend(struct.pack('>H', 1))
hvc1_data.extend(bytes(32))
hvc1_data.extend(struct.pack('>H', 0x0018))
hvc1_data.extend(struct.pack('>h', -1))
hvc1_data.extend(box('hvcC', hvcC))
hvc1_box = box('hvc1', bytes(hvc1_data))

stsd_box = fullbox('stsd', 0, 0, struct.pack('>I', 1) + hvc1_box)
stts_box = fullbox('stts', 0, 0, struct.pack('>II', 4, 1000))
stsc_box = fullbox('stsc', 0, 0, struct.pack('>III', 1, 4, 1))

stsz_data = struct.pack('>II', 0, 4)
for n in [idr, trail, trail, trail]:
    stsz_data += struct.pack('>I', len(n))
stsz_box = fullbox('stsz', 0, 0, stsz_data)

# Build from inside out
stbl = box('stbl', stsd_box + stts_box + stsc_box + stsz_box)

dref_data = struct.pack('>I', 1) + struct.pack('>I', 12) + b'url ' + struct.pack('>I', 0x00000001)
dref_box = fullbox('dref', 0, 0, dref_data)
dinf = box('dinf', dref_box)

vmhd_box = fullbox('vmhd', 0, 1, struct.pack('>HHH', 0, 0, 0))

minf = box('minf', vmhd_box + dinf + stbl)

hdlr_data = struct.pack('>I', 0) + b'vide' + bytes(12) + b'\0'
hdlr_box = fullbox('hdlr', 0, 0, hdlr_data)

mdhd_data = struct.pack('>IIII', 0, 0, 1000, 4000) + struct.pack('>HH', 0x55c4, 0)
mdhd_box = fullbox('mdhd', 0, 0, mdhd_data)

mdia = box('mdia', mdhd_box + hdlr_box + minf)

tkhd_data = struct.pack('>IIIII', 0, 0, 1, 0, 4000)
tkhd_data += bytes(8)
tkhd_data += struct.pack('>HH', 0, 0)
tkhd_data += struct.pack('>HH', 0x0100, 0)
for v in [0x00010000,0,0, 0,0x00010000,0, 0,0,0x40000000]:
    tkhd_data += struct.pack('>I', v)
tkhd_data += struct.pack('>II', 0x02000000, 0x02000000)
tkhd_box = fullbox('tkhd', 0, 7, tkhd_data)

trak = box('trak', tkhd_box + mdia)

mvhd_data = struct.pack('>IIIII', 0, 0, 0, 1000, 4000)
mvhd_data += struct.pack('>I', 0x00010000)
mvhd_data += struct.pack('>HH', 0x0100, 0)
mvhd_data += bytes(8)
for v in [0x00010000,0,0, 0,0x00010000,0, 0,0,0x40000000]:
    mvhd_data += struct.pack('>I', v)
mvhd_data += bytes(24)
mvhd_data += struct.pack('>I', 2)
mvhd_box = fullbox('mvhd', 0, 0, mvhd_data)

moov = box('moov', mvhd_box + trak)

# ftyp
ftyp = box('ftyp', b'mif1' + struct.pack('>I', 0) + b'heic' + b'mif1')

# stco: chunk offset = after ftyp + final_moov + mdat_header
# Need to account for stco itself being added to moov
stco_box_no_data = fullbox('stco', 0, 0, struct.pack('>II', 1, 0))
stco_size = len(stco_box_no_data)
chunk_offset = len(ftyp) + len(moov) + stco_size + 8
stco_data = struct.pack('>II', 1, chunk_offset)
stco_box = fullbox('stco', 0, 0, stco_data)

# Rebuild with stco in stbl
stbl = box('stbl', stsd_box + stts_box + stsc_box + stsz_box + stco_box)
minf = box('minf', vmhd_box + dinf + stbl)
mdia = box('mdia', mdhd_box + hdlr_box + minf)
trak = box('trak', tkhd_box + mdia)
moov = box('moov', mvhd_box + trak)

mdat = box('mdat', mdat_payload)

outpath = sys.argv[1] if len(sys.argv) > 1 else '/Users/xujianxin/ai_project/hevc_extractor/build/test.heic'
with open(outpath, 'wb') as f:
    f.write(ftyp + moov + mdat)

total = len(ftyp) + len(moov) + len(mdat)
print(f'Generated {outpath}: {total} bytes')
print(f'  ftyp={len(ftyp)}  moov={len(moov)}  mdat={len(mdat)}')
print(f'  hvcC arrays: VPS={len(vps)}B SPS={len(sps)}B PPS={len(pps)}B')
print(f'  mdat samples: 4 (1 IDR + 3 TRAIL)')
