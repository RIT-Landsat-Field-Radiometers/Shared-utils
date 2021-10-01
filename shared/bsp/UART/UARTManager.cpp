/*
 * UARTManager.cpp
 *
 *  Created on: Jul 29, 2021
 *      Author: reedt
 */

#include "UARTManager.h"

#include <cstdio>
#include <string.h>
#include <stdio.h>
#include <cstdarg>



typedef struct
{
	char * str;
	size_t length;
} message;

static UARTManager * manager = nullptr;
static char * msgToFree = nullptr;


UARTManager::UARTManager(UART_HandleTypeDef * uart)
{
	this->uart = uart;
	manager = this;
	UartLock = 0;
}


void UARTManager::txComplete(UART_HandleTypeDef *huart)
{
	if(manager != nullptr && huart == manager->uart)
	{
		osEventFlagsSet(manager->UartLock, 0x01); // clear to send
	}
}


__NO_RETURN void UARTManager::main(void* arg)
{
	UARTManager * manager = (UARTManager *) arg;
	char * msg;

	HAL_UART_RegisterCallback(manager->uart, HAL_UART_TX_COMPLETE_CB_ID, txComplete);

	for(;;)
	{
		auto retval = osMessageQueueGet(manager->UARTOutboxHandle, &msg, nullptr,
		osWaitForever);
		if (retval == osOK)
		{
			osEventFlagsWait(manager->UartLock, 0x01, osFlagsWaitAll, osWaitForever);
			if(msgToFree != nullptr)
			{
				vPortFree(msgToFree);
			}
			int size = strlen(msg);
			HAL_UART_Transmit_DMA(manager->uart, (uint8_t*) msg, size);
			msgToFree = msg;
		}
		osDelay(1);
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
	UARTOutboxHandle = osMessageQueueNew(16, sizeof(char *),
			&UARTOutbox_attributes);

	/* creation of ProcessUART */
	ProcessUARTHandle = osThreadNew(this->main, (void*) this,
			&ProcessUART_attributes);
}

void UARTManager::print(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char *buffer = (char *) pvPortMalloc(vsnprintf(nullptr, 0, fmt, args) + 1);
	va_end (args);
	va_start(args, fmt);
	vsprintf(buffer, fmt, args);
	va_end (args);

	osMessageQueuePut(UARTOutboxHandle, &buffer, 0, 0);
}

void UARTManager::vprint(const char *format, va_list arg)
{
	char *buffer = (char *) pvPortMalloc(vsnprintf(nullptr, 0, format, arg) + 1);
	vsprintf(buffer, format, arg);
	osMessageQueuePut(UARTOutboxHandle, &buffer, 0, 0);
}

UARTManager::~UARTManager()
{

}

