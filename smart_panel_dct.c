/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "smart_panel_dct.h"
#include "wiced_framework.h"

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
	//DEV_TYPE_UNKNOWN = 0x00,
	DEV_TYPE_MASTER = 0x00,
	DEV_TYPE_LIGHT = 0x01,
	DEV_TYPE_CURTAIN = 0x02,
};

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

DEFINE_APP_DCT(smart_panel_app_dct_t)
{
	.device_configured = 0x00,
    //.dev_type = DEV_TYPE_UNKNOWN,
	//.dev_name_length = 4,
	.dev_name = "Unnamed device",
	//.specific.light_config.light_status = 0x00,
	.specific.curtain_config.calibrated = 0x01,
	.specific.curtain_config.full_pos_ms = 10000,
};

/******************************************************
 *               Function Definitions
 ******************************************************/
wiced_result_t load_app_data(smart_panel_app_dct_t* app_dct)
{
	smart_panel_app_dct_t* dct_app;
	if( wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *dct_app ) ) != WICED_SUCCESS)
	{
		return WICED_ERROR;
	}
	*app_dct = *dct_app;
	wiced_dct_read_unlock( dct_app, WICED_TRUE );
	return WICED_SUCCESS;
}

void store_app_data(smart_panel_app_dct_t* app_dct)
{
	wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(smart_panel_app_dct_t) );
}

wiced_result_t load_wifi_data(platform_dct_wifi_config_t* wifi_dct)
{
	platform_dct_wifi_config_t* dct_wifi_config;
	/* get the wi-fi config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
	wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof( *dct_wifi_config ) );

	*wifi_dct = *dct_wifi_config;

	/* release the read lock */
	wiced_dct_read_unlock( dct_wifi_config, WICED_TRUE );
	return WICED_SUCCESS;
}

void store_wifi_data(platform_dct_wifi_config_t* wifi_dct)
{
	wiced_dct_write( (const void*) wifi_dct, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
}

