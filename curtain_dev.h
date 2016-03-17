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
#define CURTAIN_KEY_OPEN KEY_1
#define CURTAIN_KEY_STOP KEY_2
#define CURTAIN_KEY_CLOSE KEY_3

#define CURTAIN_GPIO_OPEN	RELAY_GPIO_1
#define CURTAIN_GPIO_CLOSE	RELAY_GPIO_2
//#define CURTAIN_GPIO_STOP	RELAY_GPIO_1

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum {
	CURTAIN_FUN_SET_POS = 0x21,
	CURTAIN_FUN_GET_POS = 0x22,
	CURTAIN_FUN_REPORT_POS = 0x23,
}curtain_fun_t;

typedef enum {
	CURTAIN_STATE_NONE = 0x00,
	CURTAIN_STATE_OPENING = 0x01,
	CURTAIN_STATE_CLOSING = 0x02,
	CURTAIN_STATE_PROGRAMMING = 0x03,
	CURTAIN_STATE_CALIBRATING = 0x04,
}curtain_state_t;

typedef uint32_t curtain_pos_ms_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef void (*curtain_handler_t)(void *arg);
typedef uint8_t curtain_pos_t;
/******************************************************
 *                    Structures
 ******************************************************/
typedef struct curtain {
	curtain_state_t current_state;
	curtain_pos_ms_t current_pos_ms;
	curtain_pos_ms_t dest_pos_ms;
	curtain_pos_ms_t full_pos_ms;
	wiced_bool_t calibrated;
	wiced_timer_t set_pos_timer;
	uint32_t last_op_timestamp;
	curtain_handler_t  function;
	//void (*function)(curtain_t *curtain);
    wiced_worker_thread_t* thread;
}curtain_t;

typedef struct curtain_dev {
	uint32_t curtain_count;
	curtain_t curtain[];
}curtain_dev_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
wiced_result_t curtain_init(curtain_t **curtain_dev, wiced_worker_thread_t* thread, curtain_handler_t function);
void curtain_open(curtain_t *curtain);
void curtain_close(curtain_t *curtain);
void curtain_stop(curtain_t *curtain);
void curtain_cali_enable(curtain_t *curtain);
void curtain_cali_start(curtain_t *curtain);
void curtain_cali_done(curtain_t *curtain);
int set_curtain_pos(curtain_t *curtain, uint8_t pos);
curtain_pos_t get_curtain_pos(curtain_t *curtain);

#ifdef __cplusplus
} /* extern "C" */
#endif
