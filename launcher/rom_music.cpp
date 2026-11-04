// A cartridge dump's menu music (rom_music.h): the dump's file table (its FNT and
// FAT, from the header) finds data/sound_data.sdat; the archive's SYMB block names
// its streams, INFO gives each one's file, its FAT where that file is; the stream
// (STRM) is IMA ADPCM in blocks, one block per channel in turn, decoded here. Only
// the bytes needed are read: a dump is 64 to 128 MB, the music a few hundred KB.
#include "rom_music.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace {

struct Reader {
    std::ifstream f;
    bool ok = true;
    std::vector<uint8_t> at(uint64_t off, size_t n) {
        std::vector<uint8_t> v(n);
        if (!f.seekg((std::streamoff)off) || !f.read((char*)v.data(), (std::streamsize)n)) { ok = false; v.clear(); }
        return v;
    }
};

uint32_t u32(const std::vector<uint8_t>& v, size_t o) {
    return o + 4 <= v.size() ? v[o] | v[o + 1] << 8 | v[o + 2] << 16 | (uint32_t)v[o + 3] << 24 : 0;
}
uint16_t u16(const std::vector<uint8_t>& v, size_t o) { return o + 2 <= v.size() ? v[o] | v[o + 1] << 8 : 0; }

std::string cstr(const std::vector<uint8_t>& v, size_t o) {
    std::string s;
    while (o < v.size() && v[o]) s += (char)v[o++];
    return s;
}

// the file id of `want` (a/b/c) in the dump's name table, or -1
int find_file(Reader& r, uint32_t fnt_off, uint32_t fnt_size, const std::string& want) {
    std::vector<uint8_t> fnt = r.at(fnt_off, fnt_size);
    if (fnt.empty()) return -1;
    int found = -1;
    // directories: 0xF000 the root; each one's entries at its offset, with its first file id
    std::vector<std::pair<uint16_t, std::string>> todo = {{0xF000, ""}};
    while (!todo.empty() && found < 0) {
        auto [did, path] = todo.back();
        todo.pop_back();
        size_t e = 8 * (did & 0xFFF);
        uint32_t eoff = u32(fnt, e);
        int fid = u16(fnt, e + 4);
        size_t p = eoff;
        while (p < fnt.size()) {
            uint8_t b = fnt[p++];
            if (!b) break;
            size_t n = b & 0x7F;
            if (p + n > fnt.size()) break;
            std::string name((const char*)&fnt[p], n);
            p += n;
            if (b & 0x80) {
                todo.push_back({u16(fnt, p), path + name + "/"});
                p += 2;
            } else {
                if (path + name == want) { found = fid; break; }
                ++fid;
            }
        }
    }
    return found;
}

const int kIndex[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
const int kStep[89] = {7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,
                       25,    28,    31,    34,    37,    41,    45,    50,    55,    60,    66,    73,    80,
                       88,    97,    107,   118,   130,   143,   157,   173,   190,   209,   230,   253,   279,
                       307,   337,   371,   408,   449,   494,   544,   598,   658,   724,   796,   876,   963,
                       1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,  3327,
                       3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487,
                       12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

// one ADPCM block: its first sample and step index, then two samples a byte
void decode_block(const uint8_t* b, size_t len, size_t want, std::vector<int16_t>& out) {
    if (len < 4) return;
    int pcm = (int16_t)(b[0] | b[1] << 8), idx = std::clamp(b[2] | b[3] << 8, 0, 88);
    size_t n = 0;
    for (size_t i = 4; i < len && n < want; ++i)
        for (int half = 0; half < 2 && n < want; ++half) {
            int nib = half ? b[i] >> 4 : b[i] & 15, step = kStep[idx], diff = step >> 3;
            if (nib & 1) diff += step >> 2;
            if (nib & 2) diff += step >> 1;
            if (nib & 4) diff += step;
            pcm = std::clamp(nib & 8 ? pcm - diff : pcm + diff, -32768, 32767);
            idx = std::clamp(idx + kIndex[nib & 7], 0, 88);
            out.push_back((int16_t)pcm);
            ++n;
        }
}

}  // namespace

bool rom_music_load(const std::string& rom_path, const char* strm, RomMusic& out, std::string* err) {
    auto fail = [&](const char* why) { if (err) *err = why; return false; };
    Reader r;
    r.f.open(std::filesystem::path((const char8_t*)rom_path.c_str()), std::ios::binary);
    if (!r.f) return fail("cannot open the dump");
    std::vector<uint8_t> h = r.at(0, 0x200);
    if (h.empty()) return fail("not a dump");
    uint32_t fnt_off = u32(h, 0x40), fnt_size = u32(h, 0x44), fat_off = u32(h, 0x48), fat_size = u32(h, 0x4C);
    if (!fnt_off || !fnt_size || fnt_size > (16u << 20) || !fat_off) return fail("no file table");
    int fid = find_file(r, fnt_off, fnt_size, "data/sound_data.sdat");
    if (fid < 0 || (uint32_t)(8 * fid + 8) > fat_size) return fail("no sound archive");
    std::vector<uint8_t> fe = r.at(fat_off + 8 * fid, 8);
    uint64_t sdat = u32(fe, 0), sdat_end = u32(fe, 4);
    if (!r.ok || sdat_end <= sdat) return fail("bad file table");
    std::vector<uint8_t> sh = r.at(sdat, 0x40);
    if (std::memcmp(sh.data(), "SDAT", 4)) return fail("not a sound archive");
    uint32_t blocks[8] = {};
    int nblocks = u16(sh, 0x0E);
    for (int i = 0; i < 8; ++i) blocks[i] = u32(sh, 0x10 + 4 * i);
    if (nblocks == 3) {                 // no SYMB: INFO, FAT, FILE follow
        for (int i = 7; i >= 2; --i) blocks[i] = blocks[i - 2];
        blocks[0] = blocks[1] = 0;
    }
    uint32_t symb_off = blocks[0], symb_size = blocks[1], info_off = blocks[2], info_size = blocks[3],
             sfat_off = blocks[4], sfat_size = blocks[5];
    if (!symb_off || symb_size > (4u << 20) || info_size > (4u << 20) || sfat_size > (4u << 20)) return fail("no names in the archive");
    std::vector<uint8_t> symb = r.at(sdat + symb_off, symb_size), info = r.at(sdat + info_off, info_size),
                         sfat = r.at(sdat + sfat_off, sfat_size);
    if (!r.ok) return fail("archive cut short");
    const int kStrm = 7;                // SEQ SEQARC BANK WAVEARC PLAYER GROUP PLAYER2 STRM
    uint32_t so = u32(symb, 8 + 4 * kStrm), io = u32(info, 8 + 4 * kStrm);
    if (!so || !io) return fail("no streams");
    uint32_t count = u32(symb, so);
    int index = -1;
    for (uint32_t i = 0; i < count && index < 0; ++i) {
        uint32_t no = u32(symb, so + 4 + 4 * i);
        if (no && cstr(symb, no) == strm) index = (int)i;
    }
    if (index < 0) return fail("no such stream");
    uint32_t ro = u32(info, io + 4 + 4 * index);
    if (!ro) return fail("stream without a record");
    uint32_t sfid = u16(info, ro);
    uint32_t nfat = u32(sfat, 8);
    if (sfid >= nfat) return fail("stream without a file");
    uint64_t file_off = sdat + u32(sfat, 12 + 16 * sfid), file_size = u32(sfat, 16 + 16 * sfid);
    if (file_size < 0x68 || file_size > (64u << 20)) return fail("stream of an odd size");
    std::vector<uint8_t> s = r.at(file_off, (size_t)file_size);
    if (!r.ok || std::memcmp(s.data(), "STRM", 4)) return fail("not a stream");
    int type = s[0x18], channels = s[0x1A];
    out = RomMusic();
    out.looped = s[0x19] != 0;
    out.rate = u16(s, 0x1C);
    out.loop = u32(s, 0x20);
    uint32_t samples = u32(s, 0x24), data = u32(s, 0x28), nblk = u32(s, 0x2C), blen = u32(s, 0x30),
             spb = u32(s, 0x34), lastlen = u32(s, 0x38), lastspb = u32(s, 0x3C);
    if (type != 2 || channels < 1 || channels > 2 || !out.rate || !nblk || !blen) return fail("a stream of a kind not decoded (not ADPCM)");
    std::vector<int16_t> ch[2];
    for (int c = 0; c < channels; ++c) ch[c].reserve(samples);
    size_t p = data;
    for (uint32_t b = 0; b < nblk; ++b)
        for (int c = 0; c < channels; ++c) {
            size_t len = b + 1 == nblk ? lastlen : blen, want = b + 1 == nblk ? lastspb : spb;
            if (p + len > s.size()) { b = nblk; break; }
            decode_block(&s[p], len, want, ch[c]);
            p += len;
        }
    size_t n = ch[0].size();
    if (channels == 2) n = std::min(n, ch[1].size());
    if (!n) return fail("an empty stream");
    out.pcm.resize(2 * n);
    for (size_t i = 0; i < n; ++i) {
        out.pcm[2 * i] = ch[0][i];
        out.pcm[2 * i + 1] = channels == 2 ? ch[1][i] : ch[0][i];
    }
    if (out.loop >= n) out.loop = 0;
    return true;
}
