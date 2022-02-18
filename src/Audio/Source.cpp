#include "Source.h"

Source::Source(DataSource& ds) : ds(ds){}

int Source::getBytesPerSample(){
	return bytesPerSample;
}

int Source::getSampleRate(){
	return sampleRate;
}

int Source::getChannels(){
	return channels;
}
