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

#include <stdint.h>
#include "wiced.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/
enum light_status{
	DEV_LIGHT_ON = 0x0,
	DEV_LIGHT_OFF = 0x1,
};

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/
typedef struct {
	uint8_t light_status;
} light_config_t;

typedef struct {
	uint8_t calibrated;
	uint32_t	current_pos_ms;
	uint32_t	full_pos_ms;
} curtain_config_t;

typedef struct {
	uint8_t		device_configured;
	uint8_t		dev_type;
	uint8_t		dev_index;
	uint8_t		dev_name[32];
	union {
		light_config_t light_config;
		curtain_config_t curtain_config;
	} specific;
} smart_panel_app_dct_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
wiced_result_t load_app_data(smart_panel_app_dct_t* app_dct);
void store_app_data(smart_panel_app_dct_t* app_dct);
wiced_result_t load_wifi_data(platform_dct_wifi_config_t* wifi_dct);
void store_wifi_data(platform_dct_wifi_config_t* wifi_dct);

#ifdef __cplusplus
} /*extern "C" */
#endif
