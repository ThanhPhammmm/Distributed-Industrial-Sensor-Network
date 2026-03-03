/*
 * Configuration.h
 *
 *  Created on: Mar 2, 2026
 *      Author: ThanhPham25
 */

#ifndef INC_CONFIGURATION_H_
#define INC_CONFIGURATION_H_

#define MAX_SLAVES					32
#define SLAVE_ADDR_MIN				1
#define SLAVE_ADDR_MAX				254
#define MAX_SENSORS_PER_SLAVE		8

#define RS485_UART_INSTANCE			USART2
#define RS485_DMA_STREAM_TX			DMA1_Stream6
#define RS485_DMA_STREAM_RX			DMA1_Stream5
#define RS485_DE_PORT				GPIOA
#define RS485_DE_PIN				GPIO_PIN_8
#define RS485_BAUDRATE				115200

#define RS485_TX_QUEUE_SIZE			8    /* pending TX frames   */
#define RS485_RX_FRAME_QUEUE_SIZE	8    /* parsed frames → Protocol_Task */

#define PROTOCOL_RETRY_MAX				3
#define PROTOCOL_TIMEOUT_MS				200
#define PROTOCOL_TX_CMD_QUEUE_SIZE		8    /* DeviceManager → Protocol_Task */
#define PROTOCOL_VALID_FRAME_QUEUE_SIZE	8   /* Protocol_Task → DeviceManager */

#define DEVMGR_POLL_INTERVAL_MS		500
#define DEVMGR_OFFLINE_THRESHOLD	5    /* missed polls before offline */

#define PRIO_PROTOCOL				6
#define PRIO_DEVMGR					5

#define STACK_PROTOCOL				512
#define STACK_DEVMGR				512

#endif /* INC_CONFIGURATION_H_ */
