/* Copyright (c) 2007 Joint Institute for VLBI in Europe (Netherlands)
 * All rights reserved.
 *
 * Author(s): Nico Kruithof <Kruithof@JIVE.nl>, 2007
 *
 * $Id: channel_extractor.h 412 2007-12-05 12:13:20Z kruithof $
 *
 */

#ifndef DELAY_CORRECTION_H
#define DELAY_CORRECTION_H

#include <complex>
#include <fftw3.h>

#include "tasklet/tasklet.h"
#include "delay_table_akima.h"
#include "bits_to_float_converter.h"
#include "control_parameters.h"

#include "timer.h"

class Delay_correction : public Tasklet {
public:
  typedef Bits_to_float_converter::Output_buffer_element Input_buffer_element;
  typedef Bits_to_float_converter::Output_buffer         Input_buffer;
  typedef Bits_to_float_converter::Output_buffer_ptr     Input_buffer_ptr;

  typedef Input_buffer_element                           Output_buffer_element;
  typedef Input_buffer                                   Output_buffer;
  typedef Input_buffer_ptr                               Output_buffer_ptr;

  Delay_correction();
  virtual ~Delay_correction();

  /// Set the input
  void connect_to(Input_buffer_ptr new_input_buffer);

  /// Get the output
  Output_buffer_ptr get_output_buffer();

  /// Set the delay table
  void set_delay_table(const Delay_table_akima &delay_table);

  void set_parameters(const Correlation_parameters &parameters);

  /// Do one delay step
  void do_task();

  bool has_work();
  const char *name() {
    return __PRETTY_FUNCTION__;
  }

private:
  void fractional_bit_shift(int integer_shift,
                            FLOAT fractional_delay);
  void fringe_stopping(FLOAT output[]);

private:
  // access functions to the correlation parameters
  int number_channels();
  int sample_rate();
  int bandwidth();
  int length_of_one_fft(); // Length of one fft in microseconds
  int sideband();
  int64_t channel_freq();
  double get_delay(int64_t time);

private:
  Input_buffer_ptr    input_buffer;
  Output_buffer_ptr   output_buffer;

  int64_t                current_time; // In microseconds
  Correlation_parameters correlation_parameters;

  int n_ffts_per_integration, current_fft, total_ffts;

  FFTW_PLAN           plan_t2f, plan_f2t;

  std::complex<FLOAT> *data;

  // frequency scale for the fractional bit shift
  std::vector<FLOAT>  freq_scale;

  // For fringe stopping we do a linear approximation
  // maximal_phase_change is the maximal angle between two
  // sample points
  static const FLOAT maximal_phase_change; // 5.7 degrees
  int                n_recompute_delay;

  bool               delay_table_set;
  Delay_table_akima  delay_table;

  Timer              delay_timer;
};

#endif /*DELAY_CORRECTION_H*/
