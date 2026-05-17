// Generate a minimal valid HEIF test file with HEVC content
// g++ -std=c++11 -o gen_heif generate_test_heif.cpp && ./gen_heif test.heic

#include <stdio.h>
#include <string.h>
#include <vector>
#include <stdint.h>

static void w32(FILE* f, uint32_t v) {
    uint8_t b[4] = {(uint8_t)(v>>24),(uint8_t)(v>>16),(uint8_t)(v>>8),(uint8_t)v};
    fwrite(b,1,4,f);
}
static void w16(FILE* f, uint16_t v) {
    uint8_t b[2] = {(uint8_t)(v>>8),(uint8_t)v};
    fwrite(b,1,2,f);
}
static void w8(FILE* f, uint8_t v) { fwrite(&v,1,1,f); }
static void w4(FILE* f, const char* s) { fwrite(s,1,4,f); }
static void wb(FILE* f, const uint8_t* d, size_t n) { fwrite(d,1,n,f); }
static void wz(FILE* f, size_t n) { for(size_t i=0;i<n;i++) w8(f,0); }

static void writeMatrix(FILE* f) {
    w32(f,0x00010000); w32(f,0); w32(f,0);
    w32(f,0); w32(f,0x00010000); w32(f,0);
    w32(f,0); w32(f,0); w32(f,0x40000000);
}

// Dummy NAL units
static const uint8_t vpsNalu[] = {
    0x40,0x01,0x0c,0x01,0xff,0xff,0x01,0x60,0x00,0x00,
    0x03,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x00,0x03,
    0x00,0x9d,0x00,0x00
};
static const uint8_t spsNalu[] = {
    0x42,0x01,0x01,0x01,0x01,0x60,0x00,0x00,0x03,0x00,
    0x00,0x03,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x9d,
    0x00,0x00
};
static const uint8_t ppsNalu[] = {
    0x44,0x01,0xa5,0x58,0x08
};
static const uint8_t idrNalu[] = {
    0x26,0x01,0xaf,0x00,0x00,0x03,0x00,0x00,0x03,0x00
};
static const uint8_t trailNalu[] = {
    0x02,0x01,0xaf,0x08,0x1a,0x00,0x00,0x03,0x00,0x00
};

int main(int argc, char* argv[]) {
    const char* outPath = (argc > 1) ? argv[1] : "test.heic";
    FILE* f = fopen(outPath, "wb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", outPath); return 1; }

    // ============================================================
    // Pre-build sub-box payloads
    // ============================================================

    // hvcC box payload
    std::vector<uint8_t> hvcC;
    hvcC.push_back(1); // configurationVersion = 1
    uint8_t ptl[] = {
        0x01,                           // profile_space+tier+profile_idc
        0x60,0x00,0x00,0x00,            // compatibility_flags
        0x00,0x00,0x00,0x00,0x00,0x00,  // constraint_indicator_flags
        0x5d,                           // level_idc
        0xf0,0x00,                      // min_spatial_segmentation
        0xfc,0xfc,                      // parallelismType + chroma
        0xf8,0xf8,                      // bit_depth luma+chroma
        0x00,0x00,                      // avgFrameRate
        0x00                            // constantFrameRate, numTemporalLayers, temporalIdNested, lengthSizeMinusOne
    };
    hvcC.insert(hvcC.end(), ptl, ptl+sizeof(ptl));
    hvcC.push_back(3);  // numOfArrays = 3 (VPS, SPS, PPS)

    auto addArray = [&](uint8_t nalType, const uint8_t* data, size_t len) {
        hvcC.push_back(nalType & 0x3F);  // completeness(1)=0, reserved(1)=0, NAL_unit_type(6)
        hvcC.push_back(0x00); hvcC.push_back(0x01);  // numNalus = 1
        uint16_t nlen = (uint16_t)len;
        hvcC.push_back((uint8_t)(nlen>>8)); hvcC.push_back((uint8_t)nlen);
        hvcC.insert(hvcC.end(), data, data+len);
    };
    addArray(32, vpsNalu, sizeof(vpsNalu));
    addArray(33, spsNalu, sizeof(spsNalu));
    addArray(34, ppsNalu, sizeof(ppsNalu));

    // mdat payload: 4 length-prefixed NAL units
    std::vector<uint8_t> mdatPayload;
    auto addSample = [&](const uint8_t* d, size_t len) {
        uint32_t sz = (uint32_t)len;
        mdatPayload.push_back((uint8_t)(sz>>24));
        mdatPayload.push_back((uint8_t)(sz>>16));
        mdatPayload.push_back((uint8_t)(sz>>8));
        mdatPayload.push_back((uint8_t)sz);
        mdatPayload.insert(mdatPayload.end(), d, d+len);
    };
    addSample(idrNalu, sizeof(idrNalu));
    addSample(trailNalu, sizeof(trailNalu));
    addSample(trailNalu, sizeof(trailNalu));
    addSample(trailNalu, sizeof(trailNalu));

    uint32_t sampleSizes_arr[4] = {
        (uint32_t)sizeof(idrNalu), (uint32_t)sizeof(trailNalu),
        (uint32_t)sizeof(trailNalu), (uint32_t)sizeof(trailNalu)
    };

    // ============================================================
    // Calculate all box sizes (data only, without 8-byte header)
    // ============================================================

    // hvcC box: 8-byte header + payload
    uint32_t hvcCSize = 8 + (uint32_t)hvcC.size();

    // hvc1 sample entry inside stsd:
    // VisualSampleEntry base (86 bytes) + hvcC box
    uint32_t hvc1Size = 8 + 86 + hvcCSize;

    // stsd: 8-byte header + 4(version/flags) + 4(entry_count) + entries
    uint32_t stsdSize = 8 + 4 + 4 + hvc1Size;

    // stts: 8-byte header + 4(version/flags) + 4(entry_count) + 1*(4+4)
    uint32_t sttsSize = 8 + 4 + 4 + 8;

    // stsc: 8-byte header + 4(version/flags) + 4(entry_count) + 1*(4+4+4)
    uint32_t stscSize = 8 + 4 + 4 + 12;

    // stsz: 8-byte header + 4(version/flags) + 4(sample_size) + 4(count) + 4*4
    uint32_t stszSize = 8 + 4 + 4 + 4 + 16;

    // mdat offset = ftyp_size + moov_header(8) + rest_of_moov + mdat_header(8)
    // We'll compute this later; use a pre-calculated value
    // ftyp=24, moov_header=8, moov_body, mdat_header=8
    // moov_body = mvhd + trak
    // mvhd = 8 + 100 = 108
    // trak = 8 + tkhd(92) + mdia(mdhd(32) + hdlr(33) + minf(vmhd(20) + dinf(dref(28)) + stbl(stsd + stts + stsc + stsz + stco)))
    // stco: 8 + 4 + 4 + 4 = 20
    uint32_t stcoSize = 8 + 4 + 4 + 4;

    uint32_t stblSize = 8 + stsdSize + sttsSize + stscSize + stszSize + stcoSize;

    uint32_t drefSize = 8 + 4 + 4 + 12 + 4;  // +4 for the url entry header
    // Actually dref box: 8(header) + 4(version/flags) + 4(entry_count) + url_entry
    // url_entry: 4(size) + 4(type="url ") + 4(version/flags=1)
    // = 8 + 4 + 4 + 4 + 4 + 4 = 28
    drefSize = 28;

    uint32_t dinfSize = 8 + drefSize;
    uint32_t vmhdSize = 8 + 4 + 4;  // 8(header) + 4(version/flags) + 4(graphics+opcolor)

    // Actually vmhd: 8(header) + 4(version/flags) + 2(graphics) + 2*2(opcolor) = 8+4+2+4=18
    // But spec says 20 bytes total. Let me check.
    // vmhd: version(1)+flags(3)=4, graphicsmode(2)=2, opcolor(2+2+2)=6 = 12 bytes data
    // 8 + 12 = 20
    vmhdSize = 20;

    uint32_t minfSize = 8 + vmhdSize + dinfSize + stblSize;

    // hdlr: 8(header) + 4(version/flags) + 4(pre_defined) + 4(handler_type) + 3*4(reserved) + 1(name) = 8+4+4+4+12+1 = 33
    uint32_t hdlrSize = 33;

    // mdhd: 8(header) + 4(version/flags) + 2*4(creation/mod) + 4(timescale) + 4(duration) + 2(lang) + 2(predef) = 8+4+8+4+4+2+2 = 32
    uint32_t mdhdSize = 32;

    uint32_t mdiaSize = 8 + mdhdSize + hdlrSize + minfSize;

    // tkhd: 8(header) + 84(data) = 92
    uint32_t tkhdSize = 92;

    uint32_t trakSize = 8 + tkhdSize + mdiaSize;

    // mvhd: 8(header) + 100(data) = 108
    // data: 4(version/flags) + 4*3(times) + 4(rate) + 2(vol) + 2(resv) + 2*4(resv) + 36(matrix) + 6*4(predef) + 4(next_id) = 4+12+4+2+2+8+36+24+4 = 96+4=100
    // Let me recount: 4 + 4+4+4+4=16 + 4=4 + 2+2=4 + 4+4=8 + 36 + 24 + 4 = 4+20+4+4+8+36+24+4=104
    // Hmm, I keep getting inconsistent counts. Let me count from the write calls:
    // w32 version/flags = 4
    // w32 creation_time = 4
    // w32 modification_time = 4
    // w32 timescale = 4
    // w32 duration = 4
    // w32 rate = 4
    // w16 volume + w16 reserved = 4
    // w32 + w32 reserved = 8
    // writeMatrix = 9*w32 = 36
    // 6 * w32 pre_defined = 24
    // w32 next_track_id = 4
    // TOTAL = 4+4+4+4+4+4+4+8+36+24+4 = 100 bytes
    uint32_t mvhdSize = 8 + 100;  // 108

    // ============================================================
    // moov total size
    // ============================================================
    uint32_t moovBodySize = mvhdSize + trakSize; // includes their headers
    uint32_t moovSize = 8 + moovBodySize; // + moov's own header

    // ============================================================
    // ftyp
    // ============================================================
    uint32_t ftypSize = 8 + 4 + 4 + 4 + 4;  // 24

    // mdat
    uint32_t mdatSize = 8 + (uint32_t)mdatPayload.size();

    // ============================================================
    // Now write the file
    // ============================================================

    // --- ftyp ---
    w32(f, ftypSize); w4(f, "ftyp");
    w4(f, "mif1");     // major brand
    w32(f, 0);          // minor version
    w4(f, "heic");     // compatible
    w4(f, "mif1");     // compatible

    // --- moov ---
    w32(f, moovSize); w4(f, "moov");

    // --- mvhd ---
    w32(f, mvhdSize); w4(f, "mvhd");
    w32(f, 0);          // version=0, flags=0
    w32(f, 0);          // creation_time
    w32(f, 0);          // modification_time
    w32(f, 1000);       // timescale
    w32(f, 4000);       // duration
    w32(f, 0x00010000); // rate 1.0
    w16(f, 0x0100);     // volume
    w16(f, 0);          // reserved
    w32(f, 0); w32(f, 0);// reserved[2]
    writeMatrix(f);     // 36 bytes
    for(int i=0;i<6;i++) w32(f,0); // pre_defined[6]
    w32(f, 2);          // next_track_id

    // --- trak ---
    w32(f, trakSize); w4(f, "trak");

    // --- tkhd ---
    w32(f, tkhdSize); w4(f, "tkhd");
    w32(f, 0x00000007); // version=0, flags=7
    w32(f, 0); w32(f, 0); // creation, modification
    w32(f, 1);          // track_ID
    w32(f, 0);          // reserved
    w32(f, 4000);       // duration
    w32(f, 0); w32(f, 0); // reserved[2]
    w16(f, 0); w16(f, 0); // layer, alternate_group
    w16(f, 0x0100); w16(f, 0); // volume, reserved
    writeMatrix(f);
    w32(f, 0x02000000); // width
    w32(f, 0x02000000); // height

    // --- mdia ---
    w32(f, mdiaSize); w4(f, "mdia");

    // --- mdhd ---
    w32(f, mdhdSize); w4(f, "mdhd");
    w32(f, 0);          // version, flags
    w32(f, 0); w32(f, 0); // creation, modification
    w32(f, 1000);       // timescale
    w32(f, 4000);       // duration
    w16(f, 0x55c4);     // language
    w16(f, 0);          // pre_defined

    // --- hdlr ---
    w32(f, hdlrSize); w4(f, "hdlr");
    w32(f, 0);          // version, flags
    w32(f, 0);          // pre_defined
    w4(f, "vide");      // handler_type
    w32(f, 0); w32(f, 0); w32(f, 0); // reserved[3]
    w8(f, 0);           // name

    // --- minf ---
    w32(f, minfSize); w4(f, "minf");

    // --- vmhd ---
    w32(f, vmhdSize); w4(f, "vmhd");
    w32(f, 0x00000001); // version=0, flags=1
    w16(f, 0);          // graphics_mode
    w16(f, 0); w16(f, 0); // opcolor

    // --- dinf ---
    w32(f, dinfSize); w4(f, "dinf");

    // --- dref ---
    w32(f, drefSize); w4(f, "dref");
    w32(f, 0);          // version, flags
    w32(f, 1);          // entry_count
    w32(f, 12); w4(f, "url "); w32(f, 0x00000001); // url entry

    // --- stbl ---
    w32(f, stblSize); w4(f, "stbl");

    // --- stsd ---
    w32(f, stsdSize); w4(f, "stsd");
    w32(f, 0);          // version, flags
    w32(f, 1);          // entry_count

    // --- hvc1 ---
    w32(f, hvc1Size); w4(f, "hvc1");
    wz(f, 6);           // reserved
    w16(f, 1);          // data_reference_index
    w16(f, 0); w16(f, 0); // pre_defined, reserved
    w32(f, 0); w32(f, 0); w32(f, 0); // pre_defined[3]
    w16(f, 512); w16(f, 512); // width, height
    w32(f, 0x00480000); w32(f, 0x00480000); // resolution
    w32(f, 0);          // reserved
    w16(f, 1);          // frame_count
    wz(f, 32);          // compressor_name
    w16(f, 0x0018);     // depth
    w16(f, 0xFFFF);     // pre_defined

    // --- hvcC ---
    w32(f, hvcCSize); w4(f, "hvcC");
    wb(f, hvcC.data(), hvcC.size());

    // --- stts ---
    w32(f, sttsSize); w4(f, "stts");
    w32(f, 0);          // version, flags
    w32(f, 1);          // entry_count
    w32(f, 4); w32(f, 1000); // 4 samples, duration 1000 each

    // --- stsc ---
    w32(f, stscSize); w4(f, "stsc");
    w32(f, 0);          // version, flags
    w32(f, 1);          // entry_count
    w32(f, 1); w32(f, 4); w32(f, 1); // first_chunk=1, samples_per_chunk=4, desc_index=1

    // --- stsz ---
    w32(f, stszSize); w4(f, "stsz");
    w32(f, 0);          // version, flags
    w32(f, 0);          // sample_size = 0 (variable)
    w32(f, 4);          // sample_count
    for (int i = 0; i < 4; i++) w32(f, sampleSizes_arr[i]);

    // --- stco ---
    // chunk offset: ftyp(24) + moovSize + mdat_header(8)
    uint32_t mdatStart = 24 + moovSize + 8;
    w32(f, stcoSize); w4(f, "stco");
    w32(f, 0);          // version, flags
    w32(f, 1);          // entry_count
    w32(f, mdatStart);  // chunk offset

    // --- mdat ---
    w32(f, mdatSize); w4(f, "mdat");
    wb(f, mdatPayload.data(), mdatPayload.size());

    fclose(f);

    printf("Generated %s\n", outPath);
    printf("  Total size: %u bytes\n", ftypSize + moovSize + mdatSize);
    printf("  ftyp: %u  moov: %u  mdat: %u\n", ftypSize, moovSize, mdatSize);
    printf("  VPS=%zuB SPS=%zuB PPS=%zuB  mdat NALUs: 4\n",
           sizeof(vpsNalu), sizeof(spsNalu), sizeof(ppsNalu));
    return 0;
}
