#ifndef CIRCUITMESS_AUDIO_AUDIOSYSTEM_H
#define CIRCUITMESS_AUDIO_AUDIOSYSTEM_H

#include "Chirp.hpp"
#include <Util/Task.h>
#include <Sync/Mutex.h>

/**
 * Simple Audio system designed for short SFX played on a simple piezo buzzer.
 */
class AudioSystem {
public:
	AudioSystem();

	/**
	 * Plays the specified sound, interrupts the currently playing sound.
	 */
	void play(std::initializer_list<Chirp> sound);
	void play(const Sound& sound);

	void stop();

private:
	Task task;
	Sound queued;
	Sound current;

	Mutex mut;

	void playbackFunc();
	volatile uint32_t startMillis = 0;
	volatile uint32_t currentMillis = 0;
	volatile uint32_t chirpID = 0;
};

extern AudioSystem Audio;

#endif //CIRCUITMESS_AUDIO_AUDIOSYSTEM_H
