#include "dedispersion_tasklet.h"

Dedispersion_tasklet::Dedispersion_tasklet() {
}

Dedispersion_tasklet::~Dedispersion_tasklet(){
}

bool
Dedispersion_tasklet::do_task(){
  bool done_work = false;
  for (size_t i=0; i<dedispersion_modules.size(); i++) {
    if (dedispersion_modules[i] != Coherent_dedispersion_ptr()) {
      if (dedispersion_modules[i]->has_work()) {
        dedispersion_modules[i]->do_task();
        done_work = true;
      }
    }
  }
  return done_work;
}

void
Dedispersion_tasklet::empty_output_queue(){
  for (size_t i=0; i<dedispersion_modules.size(); i++) {
    if (dedispersion_modules[i] != Coherent_dedispersion_ptr()) {
      dedispersion_modules[i]->empty_output_queue();
    }
  }
}

Dedispersion_tasklet::Delay_queue_ptr
Dedispersion_tasklet::get_output_buffer(int stream_nr) {
  return dedispersion_modules[stream_nr]->get_output_buffer();
}

void Dedispersion_tasklet::connect_to(Delay_queue_ptr buffer, int stream_nr) {
  if (dedispersion_modules.size() <= stream_nr)
    dedispersion_modules.resize(stream_nr+1);
  dedispersion_modules[stream_nr] = Coherent_dedispersion_ptr(new Coherent_dedispersion(stream_nr)); 
  dedispersion_modules[stream_nr]->connect_to(buffer);
}

void 
Dedispersion_tasklet::set_parameters(const Correlation_parameters &parameters, Pulsar &pulsar) {
  dedispersion_parameters.resize(0);
  // We don't want expensive reallocation of vector
  dedispersion_parameters.reserve(parameters.station_streams.size()); 

  for (size_t stream_nr  = 0; stream_nr < parameters.station_streams.size(); stream_nr++) {
    int station_stream = parameters.station_streams[stream_nr].station_stream; 
    size_t paridx = 0;
    int64_t channel_freq = parameters.station_streams[stream_nr].channel_freq; 
    int channel_bw = parameters.station_streams[stream_nr].bandwidth;
    if (parameters.station_streams[stream_nr].sideband != parameters.sideband) {
      int sb = (parameters.station_streams[stream_nr].sideband == 'L') ? -1 : 1;
      channel_freq += sb * channel_bw;
    }
    // See if we already have a filter for the channel_freq, channel_bw combination
    while (paridx < dedispersion_parameters.size()) {
      if ((dedispersion_parameters[paridx].channel_freq == channel_freq) &&
          (dedispersion_parameters[paridx].channel_bw == channel_bw))
        break;
      paridx++;
    }
    if (paridx == dedispersion_parameters.size()) {
      // Create new filter
      dedispersion_parameters.resize(dedispersion_parameters.size() + 1);
      dedispersion_parameters[paridx].set_parameters(parameters, pulsar, stream_nr);
    }
    dedispersion_modules[station_stream]->
      set_parameters(parameters,
                     dedispersion_parameters[paridx].filter_ptr, 
                     dedispersion_parameters[paridx].dedispersion_buffer_ptr,
                     dedispersion_parameters[paridx].zeropad_buffer_ptr,
                     dedispersion_parameters[paridx].fft_ptr);
  }
}

Dedispersion_tasklet::Dedispersion_parameters::
Dedispersion_parameters() :
    filter_ptr(new Complex_vector()), 
    dedispersion_buffer_ptr(new Complex_vector()),
    zeropad_buffer_ptr(new Real_vector()),
    fft_ptr(new SFXC_FFT()) {
}

void 
Dedispersion_tasklet::Dedispersion_parameters::
set_parameters(const Correlation_parameters &parameters, Pulsar &pulsar, int stream_nr) {
  DM = pulsar.polyco_params[0].DM;
  int32_t base_sample_rate = parameters.sample_rate;
  sample_rate = parameters.station_streams[stream_nr].sample_rate;
  int32_t sample_rate_ratio = sample_rate / base_sample_rate;
  fft_size_dedispersion = parameters.fft_size_dedispersion * sample_rate_ratio;

  sideband = (parameters.sideband == 'L') ? -1 : 1;
  channel_bw = parameters.station_streams[stream_nr].bandwidth; // In Hz
  if (parameters.sideband == parameters.station_streams[stream_nr].sideband) {
    channel_freq = parameters.station_streams[stream_nr].channel_freq; // In Hz
  } else {
    // if sideband doesn't match setup statup, the band is flipped
    channel_freq = parameters.station_streams[stream_nr].channel_freq -
                   sideband * channel_bw; 
  }
  dedispersion_freq = parameters.channel_freq + sideband * parameters.bandwidth / 2;
  dedispersion_pos = round( sideband * fft_size_dedispersion * (dedispersion_freq - channel_freq) / 
                            channel_bw);
  std::cerr << RANK_OF_NODE << " : stream_nr = " << stream_nr 
            << ", dedispfreq = " << (int64_t)dedispersion_freq 
            << ", dedisppos = " << (int64_t)dedispersion_pos 
            << ", old_ch_freq = " << parameters.station_streams[stream_nr].channel_freq
            << ", ch_freq = " << channel_freq
            << ", ch_bw = " << channel_bw
            << ", ch_sideband = " << parameters.station_streams[stream_nr].sideband
            << ", freq = " << parameters.channel_freq
            << ", bw = " << parameters.bandwidth
            << ", sideband = " << parameters.sideband
            << "\n";
  channel_offset = parameters.channel_offset;
  int n = round(channel_offset * (base_sample_rate/1000000));
  integer_channel_offset = Time(0);
  integer_channel_offset.set_sample_rate(base_sample_rate);
  integer_channel_offset.inc_samples(n);
  create_dedispersion_filter();
  // Initialize the FFT's
  fft_ptr->resize(2 * fft_size_dedispersion); 
  // Create_buffers
  dedispersion_buffer_ptr->resize(fft_size_dedispersion +1);
  zeropad_buffer_ptr->resize(2 * fft_size_dedispersion);
  // zero padding
  Real_vector &zeropad_buffer = *zeropad_buffer_ptr;
  SFXC_ZERO_F(&zeropad_buffer[fft_size_dedispersion], fft_size_dedispersion); 
}

void
Dedispersion_tasklet::Dedispersion_parameters::
create_dedispersion_filter() {
  Complex_vector &filter = *filter_ptr;
  filter.resize(fft_size_dedispersion+1);
  // Round channel offset to the nearest sample
  double fract_offset = channel_offset - 
                        integer_channel_offset.get_time_usec();
  double dnu = double(channel_bw) / fft_size_dedispersion / 1e6;
  //double f0= (channel_freq + sideband*double(channel_bw)/2) / 1e6;
  double f0= dedispersion_freq / 1e6;
  double arg0 = 0;//channel_offset * channel_freq;
  for(int i=0; i<fft_size_dedispersion+1; i++){
    //double f = sideband*dnu*(i-fft_size_dedispersion/2);
    double f = sideband*dnu*(i-dedispersion_pos);
    double arg1=sideband*f*f*DM / (2.41e-10*f0*f0*(f+f0)); // For f, f0 in [MHz] 
    double arg2 = 0;// fract_offset*dnu*i; FIXME restore
    double arg = arg0+arg1+arg2;
    arg = -2*M_PI*(arg - floor(arg)); 
    filter[i] = std::complex<FLOAT>(cos(arg)/fft_size_dedispersion, 
                                    sin(arg)/fft_size_dedispersion);
    //filter[i] = std::complex<FLOAT>(cos(arg), 
    //                                sin(arg));
    std::cerr.precision(8);
    if(RANK_OF_NODE == -18)
      std::cerr << "filter["<<i<<"]="<<filter[i] 
                << ", f= " << f << ", f0=" << f0
                << ", fract_offset = " << fract_offset
                << ", dnu = "<< dnu<<", DM="<< DM<< "\n";
  }
}
