/******      FFT      ******/
#ifndef FFT_H_
#define FFT_H_

#include "main.h"						// Global includes and definitions
#include <arduinoFFT.h>					// FFT

#define N_FFT				512

extern double fft_real[N_FFT], fft_imag[N_FFT];
extern bool windowFFT;

void computeFFT(bool window);
void performFFT(unsigned int buf_id, bool apply_EMA = true);

#endif
