#include "Oscillator.h"
#include "../Setup.hpp"

Oscillator::Oscillator(){
	waveBuffer = static_cast<int16_t*>(malloc(BUFFER_SIZE));
}

Oscillator::~Oscillator(){
	free(waveBuffer);

	for(const auto wave : pending) delete wave;
	for(const auto wave : waves) delete wave;
}

void Oscillator::tone(uint16_t freq, uint16_t duration, Wave::Type type){
	pendingMutex.lock();
	pending.insert(new Wave(freq, duration, type));
	pendingMutex.unlock();
}

void Oscillator::noTone(){
	end = true;
}

int Oscillator::available(){
	int samples = 0;

	for(const auto wave : waves){
		samples = max(samples, wave->available());
	}

	if(!pending.empty()){
		pendingMutex.lock();
		for(const auto wave : pending){
			samples = max(samples, wave->available());
		}
		pendingMutex.unlock();
	}

	return samples;
}

size_t Oscillator::generate(int16_t* outBuffer){
	if(end){
		end = false;

		for(const auto wave : waves){
			delete wave;
		}

		waves.clear();
		return 0;
	}

	if(!pending.empty()){
		pendingMutex.lock();

		for(const auto wave : pending){
			waves.insert(wave);
			printf("inserting wave\n");
		}

		pending.clear();
		pendingMutex.unlock();
	}

	size_t count = waves.size();
	if(count == 0) return 0;

	memset(outBuffer, 0, BUFFER_SIZE);
	size_t totalSamples = 0;

	std::unordered_set<Wave*> erased;
	for(const auto wave : waves){
		memset(waveBuffer, 0, min(BUFFER_SIZE, wave->available()));
		size_t samples = wave->generate(waveBuffer);

		for(int i = 0; i < samples; i++){
			outBuffer[i] += waveBuffer[i] / count;
		}

		totalSamples = max(totalSamples, samples);

		if(wave->available() == 0){
			erased.insert(wave);
		}
	}

	for(const auto wave : erased){
		printf("wave done\n");
		waves.erase(wave);
		delete wave;
	}

	return totalSamples;
}

