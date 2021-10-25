#include "OutputDAC.h"
#include "../Setup.hpp"
#include "../PerfMon.h"

#define TIMER DAC_TIMER

typedef struct {
	union {
		struct {
			uint32_t reserved0:   10;
			uint32_t alarm_en:     1;             /*When set  alarm is enabled*/
			uint32_t level_int_en: 1;             /*When set  level type interrupt will be generated during alarm*/
			uint32_t edge_int_en:  1;             /*When set  edge type interrupt will be generated during alarm*/
			uint32_t divider:     16;             /*Timer clock (T0/1_clk) pre-scale value.*/
			uint32_t autoreload:   1;             /*When set  timer 0/1 auto-reload at alarming is enabled*/
			uint32_t increase:     1;             /*When set  timer 0/1 time-base counter increment. When cleared timer 0 time-base counter decrement.*/
			uint32_t enable:       1;             /*When set  timer 0/1 time-base counter is enabled*/
		};
		uint32_t val;
	} config;
	uint32_t cnt_low;                             /*Register to store timer 0/1 time-base counter current value lower 32 bits.*/
	uint32_t cnt_high;                            /*Register to store timer 0 time-base counter current value higher 32 bits.*/
	uint32_t update;                              /*Write any value will trigger a timer 0 time-base counter value update (timer 0 current value will be stored in registers above)*/
	uint32_t alarm_low;                           /*Timer 0 time-base counter value lower 32 bits that will trigger the alarm*/
	uint32_t alarm_high;                          /*Timer 0 time-base counter value higher 32 bits that will trigger the alarm*/
	uint32_t load_low;                            /*Lower 32 bits of the value that will load into timer 0 time-base counter*/
	uint32_t load_high;                           /*higher 32 bits of the value that will load into timer 0 time-base counter*/
	uint32_t reload;                              /*Write any value will trigger timer 0 time-base counter reload*/
} hw_timer_reg_t;

typedef struct hw_timer_s {
	hw_timer_reg_t * dev;
	uint8_t num;
	uint8_t group;
	uint8_t timer;
	portMUX_TYPE lock;
} hw_timer_t;

typedef struct hw_timer_s hw_timer_t;

namespace CMA {
	void IRAM_ATTR timerStop(hw_timer_t *timer){
		timer->dev->config.enable = 0;
	}

	void IRAM_ATTR timerWrite(hw_timer_t *timer, uint64_t val){
		timer->dev->load_high = (uint32_t) (val >> 32);
		timer->dev->load_low = (uint32_t) (val);
		timer->dev->reload = 1;
	}

	void IRAM_ATTR timerAlarmWrite(hw_timer_t *timer, uint64_t alarm_value, bool autoreload){
		timer->dev->alarm_high = (uint32_t) (alarm_value >> 32);
		timer->dev->alarm_low = (uint32_t) alarm_value;
		timer->dev->config.autoreload = autoreload;
	}

	void IRAM_ATTR timerAlarmDisable(hw_timer_t *timer){
		timer->dev->config.alarm_en = 0;
	}

	uint64_t IRAM_ATTR timerRead(hw_timer_t *timer){
		timer->dev->update = 1;
		while(timer->dev->update) delayMicroseconds(1);
		uint64_t h = timer->dev->cnt_high;
		uint64_t l = timer->dev->cnt_low;
		return (h << 32) | l;
	}
}

#define dis(timer) do { CMA::timerAlarmDisable(timer); CMA::timerStop(timer); CMA::timerWrite(timer, 0); } while(0)

OutputDAC* output = nullptr;

OutputDAC::OutputDAC(int pin_out, int pin_sd) : Output(true), PIN_OUT(pin_out), PIN_SD(pin_sd){
	samples = static_cast<int16_t*>(malloc(BUFFER_SIZE));

	if(timer != nullptr){
		dis(timer);
		timerEnd(timer);
		timer = nullptr;
	}

	timer = timerBegin(TIMER, 80, true);
	timerAttachInterrupt(timer, &timerInterrupt, true);    // P3= edge triggered
	dis(timer);
}

OutputDAC::~OutputDAC(){
	dis(timer);
	timerEnd(timer);
	free(samples);
	::output = nullptr;
}

// #include "soc/sens_reg.h"
void IRAM_ATTR OutputDAC::timerInterrupt(){
	OutputDAC* output = ::output;
	if(output == nullptr) return;
	if(output->currentSample >= BUFFER_SAMPLES || output->currentSample >= output->numSamples) return;

	double value = (double) output->samples[output->currentSample]; // [-1, 1]
	//value /= 3.0; // counter voltage amplification
	//value /= 3.0; // make sure we don't draw too much current
	value *= (double) output->getGain(); // apply volume [-1, 1]
	value = value / 257.0 + 127; // bring to [0, 1]
	value = value > 255 ? 255 : value; // clipping
	value = value < 0 ? 0 : value; // clipping

	//dacWrite(output->PIN_OUT, (uint8_t) value);
	// For pin 26
	CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
	CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
	SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, ((uint8_t) value), RTC_IO_PDAC1_DAC_S);
	SET_PERI_REG_MASK(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC | RTC_IO_PDAC1_DAC_XPD_FORCE);

	output->currentSample++;
}

void OutputDAC::output(size_t numSamples){
	while(this->currentSample < this->numSamples){
		delayMicroseconds(1);
	}

	currentSample = BUFFER_SAMPLES;
	memcpy(samples, inBuffer, numSamples * BYTES_PER_SAMPLE * NUM_CHANNELS);
	this->numSamples = numSamples;
	currentSample = 0;
}

void OutputDAC::init(){
	if(PIN_SD != -1){
		digitalWrite(PIN_SD, LOW);
	}

	dacWrite(PIN_OUT, 127);

	::output = this;
	currentSample = BUFFER_SAMPLES;
	numSamples = 0;
	dis(timer);
	timerAlarmWrite(timer, 1000000 / SAMPLE_RATE, true);
	timerAlarmEnable(timer);
	timerStart(timer);
}

void OutputDAC::deinit(){
	dis(timer);
	// ::output = nullptr;

	if(PIN_SD != -1){
		digitalWrite(PIN_SD, HIGH);
	}

	dacWrite(PIN_OUT, 127);

	numSamples = 0;
	currentSample = BUFFER_SAMPLES;
}