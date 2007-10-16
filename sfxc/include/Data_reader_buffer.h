/* Copyright (c) 2007 Joint Institute for VLBI in Europe (Netherlands)
 * All rights reserved.
 * 
 * Author(s): Nico Kruithof <Kruithof@JIVE.nl>, 2007
 * 
 * $Id$
 *
 */

#ifndef DATA_READER_BUFFER_H
#define DATA_READER_BUFFER_H

#include <Data_reader.h>
#include <Buffer.h>
#include <boost/shared_ptr.hpp>

//

/** Specialisation of Data_reader for reading from a buffer.
 **/
class Data_reader_buffer : public Data_reader {
  typedef Buffer_element<char,131072>      value_type;
  typedef Buffer<value_type>               Buffer;
  
public:
  /** Constructor, reads from buffer
   **/
  Data_reader_buffer(boost::shared_ptr<Buffer> buff);

  ~Data_reader_buffer();

  bool eof();  
  
private:
  size_t do_get_bytes(size_t nBytes, char *out);

  // The input buffer
  boost::shared_ptr<Buffer> buffer;
  // Number of bytes left in the current buffer-element
  int          bytes_left;
  char         *data_start;
  // Is there more data arriving in the buffer:
  bool         end_of_file;
};

#endif // DATA_READER_BUFFER_H
