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
#include "light_dev.h"
#include "smart_home_dct.h"
#include "wiced_time.h"
#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "wwd_constants.h"

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

/******************************************************
 *                    Structures
 ******************************************************/



/******************************************************
 *               Static Function Declarations
 ******************************************************/
static void set_light_status_internal(light_t *light, light_status_t status);
static light_status_t get_light_status_internal(light_t *light);

/******************************************************
 *               Variable Definitions
 ******************************************************/
static light_t light_list[] =
{
	{
		.light_no = 1,
		.key_code = KEY_1,
		.relay_io = RELAY_GPIO_1,
	},
	{
		.light_no = 2,
		.key_code = KEY_2,
		.relay_io = RELAY_GPIO_2,
	},
	{
		.light_no = 3,
		.key_code = KEY_3,
		.relay_io = RELAY_GPIO_3,
	},
	{
		.light_no = 4,
		.key_code = KEY_4,
		.relay_io = RELAY_GPIO_4,
	},
};


/******************************************************
 *               Function Definitions
 ******************************************************/
wiced_result_t light_dev_init(light_dev_t **light_dev_arg, wiced_worker_thread_t* thread, light_handler_t function)
{
	smart_home_app_dct_t*   dct_app;
	light_dev_t *light_dev;
	uint8_t light_status;
	int i;

	light_dev = (light_dev_t *) malloc_named("light_dev", sizeof(light_dev_t));
    if ( light_dev == NULL ) {
        return WICED_ERROR;
    }
	
	memset( light_dev, 0, sizeof(light_dev_t) );
	light_dev->light_list = light_list;
	light_dev->light_count = sizeof(light_list)/sizeof(light_t);
	light_dev->function   = function;
    light_dev->thread     = thread;

	wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *dct_app ) );
	light_status = dct_app->specific.light_config.light_status;
	wiced_dct_read_unlock( dct_app, WICED_TRUE );

	for(i = 0; i < sizeof(light_list)/sizeof(light_t); i++) {
		light_list[i].owner = light_dev;
		light_list[i].status = light_status & (0x01 << i);
		wiced_gpio_init(light_list[i].relay_io, OUTPUT_PUSH_PULL);
		set_light_status_internal(&light_list[i], light_list[i].status);
	}
	*light_dev_arg = light_dev;
	
	return WICED_SUCCESS;
}

void switch_light_status(light_t *light)
{
	wiced_bool_t gpio_status;
	light_dev_t *light_dev = light->owner;

	light_status_t status = light->status;

	if(status == LIGHT_STATUS_ON) {
		set_light_status_internal(light, LIGHT_STATUS_OFF);
	} else if(status == LIGHT_STATUS_OFF) {
		set_light_status_internal(light, LIGHT_STATUS_ON);
	}
	light_dev->function(light);
	return;
}

static void set_light_status_internal(light_t *light, light_status_t status)
{
	light->status = status;
	if(status == LIGHT_STATUS_ON) {
		wiced_gpio_output_high( light->relay_io);
		WPRINT_APP_INFO(("light_%d is on\n", light->light_no));
	}
	else if(status == LIGHT_STATUS_OFF) {
		wiced_gpio_output_low( light->relay_io );
		WPRINT_APP_INFO(("light_%d is off\n", light->light_no));
	}
	return;
}
# if 0
static light_status_t get_light_status_internal(light_t *light)
{
	light_status_t status;
	wiced_bool_t gpio_status;
	gpio_status = wiced_gpio_output_get( light->relay_io );
	if(gpio_status == WICED_TRUE)
		status = LIGHT_STATUS_ON;
	else
		status = LIGHT_STATUS_OFF;
	return status;
}
#endif
int set_light_status(uint8_t light_no, light_status_t status)
{
	int i;
	
	for(i = 0; i < sizeof(light_list)/sizeof(light_t); i++) {
		if(light_no == light_list[i].light_no) {
			set_light_status_internal(&light_list[i], status);
			return 0;
		}
	}
	return 1;
}

light_status_t get_light_status(uint8_t light_no)
{
	int i;
	light_status_t light_status = LIGHT_STATUS_UNKNOWN;

	for(i = 0; i < sizeof(light_list)/sizeof(light_t); i++) {
		if(light_no == light_list[i].light_no) {
			light_status = light_list[i].status;
		}
	}
	return light_status;
}

void get_lights_status(uint8_t *light_count, uint8_t *lights_status)
{
	int i;
	//light_status_t light_status = LIGHT_STATUS_UNKNOWN;
	
	*light_count = 7;
	for(i = 0; i < sizeof(light_list)/sizeof(light_t); i++) {
		if(light_list[i].light_no >0 && light_list[i].light_no < 8) {
			*lights_status |= (light_list[i].status & 0x01) << (light_list[i].light_no - 1);
		}
	}
}
