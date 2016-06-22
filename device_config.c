/*
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

#include <stdlib.h>
#include <string.h>
#include "wiced.h"
#include "http_server.h"
#include "dns_redirect.h"
#include <wiced_utilities.h>
#include "wiced_network.h"
#include <resources.h>
#include "wiced_framework.h"
#include "smart_panel_dct.h"
#include "device_config.h"
#include "device_config_content.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define HTTPS_PORT   443
#define HTTP_PORT    80

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

/******************************************************
 *               Variable Definitions
 ******************************************************/

static const wiced_ip_setting_t device_init_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  1,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  1,  1 ) ),
};

extern const wiced_http_page_t device_config_http_page_database[];
extern const wiced_http_page_t device_config_https_page_database[];

wiced_http_server_t*         http_server;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t configure_device()
{
    wiced_bool_t* device_configured;

    if ( WICED_SUCCESS != wiced_dct_read_lock( (void**) &device_configured, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, OFFSETOF(smart_panel_app_dct_t, device_configured), sizeof(wiced_bool_t) ) )
    {
        return WICED_ERROR;
    }
    /* Prepare the HTTP server */
    http_server = MALLOC_OBJECT("http server", wiced_http_server_t);
    memset( http_server, 0, sizeof(wiced_http_server_t) );

    /* Bring up the softAP interface ------------------------------------------------------------- */
    wiced_network_up(WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &device_init_ip_settings);

    /* Start the HTTP server */
    wiced_http_server_start( http_server, HTTP_PORT, device_config_http_page_database, WICED_AP_INTERFACE );
#if 0
	if ( *device_configured != WICED_TRUE )
	{
        /* Wait for configuration to complete */
        wiced_rtos_thread_join( &http_server->thread );

        /* Cleanup HTTP server */
        wiced_http_server_stop(http_server);
        free( http_server );
	}
#endif
    wiced_dct_read_unlock( device_configured, WICED_FALSE );

    return WICED_SUCCESS;
}

