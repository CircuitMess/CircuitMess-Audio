#ifndef CIRCUITMESS_AUDIO_CHIRP_HPP
#define CIRCUITMESS_AUDIO_CHIRP_HPP
#include <Arduino.h>
#include <vector>

/**
 * A chirp is a waveform that “sweeps” from a starting frequency to an ending frequency, during the specified duration of time.
 */
struct Chirp{
	uint16_t startFreq; //[Hz]
	uint16_t endFreq; //[Hz]
	uint32_t duration; //[ms]
};

typedef std::vector<Chirp> Sound;

#endif //CIRCUITMESS_AUDIO_CHIRP_HPP
