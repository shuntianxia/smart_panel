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

typedef enum{
	KEY_EVENT_NONE,  /* No event              */
	KEY_EVENT_PRESSED,  /* Key has been pressed  */
	KEY_EVENT_RELEASED,  /* Key has been released */
	KEY_EVENT_LONG_PRESSED,
	KEY_EVENT_LONG_RELEASED,
	KEY_EVENT_LONG_LONG_PRESSED,
	KEY_EVENT_LONG_LONG_RELEASED,
}uart_key_event_t;

typedef enum{
	KEY_NO_DEFINE = 0x00,
	KEY_1 = 0x01,
	KEY_2 = 0x02,
	KEY_3 = 0x03,
	KEY_4 = 0x04,
	KEY_5 = 0x05,
	KEY_6 = 0x06,
	KEY_LEFT_UP = 0x81,
	KEY_LEFT_DOWN = 0x82,
	KEY_RIGHT_UP = 0x83,
	KEY_RIGHT_DOWN = 0x84,
	KEY_RELEASE = 0xfe,
}uart_key_code_t;

typedef enum{
	SHORT_PRESS_PRESSED,
	SHORT_PRESS_RELEASED,
	LONG_PRESS_PRESSED,
	LONG_PRESS_RELEASED,
	LONG_LONG_PRESS_PRESSED,
	LONG_LONG_PRESS_RELEASED,
}key_action_t;

typedef struct key_event{
	uart_key_code_t k_code;
	key_action_t k_action;
}key_event_t;

//typedef uint32_t uart_key_event_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

//typedef uint32_t uart_key_code_t;
typedef void (*uart_keypad_handler_t)(uart_key_code_t code, uart_key_event_t event);

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct uart_keypad
{
    /* Private members. Internal use only */
    struct uart_keypad_internal* internal;
}uart_keypad_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t uart_keypad_enable ( uart_keypad_t* keypad, wiced_worker_thread_t* thread, uart_keypad_handler_t function, uint32_t held_key_interval_ms);
wiced_result_t uart_keypad_disable( uart_keypad_t *keypad );
void uart_interrupt_key_handler();

#ifdef __cplusplus
} /* extern "C" */
#endif
