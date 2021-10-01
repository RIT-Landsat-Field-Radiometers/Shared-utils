/*
 * UARTManager.h
 *
 *  Created on: Jul 29, 2021
 *      Author: reedt
 */

#ifndef SRC_SHARED_LOGGING_UARTMANAGER_H_
#define SRC_SHARED_LOGGING_UARTMANAGER_H_

#include "main.h"
#include <cstdint>
#include "cmsis_os.h"
#include <stdarg.h>



class UARTManager
{
private:
	UART_HandleTypeDef * uart;

	/* Definitions for ProcessUART */
	osThreadId_t ProcessUARTHandle;
	osThreadAttr_t ProcessUART_attributes =
	{ .name = "ProcessUART", };

	/* Definitions for UARTOutbox */
	osMessageQueueId_t UARTOutboxHandle;
	const osMessageQueueAttr_t UARTOutbox_attributes =
	{ .name = "UARTOutbox" };

	osEventFlagsId_t UartLock;

	static void main(void *);

	static void txComplete(UART_HandleTypeDef *huart);

public:
	UARTManager(UART_HandleTypeDef * uart);
	void start();
	void print(const char *fmt, ...);
	void vprint( const char * format, va_list arg );

	virtual ~UARTManager();
	UARTManager(const UARTManager &other) = default;
	UARTManager(UARTManager &&other) = default;
	UARTManager& operator=(const UARTManager &other) = default;
	UARTManager& operator=(UARTManager &&other) = default;
};

#endif /* SRC_SHARED_LOGGING_UARTMANAGER_H_ */
