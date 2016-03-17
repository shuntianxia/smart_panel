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
#include "uart_keypad.h"
#include "comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define MAX_LIGHT_COUNT 4

#define SETPOINT_UP_KEY_CODE          (1)                           /* Set Point UP key code  */
#define SETPOINT_DOWN_KEY_CODE        (2)                           /* Set Point DOWN key code */

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum {
	LIGHT_FUN_SET_STATE = 0x11,
	LIGHT_FUN_GET_STATE = 0x12,
	LIGHT_FUN_REPORT_STATE = 0x13,
}light_fun_t;

typedef enum {
	LIGHT_STATUS_OFF = 0x00,
	LIGHT_STATUS_ON = 0x01,
	LIGHT_STATUS_UNKNOWN = 0x02,
}light_status_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef void (*light_handler_t)(void *arg);

/******************************************************
 *                    Structures
 ******************************************************/
typedef struct light {
	uint8_t light_no;
	uart_key_code_t key_code;
	relay_gpio_t relay_io;
	light_status_t status;
	struct light_dev *owner;
}light_t;

typedef struct light_dev {
	uint32_t light_count;
	//light_t light_list[MAX_LIGHT_COUNT];
	light_t *light_list;
	light_handler_t  function;
	//void (*function)(void);
	wiced_worker_thread_t* thread;
}light_dev_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
wiced_result_t light_dev_init(light_dev_t **light_dev_arg, wiced_worker_thread_t* thread, light_handler_t function);
int set_light_status(uint8_t light_no, light_status_t status);
void switch_light_status(light_t *light);
light_status_t get_light_status(uint8_t light_no);
void get_lights_status(uint8_t *light_count, uint8_t *lights_status);

#ifdef __cplusplus
} /* extern "C" */
#endif
