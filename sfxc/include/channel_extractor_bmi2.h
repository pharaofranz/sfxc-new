#include <map>

#include "channel_extractor_interface.h"

class Channel_extractor_bmi2 : public Channel_extractor_interface {
 public:
  Channel_extractor_bmi2();

  void initialise(const std::vector<std::vector<int>> &track_positions_,
		  int size_of_one_input_word_,
		  int input_sample_size_, int bits_per_sample_);

  void extract(unsigned char *input, unsigned char **output);

 private:
  Channel_extractor_interface* fallback_;
  std::map<size_t, size_t> channel_map;
  size_t nbytes;
};
