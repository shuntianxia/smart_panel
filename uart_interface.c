/**
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 */
#include "wiced.h"
#include "uart_interface.h"
#include "wiced_time.h"
#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "wwd_constants.h"
#include "smart_panel.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define UART_RECEIVE_STACK_SIZE (6200)
#define RX_BUFFER_SIZE    64
#define CMD_HEAD_BYTES 8

/******************************************************
 *                    Constants
 ******************************************************/



/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/


/******************************************************
 *               Static Function Declarations
 ******************************************************/



/******************************************************
 *               Variable Definitions
 ******************************************************/
static wiced_thread_t uart_thread;
extern glob_info_t this_dev;

wiced_uart_config_t uart_config =
{
    .baud_rate    = 9600,
    .data_width   = DATA_WIDTH_8BIT,
    .parity       = NO_PARITY,
    .stop_bits    = STOP_BITS_1,
    .flow_control = FLOW_CONTROL_DISABLED,
};

wiced_ring_buffer_t rx_buffer;
uint8_t             rx_data[RX_BUFFER_SIZE];
uart_data_info uart_msg;


/******************************************************
 *               Function Definitions
 ******************************************************/
wiced_result_t uart_receive_enable(wiced_thread_function_t function)
{
	/* Initialise ring buffer */
    ring_buffer_init(&rx_buffer, rx_data, RX_BUFFER_SIZE );

    /* Initialise UART. A ring buffer is used to hold received characters */
    wiced_uart_init( E200_UART, &uart_config, &rx_buffer );

	wiced_rtos_create_thread(&uart_thread, WICED_NETWORK_WORKER_PRIORITY, "uart receive thread", function, UART_RECEIVE_STACK_SIZE, 0);
	return WICED_SUCCESS;
}

void master_process_uart_msg(uint32_t arg)
{
	char c;
	while ( wiced_uart_receive_bytes( WICED_UART_2, &c, 1, WICED_NEVER_TIMEOUT ) == WICED_SUCCESS )
	{
		WPRINT_APP_INFO(("RcvChar=0x%02x\n", c));
		/* you can add your handle code below.*/
		if(uart_msg.state == DATA_IDLE)
		{
			//*(pRxBuff->pWritePos) = RcvChar;
			uart_msg.msg_buf[uart_msg.pos++] = c;
			//pRxBuff->pWritePos++;

			if(uart_msg.pos == CMD_HEAD_BYTES)
			{
				uart_msg.data_len = *((uint8_t *)uart_msg.msg_buf+4);
			}

			if(uart_msg.pos == (CMD_HEAD_BYTES + uart_msg.data_len)) // Complete data reception
			{
				if(uart_msg.msg_buf[0] == 0x42 && uart_msg.msg_buf[1] == 0x53 && uart_msg.msg_buf[2] == 0x54)
				{
					uart_msg.state = DATA_READY;
					//if(this_dev.next_dev_ip != NULL) {
					//if(cur_ctrl_flag == CTRL_FLAG_NONE) {
						WPRINT_APP_INFO(("forward uart msg via socket\n"));
						send_to_next_dev((char*)uart_msg.msg_buf, uart_msg.pos);
						this_dev.cur_ctrl_flag = CTRL_FLAG_PANEL;
					//}
					//}
					//forward_to_next_dev(uart_msg.msg_buf, uart_msg.data_len);
				}
				uart_msg.state = DATA_IDLE;
				uart_msg.pos = 0;
			}
		}
	}
}

