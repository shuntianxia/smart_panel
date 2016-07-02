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
#include "smart_panel_dct.h"

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
enum dev_type{
	DEV_TYPE_MASTER = 0x00,
	DEV_TYPE_LIGHT = 0x01,
	DEV_TYPE_CURTAIN = 0x02,
};

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef wiced_result_t (*parse_socket_msg_fun_t)(char *, uint16_t);

//typedef wiced_result_t (*parse_uart_msg_fun_t)();

typedef void (*device_keypad_handler_t)(uart_key_code_t, uart_key_event_t);


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
	device_keypad_handler_t device_keypad_handler;
	wiced_worker_thread_t wifi_setup_thread;
	union {
		light_dev_t *light_dev;
		curtain_t *curtain;
	} specific;
	smart_panel_app_dct_t app_dct;
	platform_dct_wifi_config_t wifi_dct;
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
