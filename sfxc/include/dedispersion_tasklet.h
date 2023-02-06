#ifndef DEDISPERSION_TASKLET_H
#define DEDISPERSION_TASKLET_H
#include "utils.h"
#include "control_parameters.h"
#include "correlator_node_types.h"
#include "coherent_dedispersion.h"
#ifdef USE_DOUBLE
#include "sfxc_fft.h"
#else
#include "sfxc_fft_float.h"
#endif

class Dedispersion_tasklet {
public:
  typedef Correlator_node_types::Delay_queue_ptr     Delay_queue_ptr;
  typedef Pulsar_parameters::Pulsar                  Pulsar;
  typedef boost::shared_ptr<Coherent_dedispersion>   Coherent_dedispersion_ptr;

  typedef Memory_pool_vector_element<std::complex<FLOAT> > Complex_vector;
  typedef Memory_pool_vector_element<FLOAT >               Real_vector;
  typedef boost::shared_ptr<Complex_vector>                Complex_vector_ptr;
  typedef boost::shared_ptr<Real_vector>                   Real_vector_ptr;

  class Dedispersion_parameters {
    public:
      typedef Dedispersion_tasklet::Complex_vector     Complex_vector;
      typedef Dedispersion_tasklet::Complex_vector_ptr Complex_vector_ptr;
      typedef Dedispersion_tasklet::Real_vector        Real_vector;
      typedef Dedispersion_tasklet::Real_vector_ptr    Real_vector_ptr;

      Dedispersion_parameters();
      void set_parameters(const Correlation_parameters &parameters, Pulsar &pulsar, int stream_nr);
    public:
      int64_t channel_freq; // In Hz
      int channel_bw; // In Hz
      double dedispersion_freq;
      int dedispersion_pos;
      Complex_vector_ptr filter_ptr;
      Complex_vector_ptr dedispersion_buffer_ptr;
      Real_vector_ptr zeropad_buffer_ptr;
      boost::shared_ptr<SFXC_FFT> fft_ptr;
    private:
      void create_dedispersion_filter();            
      double sample_rate;
      Time integer_channel_offset;
      double channel_offset, DM;
      int sideband;
      int current_fft, current_buffer;
      int fft_size_dedispersion;
  };
  Dedispersion_tasklet();
  ~Dedispersion_tasklet();
  bool do_task();
  void set_parameters(const Correlation_parameters &parameters, Pulsar &pulsar);
  void empty_output_queue();
  void connect_to(Delay_queue_ptr buffer, int stream_nr);
  /// Get the output
  Delay_queue_ptr get_output_buffer(int stream_nr);
private:
  // We share buffers and filters between dedispersion_modules
  std::vector<Dedispersion_parameters> dedispersion_parameters;
  std::vector<Coherent_dedispersion_ptr>  dedispersion_modules;
};
#endif
