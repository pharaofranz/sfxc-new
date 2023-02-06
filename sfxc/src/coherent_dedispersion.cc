#include "coherent_dedispersion.h"

Coherent_dedispersion::Coherent_dedispersion(int stream_nr_):
      output_queue(Delay_queue_ptr(new Delay_queue())),
      stream_nr(stream_nr_), output_memory_pool(4, NO_RESIZE), 
      current_buffer(0), n_fft_dedispersion(0) {
}

Coherent_dedispersion::~Coherent_dedispersion(){
  // Clear variable in order not to confuse the reference counting
  cur_output = Delay_queue_element();
  while(output_queue->size() > 0)
    output_queue->pop();
}

void
Coherent_dedispersion::do_task() {
  // Check if dedispersion job has already finished
  if(current_time >= stop_time)
    return;

  Delay_queue_element input = input_queue->front_and_pop();
  Memory_pool_vector_element<FLOAT> &input_data = input->data;

  // Shared data structures
  Complex_vector &filter = *filter_ptr; 
  Complex_vector &dedispersion_buffer = *dedispersion_buffer_ptr;
  Real_vector &zeropad_buffer = *zeropad_buffer_ptr;
  SFXC_FFT &fft = *fft_ptr;

  const int n_input_fft = input_data.size() / fft_size_dedispersion;
  total_input_fft += n_input_fft;

  // Allocate output buffer
  allocate_element(n_input_fft*fft_size_dedispersion/fft_size_correlation);

  for(int i=0; i<n_input_fft; i++){
    // Apply dedispersion
    memcpy(&zeropad_buffer[0], 
           &input_data[i*fft_size_dedispersion], 
           fft_size_dedispersion*sizeof(FLOAT));
    fft.rfft(&zeropad_buffer[0], &dedispersion_buffer[0]);
    SFXC_MUL_FC(&dedispersion_buffer[0], &filter[0], 
                &dedispersion_buffer[0], fft_size_dedispersion + 1);
    fft.irfft(&dedispersion_buffer[0], &time_buffer[current_buffer][0]);

    // Perform overlap-add
    overlap_add();
    current_buffer = 1 - current_buffer;
    current_time.inc_samples(fft_size_dedispersion);
    current_fft += 1;
  }
  // Write output data
  if(out_pos > 0){
    cur_output->data.resize(out_pos);
    output_queue->push(cur_output);
    // Push last fft
    if (current_fft == n_fft_dedispersion) {
      memset(&time_buffer[current_buffer][0], 0, time_buffer[current_buffer].size()*sizeof(FLOAT));
      allocate_element(n_input_fft*fft_size_dedispersion/fft_size_correlation);
      overlap_add();
      if (out_pos > 0) {
        cur_output->data.resize(out_pos);
        output_queue->push(cur_output);
      }
    }
  }
}

void
Coherent_dedispersion::empty_output_queue(){
  while (output_queue->size() > 0)
    output_queue->pop();
}

void
Coherent_dedispersion::overlap_add() {
  Memory_pool_vector_element<FLOAT> &data = cur_output->data;
  //NB : fft_size_dedispersion >= fft_size_correlation
  const int step_size = fft_size_correlation / 2;
  int nstep=fft_size_dedispersion/step_size;
  // First time only fft_size_dedispersion/2 of data is send
  if ((current_fft==0) || (current_fft == n_fft_dedispersion))
    nstep /= 2;

  for(int n=0; (n < nstep); n++){
    int i, j;
    if(n<nstep/2){
      i = 3*fft_size_dedispersion/2 + n*step_size;
      j = fft_size_dedispersion/2 + n*step_size;
    }else{
      i = (n-nstep/2)*step_size;
      j = fft_size_dedispersion + (n-nstep/2) *step_size;
    }
    // Sum overlapping windows
    SFXC_ADD_F(&time_buffer[current_buffer][i],
               &time_buffer[1-current_buffer][j],
               &data[out_pos],
               step_size);
    out_pos += step_size;
  }
}

void
Coherent_dedispersion::allocate_element(int nfft){
  cur_output = output_memory_pool.allocate();
  cur_output->data.resize(fft_size_correlation*nfft);
  out_pos = 0;
}

bool
Coherent_dedispersion::has_work(){
  if (input_queue->empty())
    return false;
  if (output_memory_pool.number_free_element() < 2)
    return false;
  return true;
}

void Coherent_dedispersion::connect_to(Delay_queue_ptr buffer) {
  input_queue = buffer;
}

Coherent_dedispersion::Delay_queue_ptr
Coherent_dedispersion::get_output_buffer() {
  SFXC_ASSERT(output_queue != Delay_queue_ptr());
  return output_queue;
}

void 
Coherent_dedispersion::set_parameters(const Correlation_parameters &parameters,
                                      Complex_vector_ptr filter_ptr_, 
                                      Complex_vector_ptr dedispersion_buffer_ptr_,
                                      Real_vector_ptr zeropad_buffer_ptr_,
                                      boost::shared_ptr<SFXC_FFT> fft_ptr_)
{
  total_input_fft = 0;
  empty_output_queue();
  int stream_idx = 0;
  while ((stream_idx < parameters.station_streams.size()) &&
         (parameters.station_streams[stream_idx].station_stream != stream_nr))
    stream_idx++;
  if (stream_idx == parameters.station_streams.size()) {
    // Data stream is not participating in current time slice
    return;
  }
  // We share the dedispersion filter and some buffers between dedispersion_modules
  // to reduce memory usage, which can become enormous at P band frequencies. 
  filter_ptr = filter_ptr_;
  dedispersion_buffer_ptr = dedispersion_buffer_ptr_;
  zeropad_buffer_ptr = zeropad_buffer_ptr_;
  fft_ptr = fft_ptr_;

  int64_t sample_rate = parameters.station_streams[stream_idx].sample_rate;
  int64_t base_sample_rate = parameters.sample_rate;
  int64_t bwratio = sample_rate / base_sample_rate;
  fft_size_dedispersion = parameters.fft_size_dedispersion * bwratio;
  fft_size_correlation = parameters.fft_size_correlation * bwratio;
  n_fft_dedispersion = parameters.slice_size / parameters.fft_size_dedispersion;
  start_time = parameters.integration_start;
  stop_time = parameters.integration_start + parameters.integration_time; 
  if(RANK_OF_NODE == -17) {
    std::cout.precision(16);
    std::cout << RANK_OF_NODE << " : start_time(" << stream_nr << ") = " << start_time 
              << ", " << start_time.get_time_usec() << "\n";
    std::cout << RANK_OF_NODE << " : stop_time(" << stream_nr << ") = " << stop_time 
              << ", " << stop_time.get_time_usec() << "\n";
    std::cout << RANK_OF_NODE << " : sample_rate(" << stream_nr << ") = " << sample_rate
              << ", base_sample_rate = " << base_sample_rate << "\n";
    std::cout << RANK_OF_NODE << " : fft_size_dedispersion(" << stream_nr << ") = " << fft_size_dedispersion
              << ", fft_size_correlation = " << fft_size_correlation << ", n_fft_dedispersion = " << n_fft_dedispersion << "\n";
  }
  current_time = parameters.stream_start;
  current_time.set_sample_rate(sample_rate);
  current_time.inc_samples(-fft_size_dedispersion /2);
  
  current_fft = 0;

  // Initialize buffers
  for (int i=0; i<2; i++) {
    time_buffer[i].resize(2 * fft_size_dedispersion);
    memset(&time_buffer[i][0], 0, time_buffer[i].size()*sizeof(FLOAT));
  }
}
