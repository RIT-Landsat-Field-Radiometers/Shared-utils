/*
 * UIManager.h
 *
 *  Created on: Jul 29, 2021
 *      Author: reedt
 */

#ifndef SRC_SHARED_LEDMANAGER_H_
#define SRC_SHARED_LEDMANAGER_H_

#include <cstdint>
#include "main.h"
#include "cmsis_os.h"

#define RED_MASK 0x04
#define GREEN_MASK 0x02
#define BLUE_MASK 0x01
#define FAST_OFFSET 0x09
#define SLOW_OFFSET 0x06
#define ON_OFFSET 0x03
#define OFF_OFFSET 0x00

typedef struct
{
	GPIO_TypeDef * port;
	uint16_t pin;
} gpio_pin_t;

typedef enum : uint8_t
{
	NONE =  0b000,
	BLUE =  0b001,
	GREEN = 0b010,
	RED =	0b100,
	WHITE = 0b111,
	CYAN =  0b011,
	YELLOW = 0b110,
	MAGENTA = 0b101
} color;


class LEDManager
{
private:
	gpio_pin_t red;
	gpio_pin_t green;
	gpio_pin_t blue;
	bool timeoutEnabled = true;
	/* Definitions for LEDEvent */
	osEventFlagsId_t LEDEventHandle;
	const osEventFlagsAttr_t LEDEvent_attributes =
	{ .name = "LEDEvent" };
	/* Definitions for LEDTimeout */
	osTimerId_t LEDTimeoutHandle;
	const osTimerAttr_t LEDTimeout_attributes =
	{ .name = "LEDTimeout" };
	/* Definitions for LEDSlow */
	osTimerId_t LEDSlowHandle;
	const osTimerAttr_t LEDSlow_attributes =
	{ .name = "LEDSlow" };
	/* Definitions for LEDFast */
	osTimerId_t LEDFastHandle;
	const osTimerAttr_t LEDFast_attributes =
	{ .name = "LEDFast" };
	/* Definitions for UI */
	osThreadId_t UIHandle;
	osThreadAttr_t UI_attributes =
	{ .name = "UI", };

	typedef enum
	{
		OFF = 0, ON = 1, SLOW = 2, FAST = 3
	} LEDstate_t;

	LEDstate_t states[3] =
	{ OFF }; //R=2, G=1, B=0

	// These are internal functions that are task related
	static void main(void *);
	static void slowCallback(void *);
	static void fastCallback(void *);
	static void timeoutCallback(void *);

	static const int slowPeriod = 1000; // ticks (milliseconds)
	static const int fastPeriod = 500;
	static const int LEDTimeout = 10; // seconds (* 1000 ticks)
	static const uint8_t blue_pos = 0;
	static const uint8_t red_pos = 2;
	static const uint8_t green_pos = 1;

public:
	// Can be called from any thread (no ISR)
	LEDManager(gpio_pin_t red, gpio_pin_t green, gpio_pin_t blue);
	void start(bool timeoutEnabled = true);
	osThreadId_t getThread();
	osTimerId_t getFastTimer();
	osTimerId_t getSlowTimer();
	void turnOff(color c);
	void turnOn(color c);
	void slowFlash(color c);
	void fastFlash(color c);
	void startTimeout();
	void stopTimeout();
	virtual ~LEDManager();
	LEDManager(const LEDManager &other) = default;
	LEDManager(LEDManager &&other) = default;
	LEDManager& operator=(const LEDManager &other) = default;
	LEDManager& operator=(LEDManager &&other) = default;
};

#endif /* SRC_SHARED_LEDMANAGER_H_ */
