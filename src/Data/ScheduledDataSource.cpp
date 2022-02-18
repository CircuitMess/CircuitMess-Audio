#include "ScheduledDataSource.h"

size_t ScheduledDataSource::read(uint8_t* buf, size_t size){
	return 0;
}

size_t ScheduledDataSource::available(){
	return 0;
}

void ScheduledDataSource::seek(size_t pos, int mode){

}

size_t ScheduledDataSource::size(){
	return ds.size();
}
