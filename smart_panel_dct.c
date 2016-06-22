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
	.specific.light_config.light_status = 0x00,
};

/******************************************************
 *               Function Definitions
 ******************************************************/
