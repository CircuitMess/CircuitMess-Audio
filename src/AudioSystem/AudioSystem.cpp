#include "AudioSystem.h"
#include <Audio/Piezo.h>

AudioSystem::AudioSystem() : task("audioSystem", [](Task* task){ static_cast<AudioSystem*>(task->arg)->playbackFunc(); }, 2048, this){

}

void AudioSystem::play(std::initializer_list<Chirp> sound){
	if(!task.running){
		current = sound;
		startMillis = millis();
		task.start(1, 0);
	}else{
		queued = sound;
	}
}

void AudioSystem::play(const Sound& sound){
	if(!task.running){
		current = sound;
		startMillis = millis();
		task.start(1, 0);
	}else{
		queued = sound;
	}
}

void AudioSystem::stop(){
	task.stop();
	queued.clear();
	Piezo.noTone();
}

void AudioSystem::playbackFunc(){
	while(chirpID < current.size()){
		if(!queued.empty()){
			current = queued;
			queued.clear();
			chirpID = 0;
			startMillis = millis();
			continue;
		}

		currentMillis = millis() - startMillis;

		uint16_t freq = map(currentMillis, 0, current[chirpID].duration, current[chirpID].startFreq, current[chirpID].endFreq);
		Piezo.tone(freq, 0); //duration of 0 equates to a continuous sound

		vTaskDelay(pdMS_TO_TICKS(3 + (int)(1000.0 / (double)freq)));

		if(current[chirpID].duration > currentMillis) continue;

		startMillis = millis();
		chirpID++;

		if(chirpID >= current.size()){
			current = queued;
			queued.clear();
			chirpID = 0;
		}
	}
	Piezo.noTone();
}

AudioSystem Audio;