#ifndef SIMPLEPNGENCODER_SIMPLE_PNG_ENCODER_H
#define SIMPLEPNGENCODER_SIMPLE_PNG_ENCODER_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <zlib.h>
#include <android/log.h>

const char *TAG = "SimplePngEncoder";
#define ALOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))

namespace simplepng {
    /// constants begin
    // file header
    const uint8_t HEADER_U8_ARRAY[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    const size_t HEADER_U8_ARRAY_SIZE = sizeof(HEADER_U8_ARRAY) / sizeof(uint8_t);
    // chunk types
    const uint8_t IHDR[] = {'I', 'H', 'D', 'R'};
    const uint8_t IDAT[] = {'I', 'D', 'A', 'T'};
    const uint8_t IEND[] = {'I', 'E', 'N', 'D'};
    const uint32_t CHUNK_TYPE_SIZE = sizeof(IEND) / sizeof(uint8_t);
    // IEND
    const uint8_t IEND_U8_ARRAY[] = {0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
                                     0x42,
                                     0x69, 0x82};
    const size_t IEND_U8_ARRAY_SIZE = sizeof(IEND_U8_ARRAY) / sizeof(uint8_t);
    const size_t BUFFER_SIZE = 128;
    /// constants end

    // png use big endian
    // big endian of `uint32_t`
    void big_endian_u32(std::vector<uint8_t> &result, uint32_t u32) {
        const size_t size = sizeof(uint32_t);
        for (int i = size - 1; i >= 0; i--) {
            const uint8_t u8 = static_cast<const uint8_t>(u32 >> (i * 8));
            result.push_back(u8);
        }
    }

    // big endian of `uint8_t`
    void big_endian_u8(std::vector<uint8_t> &result, uint8_t u8) {
        result.push_back(u8);
    }

    // big endian of `uint8_t[]`
    void big_endian_u8_array(std::vector<uint8_t> &result, const uint8_t *array, size_t size) {
        for (unsigned int i = 0; i < size; i++) {
            result.push_back(array[i]);
        }
    }

    /// IHDR Chunk
    struct ChunkIHDR {
        // width, height <= 2^31 - 1
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        // 8: true color of 8bit
        uint8_t m_bit_depth = 8;
        // 2: true color
        uint8_t m_color_type = 2;
        // always 0
        uint8_t m_compress_method = 0;
        // 0: no filter
        uint8_t m_filter_method = 0;
        // 0: no interlace
        // 1: Adam7
        uint8_t m_interlace_method = 0;
    };

    std::vector<uint8_t> create_ihdr_bytes(uint32_t width, uint32_t height) {
        ChunkIHDR ihdr;
        ihdr.m_width = width;
        ihdr.m_height = height;

        std::vector<uint8_t> result;
        result.reserve(sizeof(ChunkIHDR));
        big_endian_u32(result, ihdr.m_width);
        big_endian_u32(result, ihdr.m_height);
        big_endian_u8(result, ihdr.m_bit_depth);
        big_endian_u8(result, ihdr.m_color_type);
        big_endian_u8(result, ihdr.m_compress_method);
        big_endian_u8(result, ihdr.m_filter_method);
        big_endian_u8(result, ihdr.m_interlace_method);

        return std::move(result);
    }

    /// IDAT Chunk
    void deflate_u8_array(std::vector<uint8_t> &result, uint8_t *array, size_t size) {
        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        int err_code = deflateInit(&stream, 3); // compress level = 3
        if (err_code != Z_OK) {
            ALOGE("Failed to init deflate stream!");
            return;
        }

        stream.avail_in = size;
        stream.next_in = array;
        uint8_t buffer[BUFFER_SIZE];
        do {
            stream.avail_out = BUFFER_SIZE;
            stream.next_out = buffer;
            deflate(&stream, Z_SYNC_FLUSH);

            big_endian_u8_array(result, buffer, BUFFER_SIZE - stream.avail_out);
        } while (stream.avail_out == 0);

        deflateEnd(&stream);
    }

    std::vector<uint8_t> create_idat_bytes(uint8_t *image_data, uint32_t width, uint32_t height) {
        std::vector<uint8_t> result;

        // filter
        size_t size = (3 * width + 1) * height; // might overflow
        std::unique_ptr<uint8_t[]> filtered_image_data(new uint8_t[size]);
        for (uint32_t i = 0; i < height; i++) {
            uint32_t filtered_offset = i * (3 * width + 1);
            uint32_t offset = i * (3 * width);
            // filter type is none
            filtered_image_data[filtered_offset] = 0;
            memcpy(filtered_image_data.get() + filtered_offset + 1, image_data + offset,
                   width * 3);
        }
        // deflate
        deflate_u8_array(result, filtered_image_data.get(), size);
        return std::move(result);
    }

    class PngChunk {
    public:
        PngChunk(const uint8_t *chunk_type) {
            memcpy(m_chunk_type, chunk_type, CHUNK_TYPE_SIZE);
        }

        void setData(std::vector<uint8_t> &&data) {
            m_data = data;
        }

        std::vector<uint8_t> getBytes() {
            std::vector<uint8_t> result;

            // length
            uint32_t length = m_data.size();
            big_endian_u32(result, length);
            // chunk type
            big_endian_u8_array(result, m_chunk_type, CHUNK_TYPE_SIZE);
            // data
            big_endian_u8_array(result, m_data.data(), m_data.size());
            // crc
            uint32_t crc = 0;
            crc = crc32(crc, m_chunk_type, CHUNK_TYPE_SIZE);
            crc = crc32(crc, m_data.data(), m_data.size());
            big_endian_u32(result, crc);

            return std::move(result);
        }

    private:
        uint8_t m_chunk_type[CHUNK_TYPE_SIZE];
        std::vector<uint8_t> m_data;
    };

    class FileWriter {
    private:
        FILE *m_fp;
    public:
        FileWriter(FILE *fp) : m_fp(fp) {}

        ~FileWriter() {
            fclose(m_fp);
        }

        void writeU8Array(const uint8_t *array, size_t size) {
            fwrite(array, sizeof(uint8_t), size, m_fp);
        }

        void writeChunk(PngChunk &chunk) {
            std::vector<uint8_t> bytes = chunk.getBytes();
            writeU8Array(bytes.data(), bytes.size());
        }
    };

    void test_write_png(std::string filename) {
        FILE *fp = fopen(filename.c_str(), "wb");
        if (fp == nullptr) {
            ALOGE("Failed to open file: %s", filename.c_str());
            return;
        }
        FileWriter writer(fp);
        // rgb image for test
        uint32_t width = 100;
        uint32_t height = 100;
        std::unique_ptr<uint8_t[]> image_data(new uint8_t[width * height * 3]);
        for (uint32_t i = 0; i < height; i++) {
            for (uint32_t j = 0; j < width; j++) {
                // R=0, G=255, B=0
                uint32_t offset = (i * width + j) * 3;
                image_data[offset] = 0x00;
                image_data[offset + 1] = 0xff;
                image_data[offset + 2] = 0x00;
            }
        }
        // write file header
        writer.writeU8Array(HEADER_U8_ARRAY, HEADER_U8_ARRAY_SIZE);
        // write IHDR Chunk
        PngChunk ihdr_chunk(IHDR);
        ihdr_chunk.setData(create_ihdr_bytes(width, height));
        writer.writeChunk(ihdr_chunk);
        // write IDAT Chunk
        PngChunk idat_chunk(IDAT);
        idat_chunk.setData(create_idat_bytes(image_data.get(), width, height));
        writer.writeChunk(idat_chunk);
        // write IEND Chunk
        writer.writeU8Array(IEND_U8_ARRAY, IEND_U8_ARRAY_SIZE);
    }
}
#endif //SIMPLEPNGENCODER_SIMPLE_PNG_ENCODER_H
