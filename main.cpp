#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include "LZWDecoder/decoder.h"

using namespace std;

int main(int argc, char *argv[]) {
    uint32_t chunk_count = 0;
    uint32_t unique_chunk_count = 0;
    uint32_t duplicate_chunk_count = 0;
    uint32_t unique_chunk_reduce = 0;
    uint32_t duplicate_chunk_reduce = 0;

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <Compressed file> <Decompressed file>\n";
        return EXIT_SUCCESS;
    }

    ifstream input;
    ofstream output;
    input.open(argv[1], std::ios::binary);
    output.open(argv[2], std::ios::binary);
    if (!input.good()) {
        std::cerr << "Could not open input file.\n";
        return EXIT_FAILURE;
    }
    if (!output.good()) {
        std::cerr << "Could not open output file.\n";
        return EXIT_FAILURE;
    }

    vector<string> chunks;
    uint8_t src_buffer[5760];
    uint8_t decode_buffer[5760];

    while (true) {
        uint32_t header;
        input.read((char *) &header, sizeof(int32_t));
        if (input.eof())
            break;
        chunk_count++;

        if ((header & 3u) == 0) {
            // this chunk has been compressed by lzw algorithm
            int chunk_size = header >> 2u;

            // read in data
            input.read((char *)src_buffer, chunk_size);
            if (chunk_size != input.gcount()) {
                cout << "fail to read enough data!" << endl;
                exit(1);
            }

            // decode and write out
            uint32_t decode_length = lzw_decode(src_buffer, decode_buffer, chunk_size);
            string chunk = string((char *)decode_buffer, decode_length);
            chunks.push_back(chunk);
            output.write(&chunk[0], chunk.length());

            // do some analysis
            unique_chunk_count++;
            unique_chunk_reduce += chunk.length() - chunk_size;

        } else if ((header & 3u) == 1u) {
            // this is a duplicate chunk
            int location = header >> 2u;

            // defensive programming to avoid out-of-bounds reference
            if (location < chunks.size()) {
                const std::string &chunk = chunks[location];
                output.write(&chunk[0], chunk.length());
                duplicate_chunk_count++;
                duplicate_chunk_reduce += (chunk.length() - 4);
            } else {
                cout << "duplicate chunk " << location << " does not exist!" << endl;
                cout << "at chunk " << chunk_count << endl;
                exit(1);
            }

        } else if ((header & 3u) == 2u) {
            // this is a raw chunk, we can just read it and write to the output stream
            int chunk_size = header >> 2u;
            input.read((char *)src_buffer, chunk_size);
            string chunk = string((char *)src_buffer, chunk_size);
            chunks.push_back(chunk);
            output.write((char *)src_buffer, chunk_size);

        } else {
            // can not figure out the chunk type
            cout << "chunk type error" << endl;
            exit(1);
        }
    }

    uint32_t total_reduce = duplicate_chunk_reduce + unique_chunk_reduce;

    cout << "total chunk count " << chunk_count << endl;
    cout << "duplicate chunk count " << duplicate_chunk_count << endl;
    cout << "unique chunk count " << unique_chunk_count << endl;
    cout << "duplicate chunk length reduced by " << duplicate_chunk_reduce << " bytes" << endl;
    cout << "unique chunk length reduced by " << unique_chunk_reduce << " bytes" << endl;
    cout << "duplication contribution " << (double) duplicate_chunk_reduce / total_reduce << endl;
    cout << "compression contribution " << (double) unique_chunk_reduce / total_reduce << endl;

    return EXIT_SUCCESS;
}