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
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define UART_BUFF_SIZE 1024


/******************************************************
 *                   Enumerations
 ******************************************************/
enum dev_type{
	DEV_TYPE_MASTER = 0x00,
	DEV_TYPE_LIGHT = 0x01,
	DEV_TYPE_CURTAIN = 0x02,
};

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef wiced_result_t (*parse_socket_msg_fun_t)(char *, uint16_t);

typedef wiced_result_t (*parse_uart_msg_fun_t)();

typedef void (*device_keypad_handler_t)(uart_key_code_t, uart_key_event_t);

//typedef void (*uart_receive_handler_t)(void);
typedef struct {
	uint16_t	prefix;
	uint8_t 	data_len;
	uint16_t	cmd_type;
	uint8_t 	status;
} keypress_cmd_t;

typedef struct uart_frame{
	uint32_t len;
	char data[UART_BUFF_SIZE];
}uart_frame_t;

/******************************************************
 *                    Structures
 ******************************************************/
typedef uint8_t dev_id_t[13];

typedef struct {
	struct list_head list;
	wiced_ip_address_t dev_ip;
	dev_id_t dev_id;
}dev_info_t;

typedef struct {
	wiced_bool_t configured;
	//uint8_t dev_index;
	uint8_t dev_type;
	uint8_t dev_name[32];
	dev_id_t dev_id;
	uint8_t wlan_status;
	uint32_t retry_ms;
	wiced_udp_socket_t udp_socket;
	wiced_tcp_socket_t tcp_client_socket;
	uint8_t tcp_conn_state;
	wiced_ip_address_t server_ip;
	wiced_timed_event_t udp_delay_event;
	parse_socket_msg_fun_t parse_socket_msg_fun;
	parse_uart_msg_fun_t parse_uart_msg_fun;
	device_keypad_handler_t device_keypad_handler;
	//uart_receive_handler_t uart_receive_handler;
	wiced_thread_t uart_send_thread;
	wiced_thread_t uart_recv_thread;
	wiced_worker_thread_t wifi_setup_thread;
	wiced_queue_t uart_send_queue;
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
