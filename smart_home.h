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

#include "light_dev.h"
#include "curtain_dev.h"

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

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef wiced_result_t (*parse_socket_msg_fun_t)(char *, uint16_t);

typedef wiced_result_t (*parse_uart_msg_fun_t)();

typedef void (*device_keypad_handler_t)(uart_key_code_t, uart_key_event_t);

//typedef void (*uart_receive_handler_t)(void);


/******************************************************
 *                    Structures
 ******************************************************/

typedef struct {
	wiced_bool_t configured;
	uint8_t dev_index;
	uint8_t dev_type;
	uint8_t dev_name[32];
	wiced_ip_address_t *pre_dev_ip;
	wiced_ip_address_t *next_dev_ip;
	wiced_ip_address_t ctrl_ip_addr;
	uint16_t ctrl_port;
	int cur_ctrl_flag;
	wiced_timed_event_t udp_delay_event;
	parse_socket_msg_fun_t parse_socket_msg_fun;
	parse_uart_msg_fun_t parse_uart_msg_fun;
	device_keypad_handler_t device_keypad_handler;
	//uart_receive_handler_t uart_receive_handler;
	union {
		light_dev_t *light_dev;
		curtain_t *curtain;
	} specific;
} glob_info_t;



/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
