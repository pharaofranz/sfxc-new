#ifndef COHERENT_DEDISPERSION_H
#define COHERENT_DEDISPERSION_H
#include "utils.h"
#include "control_parameters.h"
#include "correlator_node_types.h"
#ifdef USE_DOUBLE
#include "sfxc_fft.h"
#else
#include "sfxc_fft_float.h"
#endif

class Coherent_dedispersion {
public:
  typedef Correlator_node_types::Delay_memory_pool   Delay_memory_pool;
  typedef Correlator_node_types::Delay_queue         Delay_queue;
  typedef Correlator_node_types::Delay_queue_ptr     Delay_queue_ptr;
  typedef Delay_queue::value_type                    Delay_queue_element;

  typedef Memory_pool_vector_element<std::complex<FLOAT> > Complex_vector;
  typedef Memory_pool_vector_element<FLOAT>                Real_vector;
  typedef boost::shared_ptr<Complex_vector>                Complex_vector_ptr;
  typedef boost::shared_ptr<Real_vector>                   Real_vector_ptr;

  typedef Pulsar_parameters::Pulsar                  Pulsar;

  Coherent_dedispersion(int stream_nr_);
  ~Coherent_dedispersion();
  void do_task();
  bool has_work();
  void empty_output_queue();
  void set_parameters(const Correlation_parameters &correlation_parameters,
                      Complex_vector_ptr filter_ptr_, 
                      Complex_vector_ptr dedispersion_buffer_ptr_,
                      Real_vector_ptr zeropad_buffer_ptr_,
                      boost::shared_ptr<SFXC_FFT>  fft);
  void connect_to(Delay_queue_ptr buffer);
  /// Get the output
  Delay_queue_ptr get_output_buffer();
private:
  void allocate_element(int nfft);
  void overlap_add();
private:
  int stream_nr;
  int out_pos;
  int current_fft, current_buffer;
  int fft_size_dedispersion, fft_size_correlation;
  int total_input_fft; // FIXME debug info
  int n_fft_dedispersion;
  Time current_time, start_time, stop_time;
  Complex_vector_ptr filter_ptr, dedispersion_buffer_ptr; 
  Real_vector_ptr zeropad_buffer_ptr;
  Real_vector time_buffer[2]; 
  Delay_queue_element cur_output;

  Delay_memory_pool output_memory_pool;
  Delay_queue_ptr input_queue;
  Delay_queue_ptr output_queue;
  boost::shared_ptr<SFXC_FFT>  fft_ptr;
};
#endif
