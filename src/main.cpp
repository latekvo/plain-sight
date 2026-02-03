#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <random>

// #include "argp.h"
#include "tree.hpp"

// note: stride_in_bytes is the amount of bytes in a row, leave at 0 for
// automatic note: code like this: vvv
/*	if (enc_dec) {
 *		//ENCODING
 * 	} else {
 *		//DECODING
 *  }
 */
void print_help() {
  std::cout << "decode: [image] (png | bmp)" << std::endl;
  std::cout << "encode: [image] [message]" << std::endl;
}

int main(int argc, char* argv[]) {
  std::deque<unsigned char> byte_queue;  // modified string
  size_t bpp = 1;
  size_t seed = 0x42;  // initial doesn't matter, can be used as a key

  // std::vector<arg_info> arglist = arg_parser(argc, argv);

  bool enc_dec;  // true - enc, false - dec
  switch (argc) {
    case 2:
      enc_dec = false;
      break;
    case 3:
      enc_dec = true;
      break;
    default:
      print_help();
      return 1;
  }

  int x, y, mode, raw_mode;
  // c array of 8-bits
  unsigned char* raw_data = stbi_load(argv[1], &x, &y, &raw_mode, 0);
  // read up the docs, how is the color info reall stored?
  unsigned char bit_mask[x * y * raw_mode];
  for (int i = 0; i < x * y * raw_mode; i++) bit_mask[i] = 0x0;
  Tree<size_t, bool> occupation;
  if (raw_mode == 4) {
    mode = 3;  // alpha is the fourth, we want to avoid alpha
  } else {
    mode = raw_mode;
  }
  // fixed reversion, checked, works as intended
  if (enc_dec) {
    FILE* file_txt = fopen(argv[2], "r");
    if (!file_txt) {
      std::cout << "failed opening payload file, treating as a string litral"
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
    // last 5 bits get chopped off, why? why not? this fixes it
    byte_queue.push_back(0x0);
    byte_queue.push_back(0x0);
    fclose(file_txt);
  }
  // bind (i think) supplies the second argument's return to first argument,
  // creates a functor out of that
  auto rng = std::bind(std::uniform_int_distribution<int>(0, x * y - 1),
                       std::mt19937(seed));

  int b_cnt = 0;
  unsigned char byte = 0x0;

  if (enc_dec) {
    byte = byte_queue.front();
    byte_queue.pop_front();
  }
  while ((0 < byte_queue.size() && enc_dec) || (!enc_dec)) {
    int r_coor = rng() * raw_mode;

    if (occupation.find(r_coor)) {
      continue;
    }

    occupation.add(r_coor, true);

    // ocuppy pixel
    for (int ch = 0; ch < mode; ch++) {
      if (b_cnt == 8) {
        b_cnt = 0;
        if (enc_dec) {
          byte = byte_queue.front();
          byte_queue.pop_front();
          std::cout << "\n";
        } else {
          std::cout << "\n";
          if (byte == 0x0) goto end_dec;
          byte_queue.push_back(byte);
          byte = 0x0;
        }
      }

      // bitmask holds all the changes
      for (int b = 0; b < bpp; b++) {
        // TODO: both need to store used pixels somehow, though i could deal
        // with the corruption later
        if (enc_dec) {
          std::cout << (unsigned int)byte;
          bit_mask[r_coor + ch] = (bit_mask[r_coor + ch] << 1) | (byte & 1);
          byte >>= 1;
          std::cout << " , " << (int)bit_mask[r_coor + ch] << "\n";  // DEBG
        } else {
          std::cout << (unsigned int)(raw_data[r_coor + ch] & 1);
          byte <<= 1;
          byte |= raw_data[r_coor + ch] & 1;
          raw_data[r_coor + ch] >>= 1;
          std::cout << (unsigned int)(byte & 1) << "\n";
        }
        b_cnt++;
        if (b_cnt == 8) goto n_pixel;
      }
    }
  n_pixel:
    continue;  // don't be mad, this avoids additional ifs
  }

end_dec:

  if (enc_dec) {
    for (int i = 0; i < x * y * raw_mode; i++) {
      // could use foo &= ~x
      raw_data[i] = (raw_data[i] >> bpp << bpp) | bit_mask[i];
    }
    // will use switch after adding flags support
    //  BMP, PNG tested
    stbi_write_png("output.png", x, y, raw_mode, raw_data, x * raw_mode);
    stbi_write_bmp("output.bmp", x, y, raw_mode, raw_data);
    // JPG DOESNT WORK stbi_write_jpg("out/output.jpg", x, y, raw_mode,
    // raw_data, 100);
  } else {
    std::cout << "END_DEC, length: " << byte_queue.size() << "\n";
    for (int i = 0; i < byte_queue.size(); i++) {
      std::cout << byte_queue[i];
    }
    std::cout << std::endl;
  }

  return 0;
}
