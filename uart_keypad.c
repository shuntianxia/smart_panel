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
#include "uart_keypad.h"
#include "wiced_time.h"
#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "wwd_constants.h"
#include "uart_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define DEBOUNCE_TIME_MS         (150)
#define UART_RECEIVE_STACK_SIZE (6200)
#define RX_BUFFER_SIZE    64

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct uart_key_internal
{
    uart_key_code_t              key_code;
    uint32_t                     last_irq_timestamp;
    struct uart_keypad_internal* owner;
}uart_key_internal_t;

typedef struct uart_keypad_internal
{
    //uint32_t               total_keys;
    //uart_key_code_t*       key_code_list;
    uart_keypad_handler_t  function;
    wiced_worker_thread_t* thread;
	wiced_bool_t long_long_pressed_flag;
    //uart_key_internal_t*   current_key_pressed;
	uart_key_code_t      current_key_code;
    wiced_timer_t          check_state_timer;
}uart_keypad_internal_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/

//static void           gpio_interrupt_key_handler( void* arg );
static void           check_state_timer_handler ( void* arg );
static wiced_result_t key_pressed_event_handler ( void* arg );
static wiced_result_t key_released_event_handler( void* arg );
static wiced_result_t key_long_pressed_event_handler( void* arg );
static wiced_result_t key_long_released_event_handler( void* arg );
static wiced_result_t key_long_long_pressed_event_handler( void* arg );
static wiced_result_t key_long_long_released_event_handler( void* arg );
static void device_process_uart_msg(uint32_t arg);


/******************************************************
 *               Variable Definitions
 ******************************************************/
static wiced_thread_t uart_thread;
static uart_keypad_internal_t* keypad_internal;
static wiced_uart_config_t uart_config =
{
    .baud_rate    = 9600,
    .data_width   = DATA_WIDTH_8BIT,
    .parity       = NO_PARITY,
    .stop_bits    = STOP_BITS_1,
    .flow_control = FLOW_CONTROL_DISABLED,
};

static wiced_ring_buffer_t rx_buffer;
static uint8_t rx_data[RX_BUFFER_SIZE];

/******************************************************
 *               Function Definitions
 ******************************************************/
wiced_result_t uart_receive_enable(wiced_thread_function_t function)
{
	/* Initialise ring buffer */
	ring_buffer_init(&rx_buffer, rx_data, RX_BUFFER_SIZE );

	/* Initialise UART. A ring buffer is used to hold received characters */
	wiced_uart_init( WICED_UART_2, &uart_config, &rx_buffer );

	wiced_rtos_create_thread(&uart_thread, WICED_NETWORK_WORKER_PRIORITY, "uart receive thread", function, UART_RECEIVE_STACK_SIZE, 0);
	return WICED_SUCCESS;
}

wiced_result_t uart_keypad_enable( uart_keypad_t* keypad, wiced_worker_thread_t* thread, uart_keypad_handler_t function, uint32_t held_event_interval_ms)
{
    //uart_key_internal_t* key_internal;
    uint32_t malloc_size;

    if ( !keypad || !thread || !function )
    {
        return WICED_BADARG;
    }

    malloc_size      = sizeof(uart_keypad_internal_t);
    keypad->internal = (uart_keypad_internal_t*) malloc_named("keypad internal", malloc_size);
    if ( !keypad->internal )
    {
        return WICED_ERROR;
    }
	keypad_internal = keypad->internal;
    memset( keypad->internal, 0, sizeof( *( keypad->internal ) ) );

    keypad->internal->function   = function;
    keypad->internal->thread     = thread;

    wiced_rtos_init_timer( &keypad->internal->check_state_timer, held_event_interval_ms, check_state_timer_handler, (void*) keypad->internal );

	uart_receive_enable(device_process_uart_msg);

    return WICED_SUCCESS;
}

wiced_result_t uart_keypad_disable( uart_keypad_t *keypad )
{
    malloc_transfer_to_curr_thread(keypad->internal);

    wiced_rtos_deinit_timer( &keypad->internal->check_state_timer );

    free( keypad->internal );
    keypad->internal = 0;

    malloc_leak_check( NULL, LEAK_CHECK_THREAD );
    return WICED_SUCCESS;
}

static void device_process_uart_msg(uint32_t arg)
{
	static int key_pos = 0;
	uart_keypad_internal_t* keypad = keypad_internal;
	uint8_t c;

    /* Wait for user input. If received, echo it back to the terminal */
    while ( 1 )
    {
		if(wiced_uart_receive_bytes( WICED_UART_2, &c, 1, WICED_NEVER_TIMEOUT ) != WICED_SUCCESS)
			continue;
        key_pos++;
		WPRINT_APP_INFO(("key_pos=%d, RcvChar=0x%02x\n", key_pos, c));
		if(c >= KEY_1 && c <= KEY_6) {
			//if(key_pos != 1)
			key_pos = 1;
			keypad->current_key_code = c;
			//key_code = c;
			//key_event.k_code = key_code;
			//key_event.k_action = SHORT_PRESS_PRESSED;
			//handle_key_event(&key_event);
	        wiced_rtos_send_asynchronous_event( keypad->thread, key_pressed_event_handler, (void*)keypad );
		} else if(c == KEY_RELEASE) {
			if(key_pos == 2) {
				//key_event.k_code = key_code;
				//key_event.k_action = SHORT_PRESS_RELEASED;
				//handle_key_event(&key_event);
				wiced_rtos_send_asynchronous_event( keypad->thread, key_released_event_handler, (void*)keypad );
			} else if (key_pos == 3) {
				//WPRINT_APP_INFO(("key_pos == 3\n"));
				if(keypad->current_key_code >= KEY_1 && keypad->current_key_code <= KEY_6) {
					//WPRINT_APP_INFO(("key_pos == 3  key_code >= KEY_1 && key_code <= KEY_6\n"));
					//key_event.k_code = key_code;
					//key_event.k_action = LONG_PRESS_RELEASED;
					//handle_key_event(&key_event);
					if(keypad->long_long_pressed_flag == WICED_TRUE) {
						wiced_rtos_send_asynchronous_event( keypad->thread, key_long_long_released_event_handler, (void*)keypad );
						keypad->long_long_pressed_flag = WICED_FALSE;
					} else {
						wiced_rtos_send_asynchronous_event( keypad->thread, key_long_released_event_handler, (void*)keypad );
					}
					//if(wiced_rtos_is_timer_running(&uart_timer) == WICED_SUCCESS)
						//wiced_rtos_stop_timer(&uart_timer);
				}
				else if(keypad->current_key_code >= KEY_LEFT_UP && keypad->current_key_code <= KEY_RIGHT_DOWN) {
					//WPRINT_APP_INFO(("key_pos == 3\n  key_code >= KEY_LEFT_UP && key_code <= KEY_RIGHT_DOWN"));
					//key_event.k_code = key_code;
					//key_event.k_action = SHORT_PRESS_RELEASED;
					//handle_key_event(&key_event);
					wiced_rtos_send_asynchronous_event( keypad->thread, key_released_event_handler, (void*)keypad );
				}
			}
			key_pos = 0;
			//keypad->current_key_code = KEY_NO_DEFINE;
		//} else if (c == (key_code | 0x10)) {
		} else if (c == (keypad->current_key_code | 0x10)) {
			if(key_pos == 2) {
				//key_event.k_code = key_code;
				//key_event.k_action = LONG_PRESS_PRESSED;
				//handle_key_event(&key_event);
				wiced_rtos_send_asynchronous_event( keypad->thread, key_long_pressed_event_handler, (void*)keypad );				
			}
		} else if (c >= KEY_LEFT_UP && c <= KEY_RIGHT_DOWN) {
			if(key_pos == 2) {
				keypad->current_key_code = c;
				wiced_rtos_send_asynchronous_event( keypad->thread, key_pressed_event_handler, (void*)keypad );
				//key_code = c;
				//key_event.k_code = key_code;
				//key_event.k_action = SHORT_PRESS_PRESSED;
				//handle_key_event(&key_event);
			}
		} else {
			WPRINT_APP_INFO(("else\n"));
			key_pos = 0;
			//key_code = KEY_NO_DEFINE;
		}
    }
}

#if 0
void device_process_uart_msg()
{
	static int key_pos = 0;
	static key_code_t key_code = KEY_NO_DEFINE;
	static key_event_t key_event;
	uint8_t c;

	wiced_rtos_init_timer(&uart_timer, 3000, uart_timer_handle, &key_event);

    /* Wait for user input. If received, echo it back to the terminal */
    while ( 1 )
    {
		if(wiced_uart_receive_bytes( WICED_UART_2, &c, 1, WICED_NEVER_TIMEOUT ) != WICED_SUCCESS)
			continue;
        key_pos++;
		WPRINT_APP_INFO(("key_pos=%d, RcvChar=0x%02x\n", key_pos, c));
		if(c >= KEY_1 && c <= KEY_6) {
			//if(key_pos != 1)
			key_pos = 1;
			key_code = c;
			key_event.k_code = key_code;
			key_event.k_action = SHORT_PRESS_PRESSED;
			handle_key_event(&key_event);
		} else if(c == KEY_RELEASE) {
			if(key_pos == 2) {
				key_event.k_code = key_code;
				key_event.k_action = SHORT_PRESS_RELEASED;
				handle_key_event(&key_event);
			} else if (key_pos == 3) {
				WPRINT_APP_INFO(("key_pos == 3\n"));
				if(key_code >= KEY_1 && key_code <= KEY_6) {
					WPRINT_APP_INFO(("key_pos == 3  key_code >= KEY_1 && key_code <= KEY_6\n"));
					key_event.k_code = key_code;
					key_event.k_action = LONG_PRESS_RELEASED;
					handle_key_event(&key_event);
					//if(wiced_rtos_is_timer_running(&uart_timer) == WICED_SUCCESS)
						//wiced_rtos_stop_timer(&uart_timer);
				}
				else if(key_code >= KEY_LEFT_UP && key_code <= KEY_RIGHT_DOWN) {
					WPRINT_APP_INFO(("key_pos == 3\n  key_code >= KEY_LEFT_UP && key_code <= KEY_RIGHT_DOWN"));
					key_event.k_code = key_code;
					key_event.k_action = SHORT_PRESS_RELEASED;
					handle_key_event(&key_event);
				}
			}
			key_pos = 0;
			key_code = KEY_NO_DEFINE;
		} else if (c == (key_code | 0x10)) {
			if(key_pos == 2) {
				key_event.k_code = key_code;
				key_event.k_action = LONG_PRESS_PRESSED;
				handle_key_event(&key_event);
				wiced_rtos_send_asynchronous_event( WICED_HARDWARE_IO_WORKER_THREAD, key_pressed_event_handler, NULL );
				//if(wiced_rtos_is_timer_running(&uart_timer) == WICED_ERROR)
					//wiced_rtos_start_timer(&uart_timer);
			}
		} else if (c >= KEY_LEFT_UP && c <= KEY_RIGHT_DOWN) {
			if(key_pos == 2) {
				key_code = c;
				key_event.k_code = key_code;
				key_event.k_action = SHORT_PRESS_PRESSED;
				handle_key_event(&key_event);
			}
		} else {
			WPRINT_APP_INFO(("else\n"));
			key_pos = 0;
			key_code = KEY_NO_DEFINE;
		}
    }
}
#endif

static void check_state_timer_handler( void* arg )
{
    uart_keypad_internal_t* keypad = (uart_keypad_internal_t*)arg;

    if ( keypad != NULL )
    {
        //uart_key_internal_t* key = keypad->current_key_pressed;
		uart_key_code_t key_code = keypad->current_key_code;

        if ( key_code != 0 )
        {
			wiced_rtos_send_asynchronous_event( keypad->thread, key_long_long_pressed_event_handler, (void*)arg );
			keypad->long_long_pressed_flag = WICED_TRUE;
        }
    }
}

static wiced_result_t key_pressed_event_handler( void* arg )
{
    //uart_key_internal_t* key = (uart_key_internal_t*)arg;
	uart_keypad_internal_t* keypad = (uart_keypad_internal_t*)arg;
	//WPRINT_APP_INFO(("key_pressed_event_handler ...\n"));
    if ( keypad != NULL )
    {
        //uart_keypad_internal_t* keypad = key->owner;
        keypad->function( keypad->current_key_code, KEY_EVENT_PRESSED );

        return WICED_SUCCESS;
    }

    return WICED_ERROR;
}

static wiced_result_t key_released_event_handler( void* arg )
{
	uart_keypad_internal_t* keypad = (uart_keypad_internal_t*)arg;
	//WPRINT_APP_INFO(("key_released_event_handler ...\n"));
    if ( keypad != NULL )
    {
		wiced_rtos_stop_timer( &keypad->check_state_timer );
        //keypad->current_key_code = 0;

        //wiced_rtos_stop_timer( &keypad->check_state_timer );

        keypad->function( keypad->current_key_code, KEY_EVENT_RELEASED );

        return WICED_SUCCESS;
    }

    return WICED_ERROR;
}

static wiced_result_t key_long_pressed_event_handler( void* arg )
{
    //uart_key_internal_t* key = (uart_key_internal_t*)arg;
	uart_keypad_internal_t* keypad = (uart_keypad_internal_t*)arg;

    if ( keypad != NULL )
    {
		wiced_rtos_start_timer( &keypad->check_state_timer );
        keypad->function( keypad->current_key_code, KEY_EVENT_LONG_PRESSED );

        return WICED_SUCCESS;

    }

    return WICED_ERROR;
}

static wiced_result_t key_long_released_event_handler( void* arg )
{
    uart_keypad_internal_t* keypad = (uart_keypad_internal_t*)arg;

    if ( keypad != NULL )
    {
		wiced_rtos_stop_timer( &keypad->check_state_timer );
        keypad->function( keypad->current_key_code, KEY_EVENT_LONG_RELEASED );

        return WICED_SUCCESS;
    }

    return WICED_ERROR;
}

static wiced_result_t key_long_long_pressed_event_handler( void* arg )
{
    //uart_key_internal_t* key = (uart_key_internal_t*)arg;
	uart_keypad_internal_t* keypad = (uart_keypad_internal_t*)arg;

    if ( keypad != NULL )
    {
        keypad->function( keypad->current_key_code, KEY_EVENT_LONG_LONG_PRESSED );

        return WICED_SUCCESS;

    }

    return WICED_ERROR;
}

static wiced_result_t key_long_long_released_event_handler( void* arg )
{
    uart_keypad_internal_t* keypad = (uart_keypad_internal_t*)arg;

    if ( keypad != NULL )
    {
		wiced_rtos_stop_timer( &keypad->check_state_timer );
        keypad->function( keypad->current_key_code, KEY_EVENT_LONG_LONG_RELEASED );

        return WICED_SUCCESS;
    }

    return WICED_ERROR;
}

