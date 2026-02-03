#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <random>

#include "tree.hpp"

void print_help() {
  std::cout << "decode: [image]" << std::endl;
  std::cout << "encode: [image] [message]" << std::endl;
}

int main(int argc, char* argv[]) {
  std::deque<unsigned char> byte_queue;  // modified string
  size_t bpp = 1;

  // This is the de-facto key for the message.
  // TODO: Allow for setting this, or even integrate with GPG
  size_t seed = 0x42;

  // std::vector<arg_info> arglist = arg_parser(argc, argv);

  bool shouldEncode;
  switch (argc) {
    case 2:
      shouldEncode = false;
      break;
    case 3:
      shouldEncode = true;
      break;
    default:
      print_help();
      return 1;
  }

  int x, y, mode, raw_mode;
  unsigned char* raw_data = stbi_load(argv[1], &x, &y, &raw_mode, 0);
  unsigned char bit_mask[x * y * raw_mode];

  // TODO: Simplify, use memset
  for (int i = 0; i < x * y * raw_mode; i++) {
    bit_mask[i] = 0;
  }

  Tree<size_t, bool> occupation;
  if (raw_mode == 4) {
    // Mode 4 includes alpha, which we don't want to change.
    mode = 3;
  } else {
    mode = raw_mode;
  }

  if (shouldEncode) {
    FILE* file_txt = fopen(argv[2], "r");
    if (!file_txt) {
      std::cout << "Failed opening payload file, treating as a string litral"
                << std::endl;
      file_txt = fopen("tmp.txt", "w");

      fwrite(argv[2], sizeof(char), sizeof(argv[2]), file_txt);

      fclose(file_txt);
      file_txt = fopen("tmp.txt", "r");
    }

    int c = EOF;
    while ((c = fgetc(file_txt)) != EOF) {
      char rev_c = 0x0;
      for (size_t i = 0; i < 8; i++) {
        rev_c <<= 1;
        rev_c |= (c & 1);
        c >>= 1;
      }
      byte_queue.push_back(rev_c);
    }

    // FIXME: This is a workaround for the last 5 bits getting removed
    byte_queue.push_back(0x0);
    byte_queue.push_back(0x0);
    fclose(file_txt);
  }

  auto rng = std::bind(std::uniform_int_distribution<int>(0, x * y - 1),
                       std::mt19937(seed));

  int b_cnt = 0;
  unsigned char byte = 0x0;

  if (shouldEncode) {
    byte = byte_queue.front();
    byte_queue.pop_front();
  }

  // TODO: Is the second shouldEncode necessary?
  while (!shouldEncode || (byte_queue.size() > 0 && shouldEncode)) {
    int r_coor = rng() * raw_mode;

    if (occupation.find(r_coor)) {
      // TODO: Exit after >5k failed retries.
      //       Raising bpp will resolve this issue.
      continue;
    }

    occupation.add(r_coor, true);

    for (int ch = 0; ch < mode; ch++) {
      if (b_cnt == 8) {
        b_cnt = 0;
        if (shouldEncode) {
          byte = byte_queue.front();
          byte_queue.pop_front();
        } else {
          if (byte == 0x0) goto end_dec;
          byte_queue.push_back(byte);
          byte = 0x0;
        }
      }

      // bitmask holds all the changes
      for (size_t b = 0; b < bpp; b++) {
        if (shouldEncode) {
          bit_mask[r_coor + ch] = (bit_mask[r_coor + ch] << 1) | (byte & 1);
          byte >>= 1;
        } else {
          byte <<= 1;
          byte |= raw_data[r_coor + ch] & 1;
          raw_data[r_coor + ch] >>= 1;
        }
        b_cnt++;
        if (b_cnt == 8) goto n_pixel;
      }
    }
  n_pixel:
    continue;
  }

end_dec:

  if (shouldEncode) {
    for (int i = 0; i < x * y * raw_mode; i++) {
      // Could use foo &= ~x
      raw_data[i] = (raw_data[i] >> bpp << bpp) | bit_mask[i];
    }

    // TODO: Add optional format selection
    // TODO: For JPG, we have to use a different (fourier based?)
    //       method for encoding-decoding

    stbi_write_png("output.png", x, y, raw_mode, raw_data, x * raw_mode);
    stbi_write_bmp("output.bmp", x, y, raw_mode, raw_data);
  } else {
    for (size_t i = 0; i < byte_queue.size(); i++) {
      std::cout << byte_queue[i];
    }

    std::cout << std::endl;
  }

  return 0;
}
