/*

  ESP32 FFT
  =========

  This provides a vanilla radix-2 FFT implementation and a test example.

  Author
  ------

  This code was written by [Robin Scheibler](http://www.robinscheibler.org) during rainy days in October 2017.

  License
  -------

  Copyright (c) 2017 Robin Scheibler

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/
#ifndef __FFT_H__
#define __FFT_H__

typedef enum
{
  FFT_REAL,
  FFT_COMPLEX
} fft_type_t;

typedef enum
{
  FFT_FORWARD,
  FFT_BACKWARD
} fft_direction_t;

#define FFT_OWN_INPUT_MEM 1
#define FFT_OWN_OUTPUT_MEM 2

typedef struct
{
  int size;  // FFT size
  float *input;  // pointer to input buffer
  float *output; // pointer to output buffer
  float *twiddle_factors;  // pointer to buffer holding twiddle factors
  fft_type_t type;   // real or complex
  fft_direction_t direction; // forward or backward
  unsigned int flags; // FFT flags
} fft_config_t;

fft_config_t *fft_init(int size, fft_type_t type, fft_direction_t direction, float *input, float *output);
void fft_destroy(fft_config_t *config);
void fft_execute(fft_config_t *config);
void fft(float *input, float *output, float *twiddle_factors, int n);
void ifft(float *input, float *output, float *twiddle_factors, int n);
void rfft(float *x, float *y, float *twiddle_factors, int n);
void irfft(float *x, float *y, float *twiddle_factors, int n);
void fft_primitive(float *x, float *y, int n, int stride, float *twiddle_factors, int tw_stride);
void split_radix_fft(float *x, float *y, int n, int stride, float *twiddle_factors, int tw_stride);
void ifft_primitive(float *input, float *output, int n, int stride, float *twiddle_factors, int tw_stride);
void fft8(float *input, int stride_in, float *output, int stride_out);
void fft4(float *input, int stride_in, float *output, int stride_out);

#endif // __FFT_H__
