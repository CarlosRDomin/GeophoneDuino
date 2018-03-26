/******      FFT      ******/
#include "FFT.h"

bool windowFFT = true;
double fft_real[N_FFT], fft_imag[N_FFT];
arduinoFFT FFT = arduinoFFT(fft_real, fft_imag, N_FFT, 10e3);	// Create FFT object

void computeFFT(bool window) {	// Apply the FFT to the input signal adc_values to obtain fft_magn, fft_real, fft_imag
	uint32_t t_start, t_end;
	t_start = micros();

	if (window) {
		FFT.Windowing(fft_real, N_FFT, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
	}
	FFT.Compute(FFT_FORWARD);	// Perform the time-domain -> freq-domain FFT
	FFT.ComplexToMagnitude();	// Convert RE + j*IM -> |F(w)| and save the result in fft_real
	t_end = micros();

	//consolePrintF("@t=%8d ms\t(deltaT=%6d us) -> FFT computed\n", millis(), t_end-t_start);
}

void performFFT(unsigned int buf_id, bool apply_EMA) {	// "FFT.loop()" function: computes the FFT on the geophone_buf[buf_id] and broadcasts the result through webSocketFFT
	const double F_s = 10e3, FILT_ALPHA = 0.1;	// FILT_ALPHA specifies the alpha component of the Exponential Moving Average (EMA) for the high-pass filt (removes DC component)
	static double mean_signal;					// Compute the average (EMA or arithmetic) to remove the DC component in the signal (centered at Vcc/2)
	static bool applied_EMA_last_iter = false;	// Helper flag to initialize mean_signal = geophone_buf[buf_id][0] if apply_EMA was false and now is true
	double curr_total_power = 0;
	uint32_t t_start, t_end;

	t_start = micros();	// Record current time so we can later estimate the sampling freq F_s

	if (!apply_EMA) {
		mean_signal = 0;	// Reset counter for arithmetic mean
	} else if (!applied_EMA_last_iter && apply_EMA) {	// Reset mean_signal if we weren't applying it before and now we do (otherwise it might take too long to converge to the real mean, since signal is centered around Vcc/2)
		mean_signal = geophone_buf[buf_id][0];	// Assume current ADC reading is close to the actual mean
	}

	for (uint16_t i=0; i<N_FFT; ++i) {
		if (apply_EMA) {
			mean_signal = FILT_ALPHA*geophone_buf[buf_id][i] + (1-FILT_ALPHA)*mean_signal;	// Update the moving average
			fft_real[i] = geophone_buf[buf_id][i] - mean_signal;	// High-pass filter (to remove DC component): signal-LowPassFilt = signal-mean_EMA
		} else {	// Compute the arithmetic mean
			fft_real[i] = geophone_buf[buf_id][i];
			mean_signal += fft_real[i]/N_FFT;	// Accumulate mean value of input signal (so we can later subtract it and we don't have a DC component)
		}
		fft_imag[i] = 0;
	}

	if (apply_EMA) {	// Update the EMA through the remainder of the signal points that didn't fit in the FFT
		for (uint16_t i=N_FFT; i<GEOPHONE_BUF_SIZE; ++i) {
			mean_signal = FILT_ALPHA*geophone_buf[buf_id][i] + (1-FILT_ALPHA)*mean_signal;
		}
	} else {	// If arithmetic mean, subtract the mean from the signal (so we don't have a DC component)
		for (uint16_t i=0; i<N_FFT; ++i) {
			fft_real[i] -= mean_signal;
		}
	}

	computeFFT(windowFFT);

	/*curr_total_power = fft_real[0];
	for (uint16_t i=1; i<=N_FFT/2; ++i) {
		curr_total_power += fft_real[i];
	}*/
	t_end = micros();
	//consolePrintF("@t=%8d ms\t(deltaT=%6d us) -> FFT fully processed (Total Power=%.2f; Window mean reading=%.2f)\n", millis(), t_end-t_start, curr_total_power, mean_signal);
}
