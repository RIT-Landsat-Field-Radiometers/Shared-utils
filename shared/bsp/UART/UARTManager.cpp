/*
 * UARTManager.cpp
 *
 *  Created on: Jul 29, 2021
 *      Author: reedt
 */

#include "UARTManager.h"

#include <string.h>
#include <cstdarg>

#include "Logging/printf.h"


typedef struct
{
	char *str;
	size_t length;
} message;

static UARTManager *manager = nullptr;
static char *msgToFree = nullptr;
static int msgSize = 0;


UARTManager::UARTManager(UART_HandleTypeDef *uart)
{
	this->uart = uart;
	manager = this;
	UartLock = 0;
}

void UARTManager::txComplete(UART_HandleTypeDef *huart)
{
	if (manager != nullptr && huart == manager->uart)
	{
		osEventFlagsSet(manager->UartLock, 0x01); // clear to send
	}
}

__NO_RETURN void UARTManager::main(void *arg)
{
	UARTManager *manager = (UARTManager*) arg;
	char *msg;

	HAL_UART_RegisterCallback(manager->uart, HAL_UART_TX_COMPLETE_CB_ID,
			txComplete);

	for (;;)
	{
		int32_t flags = osEventFlagsWait(manager->UartLock, 0x01,
				osFlagsWaitAll | osFlagsNoClear, msgSize > 0 ? 1 * msgSize : 10);

		if(flags < 1 && msgToFree != nullptr)
		{
			// If this times out (10ms) something is wrong, so abort the request and do the next message
			HAL_UART_Abort(manager->uart);
			HAL_UART_Transmit(manager->uart, (uint8_t*) "\r\n", 2, 10); // Add a new line so the next message isn't combined with the failed one
		}

		if (msgToFree != nullptr)
		{
			vPortFree(msgToFree);
			msgToFree = nullptr;
			msgSize = 0;
		}
		auto retval = osMessageQueueGet(manager->UARTOutboxHandle, &msg, nullptr, 10);
		if (retval == osOK)
		{
			int size = strlen(msg);
			HAL_UART_Transmit_DMA(manager->uart, (uint8_t*) msg, size);
			osEventFlagsClear(manager->UartLock, 0x01);
			msgToFree = msg;
			msgSize = size;
		}
	}
	osThreadExit();
}

void UARTManager::start()
{
	UartLock = osEventFlagsNew(nullptr);
	osEventFlagsSet(UartLock, 0x01); // Clear to send;

	ProcessUART_attributes.priority = (osPriority_t) osPriorityNormal;
	ProcessUART_attributes.stack_size = 2048 * 4;

	/* creation of UARTOutbox */
	UARTOutboxHandle = osMessageQueueNew(16, sizeof(char*),
			&UARTOutbox_attributes);

	/* creation of ProcessUART */
	ProcessUARTHandle = osThreadNew(this->main, (void*) this,
			&ProcessUART_attributes);
}

void UARTManager::print(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	size_t strsize = vsnprintf(nullptr, 0, fmt, args) + 1;
	char *buffer = (char*) pvPortMalloc(strsize);
	va_end(args);
	va_start(args, fmt);
	vsnprintf(buffer, strsize, fmt, args);
	va_end(args);

	if (osMessageQueuePut(UARTOutboxHandle, &buffer, 0, 0) != osOK)
	{
		vPortFree(buffer);
	}
}

void UARTManager::vprint(const char *format, va_list arg)
{
	size_t strsize = vsnprintf(nullptr, 0, format, arg) + 1;
	char *buffer = (char*) pvPortMalloc(strsize);
	vsnprintf(buffer, strsize,format, arg);

	if (osMessageQueuePut(UARTOutboxHandle, &buffer, 0, 0) != osOK)
	{
		vPortFree(buffer);
	}
}

UARTManager::~UARTManager()
{

}

