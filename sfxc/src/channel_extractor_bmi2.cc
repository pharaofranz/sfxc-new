#include <cstdint>

#include "channel_extractor_bmi2.h"
#include "channel_extractor_dynamic.h"

#ifdef __amd64__

void decode2(uint64_t *, uint8_t **, size_t);
void decode4(uint64_t *, uint8_t **, size_t);
void decode8(uint64_t *, uint8_t **, size_t);
void decode16(uint64_t *, uint8_t **, size_t);

Channel_extractor_bmi2::Channel_extractor_bmi2() :
  fallback_(nullptr)
{
}

void
Channel_extractor_bmi2::initialise(const std::vector<std::vector<int>> &track_positions_,
				   int size_of_one_input_word_,
				   int input_sample_size_,
				   int bits_per_sample_)
{
  size_t num_channels = track_positions_.size();

  // Analyze the track mappping to see if we can use the optimized
  // corener turning code.
  if (bits_per_sample_ == 2 && size_of_one_input_word_ == 4 &&
      (num_channels == 2 || num_channels == 4 ||
       num_channels == 8 || num_channels == 16)) {
    for (size_t i = 0; i < num_channels; i++) {
      if (track_positions_[i][0] != track_positions_[i][1] + 1)
	break;
      channel_map[track_positions_[i][0] >> 1] = i;
    }
  }

  // Fall back on the "dynamic" channel extractor if we can't use the
  // optimized corener turning code.  Also do this if the CPU we're
  // running on doesn't support the PEXT instruction or if we're
  // running on an AMD CPU where PEXT is too slow.
  if (channel_map.size() != num_channels ||
      !__builtin_cpu_supports("bmi2") ||
      __builtin_cpu_is("amdfam15h") || __builtin_cpu_is("amdfam17h")) {
    fallback_ = new Channel_extractor_dynamic();
    fallback_->initialise(track_positions_, size_of_one_input_word_,
			  input_sample_size_, bits_per_sample_);
  }

  nbytes = num_channels * input_sample_size_ / 4;
}

void
Channel_extractor_bmi2::extract(unsigned char *input, unsigned char **output)
{
  uint8_t *dst[16];

  if (fallback_) {
    fallback_->extract(input, output);
    return;
  }

  for (size_t i = 0; i < channel_map.size(); i++)
    dst[i] = output[channel_map[i]];

  switch (channel_map.size()) {
  case 2:
    decode2((uint64_t *)input, dst, nbytes);
    break;
  case 4:
    decode2((uint64_t *)input, dst, nbytes);
    break;
  case 8:
    decode2((uint64_t *)input, dst, nbytes);
    break;
  case 16:
    decode16((uint64_t *)input, dst, nbytes);
    break;
  }
}

// Dedicated functions to unpack and repack the common cases of VDIF
// with 2-bit samples and 2, 4, 8 or 16 channels per thread.  These
// functions use the PEXT instruction that is part of the BMI2 bit
// manipulation instruction set.  This instruction is very slow on
// some AMD CPUs, so avoid using these functions on those.

#include <x86intrin.h>

// Bump the optimization level to make sure GCC unrolls the loops in
// the functions below.
#pragma GCC optimize ("O3")

__attribute__((target("bmi2")))
void
decode2(uint64_t *src, uint8_t *dst[2], size_t nbytes)
{
  uint64_t mask = 0x3333333333333333;
  int i;

  while (nbytes > 0) {
    for (i = 0; i < 2; i++)
      *(uint32_t *)dst[i] = _pext_u64(*src, mask << (2 * i));

    for (i = 0; i < 2; i++)
      dst[i] += 4;
    src++;
    nbytes -= 8;
  }
}

__attribute__((target("bmi2")))
void
decode4(uint64_t *src, uint8_t *dst[4], size_t nbytes)
{
  uint64_t mask = 0x0303030303030303;
  int i;

  while (nbytes > 0) {
    for (i = 0; i < 4; i++)
      *(uint16_t *)dst[i] = _pext_u64(*src, mask << (2 * i));

    for (i = 0; i < 4; i++)
      dst[i] += 2;
    src++;
    nbytes -= 8;
  }
}

__attribute__((target("bmi2")))
void
decode8(uint64_t *src, uint8_t *dst[8], size_t nbytes)
{
  uint64_t mask = 0x0003000300030003;
  int i;

  while (nbytes > 0) {
    for (i = 0; i < 8; i++)
      *dst[i] = _pext_u64(*src, mask << (2 * i));

    for (i = 0; i < 8; i++)
      dst[i]++;
    src++;
    nbytes -= 8;
  }
}

__attribute__((target("bmi2")))
void
decode16(uint64_t *src, uint8_t *dst[16], size_t nbytes)
{
  uint64_t mask = 0x0000000300000003;
  int i;

  while (nbytes > 0) {
    for (i = 0; i < 16; i++) {
      *dst[i] = _pext_u64(src[0], mask << (2 * i)) |
	_pext_u64(src[1], mask << (2 * i)) << 4;
    }

    for (i = 0; i < 16; i++)
      dst[i]++;
    src += 2;
    nbytes -= 16;
  }
}

#else

Channel_extractor_bmi2::Channel_extractor_bmi2() :
  fallback_(nullptr)
{
}

void
Channel_extractor_bmi2::initialise(const std::vector<std::vector<int>> &track_positions_,
				   int size_of_one_input_word_,
				   int input_sample_size_,
				   int bits_per_sample_)
{
  fallback_ = new Channel_extractor_dynamic();
  fallback_->initialise(track_positions_, size_of_one_input_word_,
			input_sample_size_, bits_per_sample_);
}

void
Channel_extractor_bmi2::extract(unsigned char *input, unsigned char **output)
{
  fallback_->extract(input, output);
}

#endif
