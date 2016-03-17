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
#if 0
typedef enum {
	RELAY_GPIO_1 = WICED_GPIO_34,
	RELAY_GPIO_2 = WICED_GPIO_35,
	RELAY_GPIO_3 = 101,
	RELAY_GPIO_4 = 102
}relay_channel_t;
#else
typedef enum {
    RELAY_GPIO_1 = WICED_GPIO_34,
    RELAY_GPIO_2 = WICED_GPIO_35,
    RELAY_GPIO_3 = WICED_GPIO_36,
    RELAY_GPIO_4 = WICED_GPIO_37
}relay_gpio_t;
#endif

/******************************************************
 *                 Type Definitions
 ******************************************************/


/******************************************************
 *                    Structures
 ******************************************************/



/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
int reboot();

#ifdef __cplusplus
} /* extern "C" */
#endif
