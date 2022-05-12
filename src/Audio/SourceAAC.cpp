#include "SourceAAC.h"
#include "../PerfMon.h"
#include "../Setup.hpp"

SourceAAC::SourceAAC(DataSource &ds) : Source(ds), fillBuffer(AAC_DECODE_MIN_INPUT), dataBuffer(AAC_OUT_BUFFER){
	channels = sampleRate = bitrate = movedBytes = 0;
	bytesPerSample = 2;

	dataBuffer.clear();
	fillBuffer.clear();

	dataSize = ds.available();
	bitrate = 64000;

	hAACDecoder = AACInitDecoder();
	if(hAACDecoder == nullptr){
		Serial.println("Decoder construct fail");

	}
}

SourceAAC::~SourceAAC(){
	SourceAAC::close();
}

void SourceAAC::setSongDoneCallback(void (*callback)()) {
	songDoneCallback = callback;
}

void SourceAAC::close(){

/*	readBuffer.clear();
	if(readJobPending){
		while(readResult == nullptr){
			delayMicroseconds(1);
		}

		free(readResult->buffer);
		delete readResult;
	}*/

	channels = sampleRate = bytesPerSample = bitrate = movedBytes = 0;
	dataBuffer.clear();
	fillBuffer.clear();

	if(hAACDecoder){
		AACFreeDecoder(hAACDecoder);
		hAACDecoder = nullptr;
	}
}

int SourceAAC::available(){
	if(channels == 0 || bytesPerSample == 0 ) return 0;
	return (ds.available() / (channels * bytesPerSample));
}

/*
void SourceAAC::addReadJob(bool full){
	if(readJobPending) return;

	delete readResult;
	readResult = nullptr;

	size_t size = full ? readBuffer.writeAvailable() : AAC_READ_CHUNK;

	//Serial.printf("Adding read job, size: %ld\n", size);

	if(size == 0 || readBuffer.writeAvailable() < size){
		return;
	}

	uint8_t* buf;
	if(size <= AAC_READ_CHUNK || !psramFound()){
		buf = static_cast<uint8_t*>(malloc(size));
	}else{
		buf = static_cast<uint8_t*>(ps_malloc(size));
	}

	Sched.addJob(new SchedJob{
						 .type = SchedJob::READ,
						 .file = file,
						 .size = size,
						 .buffer = buf,
						 .result = &readResult
				 });

	readJobPending = true;
}

void SourceAAC::processReadJob(){
	if(readResult == nullptr){
		if(readBuffer.readAvailable() + fillBuffer.readAvailable() < AAC_DECODE_MIN_INPUT){
			while(readResult == nullptr){
				vTaskDelay(1);
			}
		}else{
			return;
		}
	}

	readBuffer.write(readResult->buffer, readResult->size);
	free(readResult->buffer);

	delete readResult;
	readResult = nullptr;

	readJobPending = false;
}

*/

size_t SourceAAC::generate(int16_t* outBuffer){
	if(!hAACDecoder){
		Serial.println("Decoder false");
		return 0;
	}

	Profiler.start("AAC read job process");
	refill();
	Profiler.end();

	if(fillBuffer.readAvailable() < AAC_DECODE_MIN_INPUT){
		Serial.println("if fillbuffer < aac_decode_min");
//		if(songDoneCallback != nullptr) {
//			songDoneCallback();
//		}
		if(repeat){
			ds.seek(0);
			refill();
		}else{
			return 0;
		}
	}

	while(dataBuffer.readAvailable() < BUFFER_SIZE){
		// Serial.printf("Grabbing, available %ld, taking %ld\n", readBuffer.readAvailable(), fillBuffer.writeAvailable());

		if(fillBuffer.readAvailable() < AAC_DECODE_MIN_INPUT){
			refill();
		}

		auto data = const_cast<uint8_t*>(fillBuffer.readData());
		int bytesLeft = fillBuffer.readAvailable();
		// Serial.printf("Decoding, available %ld\n", fillBuffer.readAvailable());
		int ret = AACDecode(hAACDecoder, &data, &bytesLeft, reinterpret_cast<short*>(dataBuffer.writeData()));
		if(ret){
			size_t frameSize = fillBuffer.readAvailable() - bytesLeft;
			Serial.printf("decode error %d, frame size %d B\n", ret, frameSize);
			movedBytes++;
			fillBuffer.readMove(1);
			continue;
		}
		movedBytes += fillBuffer.readAvailable() - bytesLeft;
		fillBuffer.readMove(fillBuffer.readAvailable() - bytesLeft);

		AACFrameInfo fi;
		AACGetLastFrameInfo(hAACDecoder, &fi);

		sampleRate = fi.sampRateOut;
		channels = fi.nChans;

		dataBuffer.writeMove(fi.outputSamps * channels * bytesPerSample);
	}
	Profiler.end();

	size_t size = min((size_t) BUFFER_SIZE, dataBuffer.readAvailable());
	memcpy(outBuffer, dataBuffer.readData(), size);
	dataBuffer.readMove(size);
	size_t samples = size / (NUM_CHANNELS * BYTES_PER_SAMPLE);

	for(int i = 0; i < samples; i++){
		outBuffer[i] *= volume;
	}

/*	if(samples == 0){
		ds.seek(0);

		if(songDoneCallback != nullptr) {
			songDoneCallback();
		}
		if(repeat){
			return generate(outBuffer);
		}
	}*/

	return samples;
}

void SourceAAC::refill(){
	size_t size = min(fillBuffer.writeAvailable(), ds.available());
	size = ds.read(fillBuffer.writeData(), size);
	fillBuffer.writeMove(size);
}

uint16_t SourceAAC::getDuration(){
	if(bitrate == 0) return 0;
	return 8 * dataSize / bitrate;
}

uint16_t SourceAAC::getElapsed(){
	if(bitrate == 0) return 0;
	return 8 * movedBytes / bitrate;
}

void SourceAAC::seek(uint16_t time, fs::SeekMode mode){
	size_t offset = time * bitrate / 8;
	if(offset >= dataSize){
		return;
	}
	ds.seek(offset);
	movedBytes = offset;
	resetDecoding();


	//prebaciti u File implementaciju sa SD bufferanjem u seek()
/*	if(readJobPending){
		while (readResult == nullptr){
			delayMicroseconds(1);
		}

		free(readResult->buffer);
		delete readResult;
		readResult = nullptr;
		readJobPending = false;
	}
	Sched.addJob(new SchedJob{
			.type = SchedJob::SEEK,
			.file = file,
			.size = offset,
			.buffer = nullptr,
			.result = nullptr
	});*/
}


void SourceAAC::resetDecoding() {
	dataBuffer.clear();
	fillBuffer.clear();
	AACFlushCodec(hAACDecoder);
}
