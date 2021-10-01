/*
 * UIManager.cpp
 *
 *  Created on: Jul 29, 2021
 *      Author: reedt
 */

#include "LEDManager.h"

LEDManager::LEDManager(gpio_pin_t red, gpio_pin_t green, gpio_pin_t blue)
{
	this->red = red;
	this->green = green;
	this->blue = blue;
}

__NO_RETURN void LEDManager::main(void *arg)
{
	/*
	 * * +-------------+----------------------+--------------------+--------------------+--------------------+
	 * |   RESERVED  |      FAST FLASH      |     SLOW FLASH     |         ON         |         OFF        |
	 * +=======+=====+=======+=======+======+======+======+======+======+======+======+======+======+======+
	 * | bit31 | ... | bit11 | bit10 | bit9 | bit8 | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |
	 * +-------+-----+-------+-------+------+------+------+------+------+------+------+------+------+------+
	 * |       |     |   R   |   G   |   B  |   R  |   G  |   B  |   R  |   G  |   B  |   R  |   G  |   B  |
	 * +-------+-----+-------+-------+------+------+------+------+------+------+------+------+------+------+
	 *
	 */

	LEDManager *manager = (LEDManager*) arg;
	for (;;)
	{
		uint32_t flags = osEventFlagsGet(manager->LEDEventHandle);
		if ((flags & 0xFFF) != 0x00)
		{
			if(manager->timeoutEnabled)
			{
				osTimerStart(manager->LEDTimeoutHandle, manager->LEDTimeout * 1000); // got something new, if nothing new for 10 seconds, turn off
			}

			osEventFlagsClear(manager->LEDEventHandle, 0xFFF);

			uint8_t changed = (flags & 0x7) | ((flags >> ON_OFFSET) & 0x7)
					| ((flags >> SLOW_OFFSET) & 0x7)
					| ((flags >> FAST_OFFSET) & 0x7);
			// LED state change occurred
			for (int mode = 3; mode >= 0; mode--)
			{
				uint8_t bits = flags >> (3 * (mode));
				bits &= 0b111;
				for (int idx = 0; idx < 3; idx++)
				{
					if ((bits >> idx) & 0b001)
					{
						manager->states[idx] = (LEDstate_t) mode;
					}
				}
			}
			uint8_t slow_update = 0x00;
			uint8_t fast_update = 0x00;

			if (changed & BLUE_MASK)
			{
				// Blue change
				switch (manager->states[0])
				{
				case SLOW:
					slow_update |= BLUE_MASK;
					fast_update |= (BLUE_MASK << 3);
					break;
				case FAST:
					fast_update |= BLUE_MASK;
					slow_update |= (BLUE_MASK << 3);
					break;
				case ON:
					fast_update |= (BLUE_MASK << 3);
					slow_update |= (BLUE_MASK << 3);
					HAL_GPIO_WritePin(manager->blue.port, manager->blue.pin,
							GPIO_PIN_SET);
					break;
				case OFF:
				default:
					fast_update |= (BLUE_MASK << 3);
					slow_update |= (BLUE_MASK << 3);
					HAL_GPIO_WritePin(manager->blue.port, manager->blue.pin,
							GPIO_PIN_RESET);
					break;
				}
			}
			if (changed & GREEN_MASK)
			{
				// Green change
				switch (manager->states[1])
				{
				case SLOW:
					slow_update |= GREEN_MASK;
					fast_update |= (GREEN_MASK << 3);
					break;
				case FAST:
					fast_update |= GREEN_MASK;
					slow_update |= (GREEN_MASK << 3);
					break;
				case ON:
					fast_update |= (GREEN_MASK << 3);
					slow_update |= (GREEN_MASK << 3);
					HAL_GPIO_WritePin(manager->green.port, manager->green.pin,
							GPIO_PIN_SET);
					break;
				case OFF:
				default:
					fast_update |= (GREEN_MASK << 3);
					slow_update |= (GREEN_MASK << 3);
					HAL_GPIO_WritePin(manager->green.port, manager->green.pin,
							GPIO_PIN_RESET);
					break;
				}
			}
			if (changed & RED_MASK)
			{
				// Red change
				switch (manager->states[2])
				{
				case SLOW:
					slow_update |= RED_MASK;
					fast_update |= (RED_MASK << 3);
					break;
				case FAST:
					fast_update |= RED_MASK;
					slow_update |= (RED_MASK << 3);
					break;
				case ON:
					fast_update |= (RED_MASK << 3);
					slow_update |= (RED_MASK << 3);
					HAL_GPIO_WritePin(manager->red.port, manager->red.pin,
							GPIO_PIN_SET);
					break;
				case OFF:
				default:
					fast_update |= (RED_MASK << 3);
					slow_update |= (RED_MASK << 3);
					HAL_GPIO_WritePin(manager->red.port, manager->red.pin,
							GPIO_PIN_RESET);
					break;
				}
			}

			uint32_t update = 0;
			update |= slow_update << 12;
			update |= fast_update << 18;

			osEventFlagsSet(manager->LEDEventHandle, update);
		}
		osDelay(1);
	}
	osThreadExit(); // shouldn't happen, but do it cleanly if so
}

void LEDManager::slowCallback(void *arg)
{
	LEDManager *manager = (LEDManager*) arg;
	static uint8_t leds = 0b000; // None slow-toggling
	static uint8_t cycle = 1;
	uint32_t flags = osEventFlagsGet(manager->LEDEventHandle);
	if ((flags & (0x3F << 12)) > 0)
	{
		osEventFlagsClear(manager->LEDEventHandle, 0x3F << 12);
		flags >>= 12;
		leds |= flags & 0b111;
		leds &= ~(flags >> 3);
	}

	if (leds & BLUE_MASK)
	{
		HAL_GPIO_WritePin(manager->blue.port, manager->blue.pin,
				(GPIO_PinState) cycle);
	}
	if (leds & GREEN_MASK)
	{
		HAL_GPIO_WritePin(manager->green.port, manager->green.pin,
				(GPIO_PinState) cycle);
	}
	if (leds & RED_MASK)
	{
		HAL_GPIO_WritePin(manager->red.port, manager->red.pin,
				(GPIO_PinState) cycle);
	}

	cycle ^= 0x1;
}

void LEDManager::fastCallback(void *arg)
{
	LEDManager *manager = (LEDManager*) arg;
	static uint8_t leds = 0b000; // None fast-toggling
	static uint8_t cycle = 1;
	uint32_t flags = osEventFlagsGet(manager->LEDEventHandle);
	if ((flags & (0x3F << 18)) > 0)
	{
		osEventFlagsClear(manager->LEDEventHandle, 0x3F << 18);
		flags >>= 18;
		leds |= flags & 0b111;
		leds &= ~(flags >> 3);
	}

	if (leds & BLUE_MASK)
	{
		HAL_GPIO_WritePin(manager->blue.port, manager->blue.pin,
				(GPIO_PinState) cycle);
	}
	if (leds & GREEN_MASK)
	{
		HAL_GPIO_WritePin(manager->green.port, manager->green.pin,
				(GPIO_PinState) cycle);
	}
	if (leds & RED_MASK)
	{
		HAL_GPIO_WritePin(manager->red.port, manager->red.pin,
				(GPIO_PinState) cycle);
	}

	cycle ^= 0x1;
}

void LEDManager::timeoutCallback(void* arg)
{
	LEDManager *manager = (LEDManager*) arg;
	manager->turnOff(WHITE);
}

void LEDManager::start(bool timeoutEnabled)
{
	UI_attributes.priority = (osPriority_t) osPriorityBelowNormal7;
	UI_attributes.stack_size = 2048 * 4;

	/* creation of LEDEvent */
	LEDEventHandle = osEventFlagsNew(&LEDEvent_attributes);

	/* creation of UI */
	UIHandle = osThreadNew(this->main, (void*) this, &UI_attributes);


	/* creation of LEDTimeout */
	LEDTimeoutHandle = osTimerNew(this->timeoutCallback, osTimerOnce, (void*) this,
			&LEDTimeout_attributes);
	/* creation of LEDSlow */
	LEDSlowHandle = osTimerNew(this->slowCallback, osTimerPeriodic,
			(void*) this, &LEDSlow_attributes);

	/* creation of LEDFast */
	LEDFastHandle = osTimerNew(this->fastCallback, osTimerPeriodic,
			(void*) this, &LEDFast_attributes);

	osTimerStart(LEDSlowHandle, slowPeriod / 2);
	osTimerStart(LEDFastHandle, fastPeriod / 2);
	if(timeoutEnabled)
	{
		osTimerStart(LEDTimeoutHandle, LEDTimeout * 1000);
	}
	this->timeoutEnabled = timeoutEnabled;
}

osThreadId_t LEDManager::getThread()
{
	return this->UIHandle;
}

osTimerId_t LEDManager::getFastTimer()
{
	return this->LEDFastHandle;
}

osTimerId_t LEDManager::getSlowTimer()
{
	return this->LEDSlowHandle;
}

void LEDManager::turnOff(color c)
{
	uint32_t event = ((uint8_t) c) << OFF_OFFSET;
	osEventFlagsSet(this->LEDEventHandle, event);
}

void LEDManager::turnOn(color c)
{
	uint32_t event = ((uint8_t) c) << ON_OFFSET;
	osEventFlagsSet(this->LEDEventHandle, event);
}

void LEDManager::slowFlash(color c)
{
	uint32_t event = ((uint8_t) c) << SLOW_OFFSET;
	osEventFlagsSet(this->LEDEventHandle, event);
}

void LEDManager::fastFlash(color c)
{
	uint32_t event = ((uint8_t) c) << FAST_OFFSET;
	osEventFlagsSet(this->LEDEventHandle, event);
}

void LEDManager::startTimeout()
{
	this->timeoutEnabled = true;
	osTimerStart(LEDTimeoutHandle, LEDTimeout * 1000);
}

void LEDManager::stopTimeout()
{
	this->timeoutEnabled = false;
	osTimerStop(LEDTimeoutHandle);
}

LEDManager::~LEDManager()
{
	osTimerDelete(this->LEDFastHandle);
	osTimerDelete(this->LEDSlowHandle);
	osTimerDelete(this->LEDTimeoutHandle);
	osThreadTerminate(this->UIHandle);
}

