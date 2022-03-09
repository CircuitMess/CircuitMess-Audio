#ifndef JAYD_LIBRARY_SCHEDULER_H
#define JAYD_LIBRARY_SCHEDULER_H

#include <FS.h>
#include <vector>
#include <Loop/LoopListener.h>
#include <Sync/Queue.h>

struct SchedResult {
	uint8_t error;
	size_t size;
	uint8_t* buffer;
};

struct SchedJob {
	enum { WRITE, READ, SEEK } type;
	fs::File file;
	size_t size;
	uint8_t* buffer;
	SchedResult** result;
};

class Scheduler : public LoopListener {
public:
	Scheduler();

	void addJob(SchedJob *job);
	void loop(uint micros) override;

	virtual void beforeJob() = 0;
	virtual void afterJob() = 0;
private:
	Queue jobs;

	void doJob(SchedJob* job);
};

#endif //JAYD_LIBRARY_SCHEDULER_H
