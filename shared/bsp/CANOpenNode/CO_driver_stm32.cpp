/*
 * CAN module object for generic microcontroller.
 *
 * This file is a template for other microcontrollers.
 *
 * @file        CO_driver.c
 * @ingroup     CO_driver
 * @author      Janez Paternoster
 * @copyright   2004 - 2020 Janez Paternoster
 *
 * This file is part of CANopenNode, an opensource CANopen Stack.
 * Project home page is <https://github.com/CANopenNode/CANopenNode>.
 * For more information on CANopen see <http://www.can-cia.org/>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "301/CO_driver.h"
#include "can.h"
#include "main.h"
#include "CANopen.h"

static CO_CANmodule_t* ISR_CANModule_handle = NULL;


/******************************************************************************DONE*/
void CO_CANsetConfigurationMode(void *CANptr)
{
	/* Put CAN module in configuration mode */
	/* Covered by STM32 HAL */
}

/*****************************************************************************DONE*/
void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule)
{
	/* Put CAN module in normal mode */
	if (HAL_CAN_Start((CAN_HandleTypeDef*) CANmodule->CANptr) != HAL_OK)
	{
		/* Start Error */
	}
	if (HAL_CAN_ActivateNotification((CAN_HandleTypeDef*) CANmodule->CANptr,
	CAN_IT_RX_FIFO0_MSG_PENDING |
	CAN_IT_RX_FIFO1_MSG_PENDING |
	CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK)
	{
		/* Notification Error */
	}

	CANmodule->CANnormal = true;
}

/*****************************************************************************DONE*/
CO_ReturnError_t CO_CANmodule_init(CO_CANmodule_t *CANmodule, void *CANptr,
		CO_CANrx_t rxArray[], uint16_t rxSize, CO_CANtx_t txArray[],
		uint16_t txSize, uint16_t CANbitRate)
{
	uint16_t i;

	/* verify arguments */
	if (CANmodule == NULL || rxArray == NULL || txArray == NULL)
	{
		return CO_ERROR_ILLEGAL_ARGUMENT;
	}

	ISR_CANModule_handle = CANmodule;

	/* Configure object variables */
	CANmodule->CANptr = CANptr;
	CANmodule->rxArray = rxArray;
	CANmodule->rxSize = rxSize;
	CANmodule->txArray = txArray;
	CANmodule->txSize = txSize;
	CANmodule->CANerrorStatus = 0;
	CANmodule->CANnormal = false;
	CANmodule->useCANrxFilters = false; //(rxSize <= 32U) ? true : false;/* microcontroller dependent */
	CANmodule->bufferInhibitFlag = false;
	CANmodule->firstCANtxMessage = true;
	CANmodule->CANtxCount = 0U;
	CANmodule->errOld = 0U;

	for (i = 0U; i < rxSize; i++)
	{
		rxArray[i].ident = 0U;
		rxArray[i].mask = 0xFFFFU;
		rxArray[i].object = NULL;
		rxArray[i].CANrx_callback = NULL;
	}
	for (i = 0U; i < txSize; i++)
	{
		txArray[i].bufferFull = false;
	}

	/* Configure CAN module registers */
	CO_CANmodule_disable(CANmodule);
	HAL_CAN_MspDeInit((CAN_HandleTypeDef*) CANmodule->CANptr);
	HAL_CAN_MspInit((CAN_HandleTypeDef*) CANmodule->CANptr); /* NVIC and GPIO */

	/* Configure CAN timing */
	MX_CAN1_Init();

	/* Configure CAN module hardware filters */
	CAN_FilterTypeDef can1_filter_init;

	can1_filter_init.FilterActivation = ENABLE;
	can1_filter_init.FilterBank  = 0;
	can1_filter_init.FilterFIFOAssignment = CAN_RX_FIFO0;
	can1_filter_init.FilterIdHigh = 0x0000;
	can1_filter_init.FilterIdLow = 0x0000;
	can1_filter_init.FilterMaskIdHigh = 0x0000;
	can1_filter_init.FilterMaskIdLow = 0x0000;
	can1_filter_init.FilterMode = CAN_FILTERMODE_IDMASK;
	can1_filter_init.FilterScale = CAN_FILTERSCALE_32BIT;

	if( HAL_CAN_ConfigFilter((CAN_HandleTypeDef*) CANmodule->CANptr,&can1_filter_init) != HAL_OK)
	{
		Error_Handler();
	}

	/* configure CAN interrupt registers */
	if (HAL_CAN_ActivateNotification((CAN_HandleTypeDef*) CANmodule->CANptr,
	CAN_IT_RX_FIFO0_MSG_PENDING |
	CAN_IT_RX_FIFO1_MSG_PENDING |
	CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK)
	{
		/* Notification Error */
	}

	return CO_ERROR_NO;
}

/*****************************************************************************DONE*/
void CO_CANmodule_disable(CO_CANmodule_t *CANmodule)
{
	if (CANmodule != NULL)
	{
		/* turn off the module */
		HAL_CAN_DeactivateNotification((CAN_HandleTypeDef*) CANmodule->CANptr ,
				CAN_IT_RX_FIFO0_MSG_PENDING |
				CAN_IT_RX_FIFO1_MSG_PENDING |
				CAN_IT_TX_MAILBOX_EMPTY);
		HAL_CAN_Stop((CAN_HandleTypeDef*) CANmodule->CANptr);
	}
}

/*****************************************************************************DONE*/
CO_ReturnError_t CO_CANrxBufferInit(CO_CANmodule_t *CANmodule, uint16_t index,
		uint16_t ident, uint16_t mask, bool_t rtr, void *object,
		void (*CANrx_callback)(void *object, void *message))
{
	CO_ReturnError_t ret = CO_ERROR_NO;

	if ((CANmodule != NULL) && (object != NULL) && (CANrx_callback != NULL)
			&& (index < CANmodule->rxSize))
	{
		/* buffer, which will be configured */
		CO_CANrx_t *buffer = &CANmodule->rxArray[index];

		/* Configure object variables */
		buffer->object = object;
		buffer->CANrx_callback = CANrx_callback;

		/* CAN identifier and CAN mask, bit aligned with CAN module. Different on different microcontrollers. */
		buffer->ident = (ident & 0x07FFU) << 2; // TODO: Taken from HAL example, need to verify
		if (rtr)
		{
//			buffer->ident |= 0x0800U;
			buffer->ident |= 0x02;
		}
//		buffer->mask = (mask & 0x07FFU) | 0x0800U;
		buffer->mask = (mask & 0x07FFU) << 2;
		buffer->mask |= 0x02;

		CAN_FilterTypeDef can1_filter_init;
		can1_filter_init.FilterActivation = ENABLE;
		can1_filter_init.FilterBank  = 0;
		can1_filter_init.FilterFIFOAssignment = CAN_RX_FIFO0;
		can1_filter_init.FilterIdHigh = 0x0000;
		can1_filter_init.FilterIdLow = 0x0000;
		can1_filter_init.FilterMaskIdHigh = 0x0000;
		can1_filter_init.FilterMaskIdLow = 0x0000;
		can1_filter_init.FilterMode = CAN_FILTERMODE_IDMASK;
		can1_filter_init.FilterScale = CAN_FILTERSCALE_32BIT;

		if( HAL_CAN_ConfigFilter((CAN_HandleTypeDef*) CANmodule->CANptr,&can1_filter_init) != HAL_OK)
		{
			ret = CO_ERROR_SYSCALL;
		}
	}
	else
	{
		ret = CO_ERROR_ILLEGAL_ARGUMENT;
	}

	return ret;
}

/*****************************************************************************DONE*/
CO_CANtx_t* CO_CANtxBufferInit(CO_CANmodule_t *CANmodule, uint16_t index,
		uint16_t ident, bool_t rtr, uint8_t noOfBytes, bool_t syncFlag)
{
	CO_CANtx_t *buffer = NULL;

	if ((CANmodule != NULL) && (index < CANmodule->txSize))
	{
		/* get specific buffer */
		buffer = &CANmodule->txArray[index];

		/* CAN identifier, DLC and rtr, bit aligned with CAN module transmit buffer.
		 * Microcontroller specific. */
//		buffer->ident = ((uint32_t) ident & 0x07FFU)
//				| ((uint32_t) (((uint32_t) noOfBytes & 0xFU) << 12U))
//				| ((uint32_t) (rtr ? 0x8000U : 0U));
		buffer->ident = (ident & 0x7ff) << 2;
		if (rtr) buffer->ident |= 0x02;

		buffer->DLC = noOfBytes;
		buffer->bufferFull = false;
		buffer->syncFlag = syncFlag;
	}

	return buffer;
}

/*****************************************************************************DONE*/
CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
	CO_ReturnError_t err = CO_ERROR_NO;

	/* Verify overflow */
	if (buffer->bufferFull)
	{
		if (!CANmodule->firstCANtxMessage)
		{
			/* don't set error, if bootup message is still on buffers */
			CANmodule->CANerrorStatus |= CO_CAN_ERRTX_OVERFLOW;
		}
		err = CO_ERROR_TX_OVERFLOW;
	}

	CO_LOCK_CAN_SEND(CANmodule);

	uint32_t TxMailboxNum;
	CAN_TxHeaderTypeDef TxHeader;

	TxHeader.ExtId = 0u;
	TxHeader.IDE = 0;
	TxHeader.DLC = buffer->DLC;
	TxHeader.StdId = ( buffer->ident >> 2 );
	TxHeader.RTR = ( buffer->ident & 0x2 );


	/* if CAN TX buffer is free, copy message to it */
	if ((HAL_CAN_GetTxMailboxesFreeLevel((CAN_HandleTypeDef*) CANmodule->CANptr) > 0 )
			&& CANmodule->CANtxCount == 0)
	{
		CANmodule->bufferInhibitFlag = buffer->syncFlag;
		/* copy message and txRequest */
		if(HAL_CAN_AddTxMessage((CAN_HandleTypeDef*) CANmodule->CANptr, &TxHeader, &buffer->data[0], &TxMailboxNum) != HAL_OK)
		{
			err = CO_ERROR_SYSCALL;
		}
	}
	/* if no buffer is free, message will be sent by interrupt */
	else
	{
		buffer->bufferFull = true;
		CANmodule->CANtxCount++;
	}
	CO_UNLOCK_CAN_SEND(CANmodule);

	return err;
}

/*****************************************************************************DONE??*/
void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule)
{
	uint32_t tpdoDeleted = 0U;

	CO_LOCK_CAN_SEND(CANmodule);
	/* Abort message from CAN module, if there is synchronous TPDO.
	 * Take special care with this functionality. */
	auto pendingMsgs = 3 - HAL_CAN_GetTxMailboxesFreeLevel((CAN_HandleTypeDef*) CANmodule->CANptr);
	if ( (pendingMsgs > 0) && CANmodule->bufferInhibitFlag)
	{
		// TODO: Come back to this later, does it clear all queued messages or only the TPDOs?
		//		HAL_CAN_AbortTxRequest((CAN_HandleTypeDef*) CANmodule->CANptr, (CAN_TX_MAILBOX0 | CAN_TX_MAILBOX1 | CAN_TX_MAILBOX2));
		/* clear TXREQ */
		CANmodule->bufferInhibitFlag = false;
		tpdoDeleted = 1U;
	}
	/* delete also pending synchronous TPDOs in TX buffers */
	if (CANmodule->CANtxCount != 0U)
	{
		uint16_t i;
		CO_CANtx_t *buffer = &CANmodule->txArray[0];
		for (i = CANmodule->txSize; i > 0U; i--)
		{
			if (buffer->bufferFull)
			{
				if (buffer->syncFlag)
				{
					buffer->bufferFull = false;
					CANmodule->CANtxCount--;
					tpdoDeleted = 2U;
				}
			}
			buffer++;
		}
	} CO_UNLOCK_CAN_SEND(CANmodule);

	if (tpdoDeleted != 0U)
	{
		CANmodule->CANerrorStatus |= CO_CAN_ERRTX_PDO_LATE;
	}
}

/*****************************************************************************DONE?*/
/* Get error counters from the module. If necessary, function may use
 * different way to determine errors. */
static uint16_t rxErrors = 0, txErrors = 0, overflow = 0;

void CO_CANmodule_process(CO_CANmodule_t *CANmodule)
{
	uint32_t err;

	err = ((uint32_t) txErrors << 16) | ((uint32_t) rxErrors << 8) | overflow;

	if (CANmodule->errOld != err)
	{
		uint16_t status = CANmodule->CANerrorStatus;

		CANmodule->errOld = err;

		if (txErrors >= 256U)
		{
			/* bus off */
			status |= CO_CAN_ERRTX_BUS_OFF;
		}
		else
		{
			/* recalculate CANerrorStatus, first clear some flags */
			status &= 0xFFFF
					^ (CO_CAN_ERRTX_BUS_OFF | CO_CAN_ERRRX_WARNING
							| CO_CAN_ERRRX_PASSIVE | CO_CAN_ERRTX_WARNING
							| CO_CAN_ERRTX_PASSIVE);

			/* rx bus warning or passive */
			if (rxErrors >= 128)
			{
				status |= CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE;
			}
			else if (rxErrors >= 96)
			{
				status |= CO_CAN_ERRRX_WARNING;
			}

			/* tx bus warning or passive */
			if (txErrors >= 128)
			{
				status |= CO_CAN_ERRTX_WARNING | CO_CAN_ERRTX_PASSIVE;
			}
			else if (rxErrors >= 96)
			{
				status |= CO_CAN_ERRTX_WARNING;
			}

			/* if not tx passive clear also overflow */
			if ((status & CO_CAN_ERRTX_PASSIVE) == 0)
			{
				status &= 0xFFFF ^ CO_CAN_ERRTX_OVERFLOW;
			}
		}

		if (overflow != 0)
		{
			/* CAN RX bus overflow */
			status |= CO_CAN_ERRRX_OVERFLOW;
		}

		CANmodule->CANerrorStatus = status;
	}
}

/******************************************************************************/


// DONE???
void CO_CAN_RXISR(CAN_HandleTypeDef *hcan, CO_CANmodule_t *CANmodule, uint8_t fifo)
{
	static CO_CANrxMsg_t rcvMsg; /* pointer to received message in CAN module */
	uint16_t index; /* index of received message */
	uint32_t rcvMsgIdent; /* identifier of the received message */
	CO_CANrx_t *buffer; /* receive message buffer from CO_CANmodule_t object. */
	bool_t msgMatched = false;

//	rcvMsg = 0; /* get message from module here */
	uint32_t fifonum;
	if(fifo == 0)
	{
		fifonum = CAN_RX_FIFO0;
	}
	else if(fifo == 1)
	{
		fifonum = CAN_RX_FIFO1;
	}
	else
	{
		rxErrors++;
		return; // Invalid parameter
	}

	auto res = HAL_CAN_GetRxMessage((CAN_HandleTypeDef*) CANmodule->CANptr, fifonum, &rcvMsg.RxHeader, &rcvMsg.data[0]);
	if(res != HAL_OK)
	{
		rxErrors++;
		return; // Error
	}

	rcvMsgIdent = (rcvMsg.RxHeader.StdId << 2) | (rcvMsg.RxHeader.RTR << 1);


	if (CANmodule->useCANrxFilters)
	{
		/* CAN module filters are used. Message with known 11-bit identifier has */
		/* been received */
		// TODO: Unsupported for now
//		index = 0; /* get index of the received message here. Or something similar */
//		if (index < CANmodule->rxSize)
//		{
//			buffer = &CANmodule->rxArray[index];
//			/* verify also RTR */
//			if (((rcvMsgIdent ^ buffer->ident) & buffer->mask) == 0U)
//			{
//				msgMatched = true;
//			}
//		}
	}
	else
	{
		/* CAN module filters are not used, message with any standard 11-bit identifier */
		/* has been received. Search rxArray form CANmodule for the same CAN-ID. */
		buffer = &CANmodule->rxArray[0];
		for (index = CANmodule->rxSize; index > 0U; index--)
		{
			if (((rcvMsgIdent ^ buffer->ident) & buffer->mask) == 0U)
			{
				msgMatched = true;
				break;
			}
			buffer++;
		}
	}

	/* Call specific function, which will process the message */
	if (msgMatched && (buffer != NULL) && (buffer->CANrx_callback != NULL))
	{
		buffer->CANrx_callback(buffer->object, (void*) &rcvMsg);
	}

	/* Clear interrupt flag */
}

// DONE??
void CO_CAN_TXISR(CAN_HandleTypeDef *hcan, CO_CANmodule_t *CANmodule)
{
	/* Clear interrupt flag */

	/* First CAN message (bootup) was sent successfully */
	CANmodule->firstCANtxMessage = false;
	/* clear flag from previous message */
	CANmodule->bufferInhibitFlag = false;
	/* Are there any new messages waiting to be send */
	if (CANmodule->CANtxCount > 0U)
	{
		uint16_t i; /* index of transmitting message */

		/* first buffer */
		CO_CANtx_t *buffer = &CANmodule->txArray[0];
		/* search through whole array of pointers to transmit message buffers. */
		for (i = CANmodule->txSize; i > 0U; i--)
		{
			/* if message buffer is full, send it. */
			if (buffer->bufferFull)
			{
				buffer->bufferFull = false;
				CANmodule->CANtxCount--;

				/* Copy message to CAN buffer */
				CANmodule->bufferInhibitFlag = buffer->syncFlag;

				/* canSend... */
				uint32_t TxMailbox;
				CAN_TxHeaderTypeDef TxHeader;

				TxHeader.ExtId = 0u;
				TxHeader.IDE = 0;
				TxHeader.DLC = buffer->DLC;
				TxHeader.StdId = ( buffer->ident >> 2 );
				TxHeader.RTR = ( buffer->ident & 0x2 );
				if(HAL_CAN_AddTxMessage((CAN_HandleTypeDef*) CANmodule->CANptr, &TxHeader, &buffer->data[0], &TxMailbox) != HAL_OK)
				{
					txErrors++;
				}

				break; /* exit for loop */
			}
			buffer++;
		}/* end of for loop */

		/* Clear counter if no more messages */
		if (i == 0U)
		{
			CANmodule->CANtxCount = 0U;
		}
	}
}

// DONE
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
	if(ISR_CANModule_handle != nullptr) CO_CAN_TXISR(hcan, ISR_CANModule_handle);
}
// DONE
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
	if(ISR_CANModule_handle != nullptr) CO_CAN_TXISR(hcan, ISR_CANModule_handle);
}
// DONE
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
	if(ISR_CANModule_handle != nullptr) CO_CAN_TXISR(hcan, ISR_CANModule_handle);
}
// DONE
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(ISR_CANModule_handle != nullptr) CO_CAN_RXISR(hcan, ISR_CANModule_handle, 0);
}
// DONE
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(ISR_CANModule_handle != nullptr) CO_CAN_RXISR(hcan, ISR_CANModule_handle, 1);
}
// DONE
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	auto err = HAL_CAN_GetError(hcan);
	switch(err)
	{
	case HAL_CAN_ERROR_TX_TERR0:	// Not considering an arbitration lost as error, auto-retransmit is on
	case HAL_CAN_ERROR_TX_TERR1:
	case HAL_CAN_ERROR_TX_TERR2:
		txErrors++;
		break;
	case HAL_CAN_ERROR_RX_FOV0:
	case HAL_CAN_ERROR_RX_FOV1:
		rxErrors++;
		break;
	case HAL_CAN_ERROR_NONE:
	default:
		return;
	}
}
