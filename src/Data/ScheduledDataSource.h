#ifndef CIRCUITMESS_AUDIO_SCHEDULEDDATASOURCE_H
#define CIRCUITMESS_AUDIO_SCHEDULEDDATASOURCE_H

#include "DataSource.h"
class Scheduler;

class ScheduledDataSource : public DataSource{
public:
	size_t read(uint8_t* buf, size_t size) override;
	size_t available() override;
	void seek(size_t pos, int mode) override;
	size_t size() override;
private:
	DataSource &ds;
	Scheduler &sched;
};


#endif //CIRCUITMESS_AUDIO_SCHEDULEDDATASOURCE_H
