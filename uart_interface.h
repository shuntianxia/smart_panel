/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "wiced_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum {
	DATA_IDLE = 0x0,
	DATA_READY = 0x1
} uart_data_state;

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef void (*uart_receive_handler_t)(void);


/******************************************************
 *                    Structures
 ******************************************************/
typedef struct {
	uint8_t msg_buf[64];
	uint8_t pos;
	uint8_t data_len;
	uart_data_state state;
} uart_data_info;


/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
wiced_result_t uart_receive_enable(wiced_thread_function_t function);
void master_process_uart_msg();
//void device_process_uart_msg();

#ifdef __cplusplus
} /* extern "C" */
#endif
