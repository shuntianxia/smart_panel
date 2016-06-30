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
#include "curtain_dev.h"
#include "wiced_time.h"
#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "wwd_constants.h"
#include "smart_panel_dct.h"
#include "math.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define MIN_POS_MS         (500)

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
static void motor_positive_run();
static void motor_negative_run();
static void motor_stop();
//static void motor_positive_pulse();
//static void motor_negative_pulse();
static void set_curtain_pos_ms(curtain_t *curtain, curtain_pos_ms_t pos_ms);
static wiced_result_t set_pos_step1_event_handler( void* arg );
static wiced_result_t set_pos_step2_event_handler( void* arg );
static wiced_result_t set_pos_stop_event_handler( void* arg );
static void set_pos_force_stop_handler(void* arg);
static wiced_result_t set_pos_start_event_handler( void* arg );
static void set_pos_step1_timer_handler( void* arg );
static void set_pos_step2_timer_handler( void* arg );


/******************************************************
 *               Variable Definitions
 ******************************************************/
//static curtain_t *curtain_p;
static uint32_t motor_state;

/******************************************************
 *               Function Definitions
 ******************************************************/
#if 0
curtain_t * curtain_init_bak(wiced_worker_thread_t* thread, uart_keypad_handler_t function)
{
	smart_panel_app_dct_t*   dct_app;
	curtain_t *curtain;

	curtain = (curtain_t *) malloc_named("curtain", sizeof(curtain_t));
    if ( curtain == NULL )
    {
        return NULL;
    }
	memset( curtain, 0, sizeof(curtain_t) );

	curtain->function   = function;
    curtain->thread     = thread;

	wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *dct_app ) );
	curtain->calibrated = dct_app->specific.curtain_config.calibrated;
	curtain->current_pos_ms = dct_app->specific.curtain_config.current_pos_ms;
	curtain->full_pos_ms = dct_app->specific.curtain_config.full_pos_ms;
	wiced_dct_read_unlock( dct_app, WICED_TRUE );
	
	return curtain;
}
#endif
wiced_result_t curtain_init(curtain_t **curtain_dev, wiced_worker_thread_t* thread, curtain_handler_t function)
{
	smart_panel_app_dct_t*   dct_app;
	curtain_t *curtain;

	curtain = (curtain_t *) malloc_named("curtain", sizeof(curtain_t));
    if ( curtain == NULL ) {
        return WICED_ERROR;
    }
	memset( curtain, 0, sizeof(curtain_t) );

	curtain->function   = function;
    curtain->thread     = thread;

	wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *dct_app ) );
	curtain->calibrated = dct_app->specific.curtain_config.calibrated;
	curtain->current_pos_ms = dct_app->specific.curtain_config.current_pos_ms;
	curtain->full_pos_ms = dct_app->specific.curtain_config.full_pos_ms;
	wiced_dct_read_unlock( dct_app, WICED_TRUE );

	*curtain_dev = curtain;	
	wiced_gpio_init(RELAY_GPIO_1, OUTPUT_PUSH_PULL);
	wiced_gpio_init(RELAY_GPIO_2, OUTPUT_PUSH_PULL);
	//WPRINT_APP_INFO(("RELAY_GPIO_3 = %d\n", wiced_gpio_output_get(RELAY_GPIO_3)));
	//WPRINT_APP_INFO(("RELAY_GPIO_4 = %d\n", wiced_gpio_output_get(RELAY_GPIO_4)));
	return WICED_SUCCESS;
}

void curtain_open(curtain_t *curtain)
{
#if 0
	if(curtain->current_state == CURTAIN_STATE_NONE) {
		if(curtain->calibrated == WICED_FALSE) {
			//motor_positive_run();
			set_curtain_pos_ms(curtain, 120000);
		} else {
			if(curtain->current_pos_ms < curtain->full_pos_ms) {
				set_curtain_pos_ms(curtain, curtain->full_pos_ms);
			} else {
				WPRINT_APP_INFO(("has been fully opened\n"));
			}
		}
	}
#else
	switch(curtain->current_state)
	{
		case CURTAIN_STATE_NONE:
		case CURTAIN_STATE_CLOSING:
			curtain->current_state = CURTAIN_STATE_OPENING;
			motor_positive_run();
			break;

		default:
			break;
	}
#endif
}

void curtain_close(curtain_t *curtain)
{
#if 0
	if(curtain->current_state == CURTAIN_STATE_NONE) {
		if(curtain->calibrated == WICED_FALSE) {
			//motor_negative_run();
			set_curtain_pos_ms(curtain, 0);
		} else {
			if(curtain->current_pos_ms > 0) {
				set_curtain_pos_ms(curtain, 0);
			} else {
				WPRINT_APP_INFO(("has been fully closed\n"));
			}
		}
	}
#else
	switch(curtain->current_state)
	{
		case CURTAIN_STATE_NONE:
		case CURTAIN_STATE_OPENING:
			curtain->current_state = CURTAIN_STATE_CLOSING;
			motor_negative_run();
			break;

		default:
			break;
	}
#endif
}

void curtain_stop(curtain_t *curtain)
{

	curtain_state_t state = curtain->current_state;
#if 0	
	if(state == CURTAIN_STATE_OPENING || state == CURTAIN_STATE_CLOSING) {
		wiced_rtos_send_asynchronous_event(curtain->thread, set_pos_stop_event_handler, NULL);
	} else if( state == CURTAIN_STATE_CALIBRATING) {
		curtain_cali_done(curtain);
	}
#else
	if(state == CURTAIN_STATE_CALIBRATING) {
		curtain_cali_done(curtain);
	} else if(state == CURTAIN_STATE_OPENING || state == CURTAIN_STATE_CLOSING) {
		curtain->current_state = CURTAIN_STATE_NONE;
		motor_stop();
	}
#endif
}

void curtain_cali_enable(curtain_t *curtain)
{
	if(curtain->current_state == CURTAIN_STATE_NONE) {
		curtain->calibrated = WICED_FALSE;
	}
}

void curtain_cali_start(curtain_t *curtain)
{
	if(curtain->current_state == CURTAIN_STATE_NONE) {
		motor_positive_run();
		wiced_time_get_time(&curtain->last_op_timestamp);
		curtain->current_state = CURTAIN_STATE_CALIBRATING;
	}
}

void curtain_cali_done(curtain_t *curtain)
{
	smart_panel_app_dct_t*   dct_app;
	uint32_t current_time;

	if(curtain->current_state == CURTAIN_STATE_CALIBRATING) {
		motor_stop();
		
		wiced_time_get_time(&current_time);
		curtain->full_pos_ms = current_time - curtain->last_op_timestamp;
		//curtain->current_pos_ms = curtain->full_pos_ms;
		curtain->current_state = CURTAIN_STATE_NONE;
		curtain->calibrated = WICED_TRUE;

		WPRINT_APP_INFO(("calibrate full_pos_ms = %d\n", (int)curtain->full_pos_ms));
		
		wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *dct_app ) );
		dct_app->specific.curtain_config.calibrated= curtain->calibrated;
		dct_app->specific.curtain_config.current_pos_ms= curtain->current_pos_ms;
		dct_app->specific.curtain_config.full_pos_ms= curtain->full_pos_ms;
		wiced_dct_write( (const void*) dct_app, DCT_APP_SECTION, 0, sizeof(smart_panel_app_dct_t) );
		wiced_dct_read_unlock( dct_app, WICED_TRUE );
	}
	return;
}

curtain_pos_t get_curtain_pos(curtain_t *curtain)
{
	float value;
	curtain_pos_t pos;
	int rs;
	
	if(curtain->full_pos_ms == 0) {
		rs = 104;
	} else if(curtain->current_state == CURTAIN_STATE_OPENING) {

	} else if(curtain->current_state == CURTAIN_STATE_CLOSING) {

	} else if(curtain->current_state == CURTAIN_STATE_CALIBRATING) {

	} else {
		rs = (100 * curtain->current_pos_ms)/curtain->full_pos_ms;
	}
	
	return pos;
}

int set_curtain_pos(curtain_t *curtain, uint8_t ratio)
{
	curtain_pos_ms_t pos_ms;

	if(ratio > 0x64 && ratio != 0xff) {
		return 1;
	} 
	if(ratio <= 0x64 && curtain->current_state != CURTAIN_STATE_NONE) {
		return 2;
	} 
	if(curtain->calibrated != WICED_TRUE) {
		WPRINT_APP_INFO(("please calibrate first\n"));
		return 3;
	}
	if(ratio == 0xff && curtain->current_state == CURTAIN_STATE_PROGRAMMING) {
		set_pos_force_stop_handler(curtain);
	} else if(ratio <= 0x64 && curtain->current_state == CURTAIN_STATE_NONE) {
		pos_ms = (ratio * curtain->full_pos_ms)/100;
		set_curtain_pos_ms(curtain, pos_ms);
	}
	return 0;
}

static void set_curtain_pos_ms(curtain_t *curtain, curtain_pos_ms_t pos_ms)
{
#if 0
	uint32_t diff_ms;

	diff_ms = abs(pos_ms - curtain->current_pos_ms);

	if(diff_ms < MIN_POS_MS)
		return WICED_SUCCESS;	
	if(pos_ms > curtain->current_pos_ms) {
		//diff_ms = pos_ms - curtain->current_pos_ms;
		curtain->current_state = CURTAIN_STATE_OPENING;
		motor_positive_run();
	} else if(pos_ms < curtain->current_pos_ms) {
		//diff_ms = curtain->current_pos_ms - pos_ms;
		curtain->current_state = CURTAIN_STATE_CLOSING;
		motor_negative_run();
	}
	wiced_rtos_init_timer(&curtain->check_state_timer, diff_ms, curtain_stop_timer_handler, curtain );
	wiced_rtos_send_asynchronous_event( curtain->thread, set_pos_start_event_handler, curtain );
#endif
	curtain->dest_pos_ms = pos_ms;

	if(curtain->dest_pos_ms == 0) {
		wiced_rtos_init_timer(&curtain->set_pos_timer, curtain->full_pos_ms, set_pos_step2_timer_handler, curtain );
	} else {
		wiced_rtos_init_timer(&curtain->set_pos_timer, curtain->dest_pos_ms, set_pos_step2_timer_handler, curtain );
	}
	wiced_rtos_send_asynchronous_event( curtain->thread, set_pos_start_event_handler, curtain );
}

static void set_pos_step1_timer_handler( void* arg )
{
	curtain_t *curtain = (curtain_t *)arg;

	//WPRINT_APP_INFO(("curtain_reopen_timer_handler\n"));
	//if(wiced_rtos_is_timer_running(&curtain->check_state_timer) == WICED_SUCCESS) {
		//wiced_rtos_stop_timer(&curtain->full_close_timer);
	//}
	wiced_rtos_deinit_timer(&curtain->set_pos_timer);

	if(curtain->dest_pos_ms < 100) {
		wiced_rtos_send_asynchronous_event( curtain->thread, set_pos_stop_event_handler, curtain );
	} else if(curtain->dest_pos_ms >= 100 && curtain->dest_pos_ms <= curtain->full_pos_ms) {
		//WPRINT_APP_INFO(("curtain->dest_pos_ms is %d\n", curtain->dest_pos_ms));
		wiced_rtos_init_timer(&curtain->set_pos_timer, curtain->dest_pos_ms, set_pos_step2_timer_handler, curtain );
		wiced_rtos_send_asynchronous_event( curtain->thread, set_pos_step2_event_handler, curtain );
		//wiced_rtos_start_timer(&curtain->check_state_timer);
		//motor_positive_run();
	}
}

static void set_pos_force_stop_handler(void* arg)
{
	curtain_t *curtain = (curtain_t *)arg;
	
	wiced_rtos_stop_timer(&curtain->set_pos_timer);
	wiced_rtos_deinit_timer(&curtain->set_pos_timer);
	wiced_rtos_send_asynchronous_event( curtain->thread, set_pos_stop_event_handler, curtain );
}

static void set_pos_step2_timer_handler( void* arg )
{
	curtain_t *curtain = (curtain_t *)arg;

	wiced_rtos_deinit_timer(&curtain->set_pos_timer);
	wiced_rtos_send_asynchronous_event( curtain->thread, set_pos_stop_event_handler, curtain );
}

static wiced_result_t set_pos_start_event_handler( void* arg )
{
	curtain_t *curtain = (curtain_t *)arg;

	WPRINT_APP_INFO(("set_pos_start_handler\n"));
	//wiced_time_get_time(&curtain->last_op_timestamp);
	wiced_rtos_start_timer(&curtain->set_pos_timer);
	curtain->current_state = CURTAIN_STATE_PROGRAMMING;
	if(curtain->dest_pos_ms == 0) {
		motor_negative_run();
	} else {
		motor_positive_run();
	}

    return WICED_SUCCESS;
}

static wiced_result_t set_pos_step1_event_handler( void* arg )
{
	curtain_t *curtain = (curtain_t *)arg;

	WPRINT_APP_INFO(("set_pos_step1_start_handler\n"));
	//wiced_time_get_time(&curtain->last_op_timestamp);
	wiced_rtos_start_timer(&curtain->set_pos_timer);
	curtain->current_state = CURTAIN_STATE_PROGRAMMING;
	motor_negative_run();

    return WICED_SUCCESS;
}

static wiced_result_t set_pos_step2_event_handler( void* arg )
{
	curtain_t *curtain = (curtain_t *)arg;
#if 1
	//WPRINT_APP_INFO(("set_pos_reopen_event_handler\n"));
	//wiced_time_get_time(&curtain->last_op_timestamp);
	motor_positive_run();
	wiced_rtos_start_timer(&curtain->set_pos_timer);
	//curtain->current_state = CURTAIN_STATE_PROGRAMMING;
#else
	curtain->current_state = CURTAIN_STATE_NONE;
	motor_stop();
#endif
    return WICED_SUCCESS;
}

static wiced_result_t set_pos_stop_event_handler( void* arg )
{
	curtain_t *curtain = (curtain_t *)arg;

	curtain->current_state = CURTAIN_STATE_NONE;
	motor_stop();

    return WICED_SUCCESS;
}
#if 0
static wiced_result_t set_pos_stop_event_handler_bak( void* arg )
{
	smart_panel_app_dct_t*   dct_app;
	curtain_t *curtain = (curtain_t *)arg;
	uint32_t current_time;
	uint32_t time_ms;
	uint32_t new_pos_ms;

	motor_stop();
	wiced_time_get_time( &current_time );
	time_ms = current_time - curtain->last_op_timestamp;
	if(curtain->current_state == CURTAIN_STATE_OPENING) {
		new_pos_ms = curtain->current_pos_ms + time_ms;
		if(new_pos_ms >= curtain->full_pos_ms)
			new_pos_ms = curtain->full_pos_ms;
			curtain->current_pos_ms = new_pos_ms;
	} else if(curtain->current_state == CURTAIN_STATE_CLOSING) {
		new_pos_ms = curtain->current_pos_ms - time_ms;
		if(new_pos_ms <= 0)
			new_pos_ms = 0;
		curtain->current_pos_ms = new_pos_ms;
	}
	curtain->current_state = CURTAIN_STATE_NONE;
	if(wiced_rtos_is_timer_running(&curtain->set_pos_timer) == WICED_SUCCESS) {
		wiced_rtos_stop_timer(&curtain->set_pos_timer);
	}
	wiced_rtos_deinit_timer(&curtain->set_pos_timer);
	curtain->function( curtain );
	
	wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *dct_app ) );
	dct_app->specific.curtain_config.current_pos_ms = curtain->current_pos_ms;
	wiced_dct_read_unlock( dct_app, WICED_TRUE );
	
    return WICED_ERROR;
}
#endif

static void motor_positive_run()
{
	WPRINT_APP_INFO(("motor_positive_run\n"));
/*
	if(motor_state == 0 || motor_state == 2) {
		motor_positive_pulse();
		motor_state = 1;
	}else if(motor_state == 1) {
		motor_positive_pulse();
		motor_state = 0;
	}
*/
	if(motor_state == 2)
		wiced_gpio_output_low(RELAY_GPIO_2);
	wiced_gpio_output_high(RELAY_GPIO_1);
	motor_state = 1;
}

static void motor_negative_run()
{
	WPRINT_APP_INFO(("motor_negative_run\n"));
/*
	if(motor_state == 0 || motor_state == 1) {
		motor_negative_pulse();
		motor_state = 2;
	}else if(motor_state == 2) {
		motor_negative_pulse();
		motor_state = 0;
	}
*/
	if(motor_state == 1)
		wiced_gpio_output_low(RELAY_GPIO_1);
	wiced_gpio_output_high(RELAY_GPIO_2);
	motor_state = 2;
}

static void motor_stop()
{
	WPRINT_APP_INFO(("motor_stop\n"));
/*
	if(motor_state == 1) {
		motor_positive_pulse();
	} else if(motor_state == 2) {
		motor_negative_pulse();
	}
	motor_state = 0;
*/
	wiced_gpio_output_low(RELAY_GPIO_1);
	wiced_gpio_output_low(RELAY_GPIO_2);
	motor_state = 0;
}
#if 0
static void motor_positive_pulse()
{
	wiced_gpio_output_high(RELAY_GPIO_1);
	wiced_rtos_delay_milliseconds(50);
	wiced_gpio_output_low(RELAY_GPIO_1);
}

static void motor_negative_pulse()
{
	wiced_gpio_output_high(RELAY_GPIO_2);
	wiced_rtos_delay_milliseconds(50);
	wiced_gpio_output_low(RELAY_GPIO_2);
}
#endif
