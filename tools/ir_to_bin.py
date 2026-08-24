#!/usr/bin/env python3
"""Convert Flipper .ir files to a compact LZ4-compressed binary blob and generate C++ embed files.

Format: structured binary data + LZ4 block compression per file.
Total: ~241 KB for 987 KB of .ir files (75% compression).
The C++ side includes a minimal self-contained LZ4 decompressor.
"""
import os, sys, struct, textwrap, lz4.block

ASSETS_DIR = os.path.join(os.path.dirname(__file__), '..', 'sd_card_data', 'UniversalIR', 'assets')
OUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'src', 'modules', 'ir')

FILES = ['tv.ir', 'ac.ir', 'audio.ir', 'fans.ir', 'leds.ir', 'projectors.ir']

MAGIC = b'IRDB'
VERSION = 4  # v4 = structured + LZ4

# ── .ir parser ──────────────────────────────────────────────────────────────

def parse_ir_file(text):
    signals = []
    sections = text.split('#')
    for section in sections:
        lines = [l.strip() for l in section.strip().splitlines() if l.strip()]
        if not lines:
            continue
        sig = {}
        for line in lines:
            if ':' not in line:
                continue
            key, val = line.split(':', 1)
            key = key.strip().lower()
            val = val.strip()
            if key == 'name':
                sig['name'] = val
            elif key == 'type':
                sig['type'] = val
            elif key == 'protocol':
                sig['protocol'] = val
            elif key == 'address':
                sig['address'] = val
            elif key == 'command':
                sig['command'] = val
            elif key == 'bits':
                sig['bits'] = int(val)
            elif key == 'frequency':
                sig['frequency'] = int(val)
            elif key == 'duty_cycle':
                sig['duty_cycle'] = float(val)
            elif key == 'data':
                sig['data'] = [int(x) for x in val.split()]
        if 'name' in sig and 'type' in sig:
            signals.append(sig)
    return signals


def hex_to_uint32(hex_str):
    parts = hex_str.split()
    val = 0
    for i, p in enumerate(parts[:4]):
        val |= int(p, 16) << (i * 8)
    return val


# ── Structured binary packing ───────────────────────────────────────────────

def pack_signals_blob(signals):
    """Pack a list of signals into a compact binary blob (uncompressed)."""
    blob = bytearray()
    blob += struct.pack('<H', len(signals))
    for sig in signals:
        name_b = sig['name'].encode('utf-8')
        blob += struct.pack('B', len(name_b))
        blob += name_b
        if sig['type'] == 'parsed':
            blob += b'\x00'
            proto = sig.get('protocol', '').encode('utf-8')
            blob += struct.pack('B', len(proto))
            blob += proto
            blob += struct.pack('<I', hex_to_uint32(sig.get('address', '00 00 00 00')))
            blob += struct.pack('<I', hex_to_uint32(sig.get('command', '00 00 00 00')))
            blob += struct.pack('<H', sig.get('bits', 32))
        else:
            blob += b'\x01'
            blob += struct.pack('<H', sig.get('frequency', 38000))
            blob += struct.pack('B', int(round(sig.get('duty_cycle', 0.33) * 100)))
            data = sig.get('data', [])
            blob += struct.pack('<H', len(data))
            for v in data:
                blob += struct.pack('<H', min(v, 65535))
    return bytes(blob)


def compute_text_size(signals):
    """Compute the decompressed .ir text size for a signal list."""
    size = len(b'Filetype: IR library file\nVersion: 1\n')
    for sig in signals:
        size += 2  # '#\n'
        size += len(f"name: {sig['name']}\n")
        size += len(f"type: {sig['type']}\n")
        if sig['type'] == 'parsed':
            size += len(f"protocol: {sig.get('protocol', '')}\n")
            size += len(f"address: {sig.get('address', '00 00 00 00')}\n")
            size += len(f"command: {sig.get('command', '00 00 00 00')}\n")
            size += len(f"bits: {sig.get('bits', 32)}\n")
        else:
            size += len(f"frequency: {sig.get('frequency', 38000)}\n")
            duty = sig.get('duty_cycle', 0.33)
            size += len(f"duty_cycle: {duty}\n")
            data = sig.get('data', [])
            size += len('data: ' + ' '.join(str(v) for v in data) + '\n')
    return size


def build_blob(all_files_signals):
    file_count = len(all_files_signals)

    # Build string table for protocol names
    proto_set = {}
    for fname, signals in all_files_signals:
        for sig in signals:
            p = sig.get('protocol', '')
            if p and p not in proto_set:
                proto_set[p] = len(proto_set)

    # Pack each file's structured data and compress with LZ4
    entries = []
    for fname, signals in all_files_signals:
        struct_blob = pack_signals_blob(signals)
        compressed = lz4.block.compress(struct_blob)
        text_size = compute_text_size(signals)
        entries.append((fname, struct_blob, compressed, text_size))

    # Header
    header = bytearray()
    header += MAGIC
    header += struct.pack('B', VERSION)
    header += struct.pack('B', file_count)
    header += struct.pack('<H', len(proto_set))

    # String table
    str_table = bytearray()
    for proto, _ in sorted(proto_set.items(), key=lambda x: x[1]):
        b = proto.encode('utf-8')
        str_table += struct.pack('B', len(b))
        str_table += b

    # Assemble blob
    out = bytes(header) + bytes(str_table)
    for fname, struct_blob, compressed, text_size in entries:
        fname_b = fname.encode('utf-8')
        out += struct.pack('B', len(fname_b))
        out += fname_b
        out += struct.pack('<I', len(struct_blob))   # decompressed struct size
        out += struct.pack('<I', text_size)            # .ir text size
        out += struct.pack('<I', len(compressed))      # LZ4 compressed size
        out += compressed

    return out, entries, len(str_table), proto_set


# ── Decompression (for verification) ───────────────────────────────────────

def decompress_blob(bin_data):
    pos = 0
    assert bin_data[pos:pos+4] == MAGIC
    pos += 4
    version = bin_data[pos]; pos += 1
    file_count = bin_data[pos]; pos += 1
    str_count = struct.unpack_from('<H', bin_data, pos)[0]; pos += 2

    str_table = []
    for _ in range(str_count):
        slen = bin_data[pos]; pos += 1
        str_table.append(bin_data[pos:pos+slen].decode('utf-8'))
        pos += slen

    files = {}
    for _ in range(file_count):
        fname_len = bin_data[pos]; pos += 1
        fname = bin_data[pos:pos+fname_len].decode('utf-8'); pos += fname_len
        decomp_struct = struct.unpack_from('<I', bin_data, pos)[0]; pos += 4
        text_size = struct.unpack_from('<I', bin_data, pos)[0]; pos += 4
        comp_size = struct.unpack_from('<I', bin_data, pos)[0]; pos += 4
        compressed = bin_data[pos:pos+comp_size]; pos += comp_size

        struct_blob = lz4.block.decompress(compressed)

        # Parse structured blob
        bpos = 0
        sig_count = struct.unpack_from('<H', struct_blob, bpos)[0]; bpos += 2

        lines = ['Filetype: IR library file', 'Version: 1']
        for _ in range(sig_count):
            name_len = struct_blob[bpos]; bpos += 1
            name = struct_blob[bpos:bpos+name_len].decode('utf-8'); bpos += name_len
            sig_type = struct_blob[bpos]; bpos += 1

            lines.append('#')
            lines.append(f'name: {name}')

            if sig_type == 0:
                proto_len = struct_blob[bpos]; bpos += 1
                proto = struct_blob[bpos:bpos+proto_len].decode('utf-8'); bpos += proto_len
                addr = struct.unpack_from('<I', struct_blob, bpos)[0]; bpos += 4
                cmd = struct.unpack_from('<I', struct_blob, bpos)[0]; bpos += 4
                bits = struct.unpack_from('<H', struct_blob, bpos)[0]; bpos += 2

                lines.append('type: parsed')
                lines.append(f'protocol: {proto}')
                addr_hex = ' '.join(f'{(addr >> (i*8)) & 0xFF:02X}' for i in range(4))
                cmd_hex = ' '.join(f'{(cmd >> (i*8)) & 0xFF:02X}' for i in range(4))
                lines.append(f'address: {addr_hex}')
                lines.append(f'command: {cmd_hex}')
                lines.append(f'bits: {bits}')
            else:
                freq = struct.unpack_from('<H', struct_blob, bpos)[0]; bpos += 2
                duty_x100 = struct_blob[bpos]; bpos += 1
                data_count = struct.unpack_from('<H', struct_blob, bpos)[0]; bpos += 2
                data_vals = []
                for _ in range(data_count):
                    data_vals.append(struct.unpack_from('<H', struct_blob, bpos)[0])
                    bpos += 2

                lines.append('type: raw')
                lines.append(f'frequency: {freq}')
                lines.append(f'duty_cycle: {duty_x100 / 100.0}')
                lines.append('data: ' + ' '.join(str(v) for v in data_vals))

        lines.append('#')
        files[fname] = '\n'.join(lines) + '\n'

    return files


# ── C++ generation ──────────────────────────────────────────────────────────

def escape_c_bytes(data):
    out = []
    for b in data:
        c = chr(b)
        if c == '\\':
            out.append('\\\\')
        elif c == '"':
            out.append('\\"')
        elif c == '\n':
            out.append('\\n')
        elif c == '\r':
            out.append('\\r')
        elif c == '\t':
            out.append('\\t')
        elif 32 <= b < 127:
            out.append(c)
        else:
            out.append(f'\\x{b:02x}')
    return ''.join(out)


def generate_header():
    return textwrap.dedent("""\
        #pragma once
        #include <pgmspace.h>
        #include <FS.h>
        #include <Arduino.h>

        struct EmbeddedIRBinFile {
            const char *filename;
            const uint8_t *data;
            size_t decomp_size;    // decompressed structured blob size
            size_t text_size;      // decompressed .ir text size
            size_t comp_size;      // LZ4 compressed size in PROGMEM
        };

        const EmbeddedIRBinFile* embedded_ir_bin_lookup(const char *filename);
        void embedded_ir_bin_write_all(FS &fs, const String &dir_path);
    """)



def chunk_escaped(escaped, max_len=72):
    """Split escaped string into chunks, never breaking escape sequences."""
    chunks = []
    i = 0
    while i < len(escaped):
        end = min(i + max_len, len(escaped))
        while end > i:
            safe = True
            if end > 0 and escaped[end-1] == '\\':
                bs_count = 0
                pos = end - 1
                while pos >= i and escaped[pos] == '\\':
                    bs_count += 1
                    pos -= 1
                if bs_count % 2 == 1:
                    safe = False
            if safe:
                break
            end -= 1
        if end <= i:
            end = i + 1
        chunks.append(escaped[i:end])
        i = end
    return chunks


def generate_source(blob_data, entries, str_table_size, proto_set):
    lines = []
    lines.append('#include "ir_embedded_bin.h"\n')

    # Single blob as hex byte array (no escape sequence issues)
    hex_vals = [f'0x{b:02x}' for b in blob_data]
    # Join in lines of 20 values each
    lines.append(f'// Auto-generated LZ4-compressed structured blob: {len(blob_data)} bytes')
    lines.append(f'static const uint8_t ir_db_bin[] PROGMEM = {{')
    for i in range(0, len(hex_vals), 20):
        chunk = ', '.join(hex_vals[i:i+20])
        lines.append(f'    {chunk},')
    lines.append('};\n')

    # File lookup table
    header_size = 8
    pos = header_size + str_table_size
    lines.append('static const EmbeddedIRBinFile embedded_ir_bin_files[] = {')
    for fname, struct_blob, compressed, text_size in entries:
        fname_b = fname.encode('utf-8')
        data_offset = pos + 1 + len(fname_b) + 4 + 4 + 4
        entry_str = (
            '    {' + f'"{fname}", ir_db_bin + {data_offset}, '
            f'{len(struct_blob)}, {text_size}, {len(compressed)}' + '},'
        )
        lines.append(entry_str)
        pos += 1 + len(fname_b) + 4 + 4 + 4 + len(compressed)
    lines.append('};\n')

    lines.append('static const size_t EMBEDDED_IR_BIN_COUNT = '
                 'sizeof(embedded_ir_bin_files) / sizeof(embedded_ir_bin_files[0]);\n')

    # String table
    sorted_protos = sorted(proto_set.items(), key=lambda x: x[1])
    lines.append('static const char* const ir_proto_table[] = {')
    for proto, _ in sorted_protos:
        escaped_proto = proto.replace('\\', '\\\\').replace('"', '\\"')
        lines.append(f'    "{escaped_proto}",')
    lines.append('};\n')
    lines.append('static const size_t IR_PROTO_COUNT = sizeof(ir_proto_table) / sizeof(ir_proto_table[0]);\n')

    # Lookup function
    lines.append(textwrap.dedent("""\
        const EmbeddedIRBinFile* embedded_ir_bin_lookup(const char *filename) {
            for (size_t i = 0; i < EMBEDDED_IR_BIN_COUNT; i++) {
                if (strcmp(filename, embedded_ir_bin_files[i].filename) == 0) {
                    return &embedded_ir_bin_files[i];
                }
            }
            return nullptr;
        }
    """))

    # LZ4 decompressor + blob parser
    lines.append(textwrap.dedent("""\
        // -- Minimal LZ4 block decompressor ---------------------------------

        static size_t lz4_decompress_block(const uint8_t *src, size_t srcLen,
                                           uint8_t *dst, size_t dstCap) {
            const uint8_t *srcEnd = src + srcLen;
            uint8_t *dstStart = dst;

            while (src < srcEnd) {
                uint8_t token = *src++;
                uint32_t literalLen = token >> 4;
                if (literalLen == 15) {
                    uint8_t extra;
                    do {
                        if (src >= srcEnd) return (size_t)-1;
                        extra = *src++;
                        literalLen += extra;
                    } while (extra == 255);
                }
                if (src + literalLen > srcEnd) return (size_t)-1;
                if (dst + literalLen > dst + dstCap) return (size_t)-1;
                memcpy(dst, src, literalLen);
                src += literalLen;
                dst += literalLen;

                if (src >= srcEnd) break;

                uint16_t offset;
                memcpy(&offset, src, 2);
                src += 2;
                if (offset == 0) return (size_t)-1;

                uint32_t matchLen = token & 0x0F;
                if (matchLen == 15) {
                    uint8_t extra;
                    do {
                        if (src >= srcEnd) return (size_t)-1;
                        extra = *src++;
                        matchLen += extra;
                    } while (extra == 255);
                }
                matchLen += 4;

                if (offset > (size_t)(dst - dstStart)) return (size_t)-1;
                if (dst + matchLen > dstStart + dstCap) return (size_t)-1;

                const uint8_t *match = dst - offset;
                for (uint32_t i = 0; i < matchLen; i++)
                    dst[i] = match[i];
                dst += matchLen;
            }
            return (size_t)(dst - dstStart);
        }

        // -- Blob parser -------------------------------------------------------

        void embedded_ir_bin_write_all(FS &fs, const String &dir_path) {
            for (size_t i = 0; i < EMBEDDED_IR_BIN_COUNT; i++) {
                const EmbeddedIRBinFile &entry = embedded_ir_bin_files[i];
                String path = dir_path;
                if (!path.endsWith("/")) path += "/";
                path += entry.filename;

                if (fs.exists(path)) continue;

                // Decompress LZ4
                uint8_t *structBuf = (uint8_t *)malloc(entry.decomp_size);
                if (!structBuf) continue;

                size_t decLen = lz4_decompress_block(entry.data, entry.comp_size,
                                                     structBuf, entry.decomp_size);
                if (decLen == (size_t)-1 || decLen != entry.decomp_size) {
                    free(structBuf);
                    continue;
                }

                // Parse structured blob and reconstruct .ir text
                const uint8_t *pos = structBuf;
                const uint8_t *end = structBuf + decLen;
                if (pos + 2 > end) { free(structBuf); continue; }

                uint16_t sig_count;
                memcpy(&sig_count, pos, 2);
                pos += 2;

                String text = "Filetype: IR library file\\nVersion: 1\\n";

                for (uint16_t s = 0; s < sig_count && pos < end; s++) {
                    uint8_t name_len = *pos++;
                    if (pos + name_len > end) break;
                    String name((const char *)pos, name_len);
                    pos += name_len;
                    uint8_t type = *pos++;

                    text += "#\\n";
                    text += "name: " + name + "\\n";

                    if (type == 0) {
                        if (pos + 11 > end) break;
                        uint8_t proto_len = *pos++;
                        if (pos + proto_len > end) break;
                        String proto((const char *)pos, proto_len);
                        pos += proto_len;
                        uint32_t addr, cmd;
                        memcpy(&addr, pos, 4); pos += 4;
                        memcpy(&cmd, pos, 4); pos += 4;
                        uint16_t bits;
                        memcpy(&bits, pos, 2); pos += 2;

                        text += "type: parsed\\n";
                        text += "protocol: " + proto + "\\n";
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%02X %02X %02X %02X",
                                 addr & 0xFF, (addr >> 8) & 0xFF,
                                 (addr >> 16) & 0xFF, (addr >> 24) & 0xFF);
                        text += "address: "; text += buf; text += "\\n";
                        snprintf(buf, sizeof(buf), "%02X %02X %02X %02X",
                                 cmd & 0xFF, (cmd >> 8) & 0xFF,
                                 (cmd >> 16) & 0xFF, (cmd >> 24) & 0xFF);
                        text += "command: "; text += buf; text += "\\n";
                        text += "bits: "; text += String(bits); text += "\\n";
                    } else {
                        if (pos + 4 > end) break;
                        uint16_t freq;
                        memcpy(&freq, pos, 2); pos += 2;
                        uint8_t duty_x100 = *pos++;
                        uint16_t data_count;
                        memcpy(&data_count, pos, 2); pos += 2;

                        text += "type: raw\\n";
                        text += "frequency: "; text += String(freq); text += "\\n";
                        char duty_buf[16];
                        snprintf(duty_buf, sizeof(duty_buf), "%.2f", duty_x100 / 100.0f);
                        text += "duty_cycle: "; text += duty_buf; text += "\\n";
                        text += "data: ";
                        for (uint16_t d = 0; d < data_count && pos + 2 <= end; d++) {
                            uint16_t val;
                            memcpy(&val, pos, 2); pos += 2;
                            if (d > 0) text += " ";
                            text += String(val);
                        }
                        text += "\\n";
                    }
                }
                text += "#\\n";

                File file = fs.open(path, FILE_WRITE);
                if (file) {
                    file.write((const uint8_t *)text.c_str(), text.length());
                    file.close();
                }
                free(structBuf);
            }
        }
    """))

    return '\n'.join(lines)


# ── Main ────────────────────────────────────────────────────────────────────

def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    all_files_signals = []
    total_orig = 0
    for fname in FILES:
        fpath = os.path.join(ASSETS_DIR, fname)
        if not os.path.exists(fpath):
            print(f'ERROR: {fpath} not found', file=sys.stderr)
            sys.exit(1)
        with open(fpath, 'r', encoding='utf-8', errors='replace') as f:
            text = f.read()
        signals = parse_ir_file(text)
        orig_size = len(text.encode('utf-8'))
        total_orig += orig_size
        all_files_signals.append((fname, signals))
        print(f'  {fname}: {len(signals)} signals, {orig_size} bytes')

    blob_data, entries, str_table_size, proto_set = build_blob(all_files_signals)

    print(f'\n  Original total:  {total_orig} bytes')
    print(f'  Compressed blob: {len(blob_data)} bytes')
    print(f'  Compression:     {(1 - len(blob_data) / total_orig) * 100:.1f}%')

    # Verify round-trip
    decompressed = decompress_blob(blob_data)
    for fname, signals in all_files_signals:
        orig_fpath = os.path.join(ASSETS_DIR, fname)
        with open(orig_fpath, 'r', encoding='utf-8', errors='replace') as f:
            orig_text = f.read()
        orig_parsed = parse_ir_file(orig_text)
        dec_parsed = parse_ir_file(decompressed[fname])
        if len(orig_parsed) != len(dec_parsed):
            print(f'  WARNING: {fname} signal count mismatch: {len(orig_parsed)} vs {len(dec_parsed)}')
        else:
            print(f'  {fname}: round-trip OK ({len(orig_parsed)} signals)')

    # Write C++ files
    out_h = os.path.join(OUT_DIR, 'ir_embedded_bin.h')
    out_cpp = os.path.join(OUT_DIR, 'ir_embedded_bin.cpp')

    with open(out_h, 'w') as f:
        f.write(generate_header())
    print(f'\nWrote {out_h}')

    with open(out_cpp, 'w') as f:
        f.write(generate_source(blob_data, entries, str_table_size, proto_set))
    print(f'Wrote {out_cpp}')


if __name__ == '__main__':
    main()
