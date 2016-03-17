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
#include "wwd_constants.h"
#include <wiced_utilities.h>
#include <resources.h>
#include <wiced_framework.h>
#include "smart_home_dct.h"
#include "comm.h"
#include "device_config_content.h"

/******************************************************
 *                      Macros
 ******************************************************/


/******************************************************
 *                    Constants
 ******************************************************/
#define SCRIPT_JSON_BEGIN	"var jsonObj = { "
#define SCRIPT_JSON_PT1		"\n ap_ssid: \""
#define SCRIPT_JSON_PT2		"\",\n ap_passwd: \""
#define SCRIPT_JSON_PT3		"\",\n hidden_ssid: "

#define SCRIPT_JSON_PT6		",\n sta_ssid: \""
#define SCRIPT_JSON_PT7		"\",\n sta_passwd: \""

#define SCRIPT_JSON_PT4		"\",\n dev_type: \""
#define SCRIPT_JSON_PT5		"\",\n dev_index: \""
#define SCRIPT_JSON_PT8		"\",\n dev_name: \""
#define SCRIPT_JSON_END		"\"\n };"

#define CAPTIVE_PORTAL_REDIRECT_PAGE \
    "<html><head>" \
    "<meta http-equiv=\"refresh\" content=\"0; url=/apps/smart_home/device_config.html\">" \
    "</head></html>"

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

static int32_t process_app_settings_page( const char* url_parameters, wiced_tcp_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
static int32_t process_config_save( const char* url_parameters, wiced_tcp_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
static int32_t process_reboot( const char* url_parameters, wiced_tcp_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
static int url_para_get_int(const char *url, const char *name, int *value);
static int url_para_get_str(const char *url, const char *name, char *str_buf, int buf_size);

/******************************************************
 *               Variable Definitions
 ******************************************************/

/**
 * URL Handler List
 */
START_OF_HTTP_PAGE_DATABASE(device_config_http_page_database)
    ROOT_HTTP_PAGE_REDIRECT("/config/device_config.html"),
    { "/config/device_config.html",      "text/html",                         WICED_DYNAMIC_URL_CONTENT,    .url_content.dynamic_data   = {process_app_settings_page,     0 } },
    { "/scripts/general_ajax_script.js", "application/javascript",            WICED_RESOURCE_URL_CONTENT,   .url_content.resource_data  = &resources_scripts_DIR_general_ajax_script_js,},
    { "/images/brcmlogo.png",            "image/png",                         WICED_RESOURCE_URL_CONTENT,   .url_content.resource_data  = &resources_images_DIR_brcmlogo_png,           },
    { "/images/brcmlogo_line.png",       "image/png",                         WICED_RESOURCE_URL_CONTENT,   .url_content.resource_data  = &resources_images_DIR_brcmlogo_line_png,      },
    { "/config_save",                    "text/html",                         WICED_DYNAMIC_URL_CONTENT,    .url_content.dynamic_data   = {process_config_save,           0 }          },
	{ "/reboot",						 "text/html",						  WICED_DYNAMIC_URL_CONTENT,	.url_content.dynamic_data	= {process_reboot, 			  0 }		   },
	{ "/images/favicon.ico",			 "image/vnd.microsoft.icon",          WICED_RESOURCE_URL_CONTENT,  .url_content.resource_data  = &resources_images_DIR_favicon_ico, },
	{ "/styles/buttons.css",			 "text/css",				          WICED_RESOURCE_URL_CONTENT,  .url_content.resource_data  = &resources_styles_DIR_buttons_css, },
    { IOS_CAPTIVE_PORTAL_ADDRESS,        "text/html",                         WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {CAPTIVE_PORTAL_REDIRECT_PAGE, sizeof(CAPTIVE_PORTAL_REDIRECT_PAGE) } },
    /* Add more pages here */
END_OF_HTTP_PAGE_DATABASE();

extern wiced_http_server_t*         http_server;

/******************************************************
 *               Function Definitions
 ******************************************************/

static int32_t process_app_settings_page( const char* url_parameters, wiced_tcp_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body )
{
    char                  temp_buf[11];
    uint8_t               string_size;
    platform_dct_wifi_config_t* dct_wifi_config          = NULL;
    smart_home_app_dct_t*   dct_app                  = NULL;

    UNUSED_PARAMETER( url_parameters );
    UNUSED_PARAMETER( arg );
    UNUSED_PARAMETER( http_message_body );

    wiced_tcp_stream_write_resource( stream, &resources_apps_DIR_smart_home_DIR_device_config_html );

    /* Output javascript to fill the table entry */
	
    wiced_tcp_stream_write( stream, SCRIPT_JSON_BEGIN, sizeof(SCRIPT_JSON_BEGIN)-1 );
	
	wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof( *dct_wifi_config ) );
	
	wiced_tcp_stream_write( stream, SCRIPT_JSON_PT1, sizeof(SCRIPT_JSON_PT1)-1 );
    wiced_tcp_stream_write( stream, dct_wifi_config->soft_ap_settings.SSID.value, (uint16_t) dct_wifi_config->soft_ap_settings.SSID.length);
	
    wiced_tcp_stream_write( stream, SCRIPT_JSON_PT2, sizeof(SCRIPT_JSON_PT2)-1 );
	wiced_tcp_stream_write( stream, dct_wifi_config->soft_ap_settings.security_key, (uint16_t) dct_wifi_config->soft_ap_settings.security_key_length);

	wiced_tcp_stream_write( stream, SCRIPT_JSON_PT3, sizeof(SCRIPT_JSON_PT3)-1 );
	if(dct_wifi_config->soft_ap_settings.ssid_hide == WICED_TRUE)
		wiced_tcp_stream_write( stream, "true", 4 );
	else
		wiced_tcp_stream_write( stream, "false", 5 );

	wiced_tcp_stream_write( stream, SCRIPT_JSON_PT6, sizeof(SCRIPT_JSON_PT6)-1 );
    wiced_tcp_stream_write( stream, dct_wifi_config->stored_ap_list[0].details.SSID.value, (uint16_t) dct_wifi_config->stored_ap_list[0].details.SSID.length);
	
    wiced_tcp_stream_write( stream, SCRIPT_JSON_PT7, sizeof(SCRIPT_JSON_PT7)-1 );
	wiced_tcp_stream_write( stream, dct_wifi_config->stored_ap_list[0].security_key, (uint16_t) dct_wifi_config->stored_ap_list[0].security_key_length);
	
	wiced_dct_read_unlock( dct_wifi_config, WICED_TRUE );
	
	wiced_tcp_stream_write( stream, SCRIPT_JSON_PT4, sizeof(SCRIPT_JSON_PT4)-1 );
	
	wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *dct_app ) );
	memset(temp_buf, ' ', 3);
    string_size = unsigned_to_decimal_string(dct_app->dev_type, (char*)temp_buf, 1, 3);
    wiced_tcp_stream_write(stream, temp_buf, (uint16_t) string_size);
	
	wiced_tcp_stream_write( stream, SCRIPT_JSON_PT5, sizeof(SCRIPT_JSON_PT5)-1 );
	
    string_size = unsigned_to_decimal_string(dct_app->dev_index, (char*)temp_buf, 1, 3);
    wiced_tcp_stream_write(stream, temp_buf, (uint16_t) string_size);

	wiced_tcp_stream_write( stream, SCRIPT_JSON_PT8, sizeof(SCRIPT_JSON_PT8)-1 );
	wiced_tcp_stream_write( stream, dct_app->dev_name, (uint16_t) strlen(dct_app->dev_name));

	wiced_dct_read_unlock( dct_app, WICED_TRUE );
	
	wiced_tcp_stream_write( stream, SCRIPT_JSON_END, sizeof(SCRIPT_JSON_END)-1 );

    wiced_tcp_stream_write_resource( stream, &resources_apps_DIR_smart_home_DIR_device_config_html_dev_settings_bottom );

    return 0;
}

static int32_t process_config_save( const char* url_parameters, wiced_tcp_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body )
{
    UNUSED_PARAMETER( arg );
    UNUSED_PARAMETER( http_message_body );
	platform_dct_wifi_config_t* dct_wifi_config = NULL;
    smart_home_app_dct_t*   dct_app = NULL;
	char tmp_buf[256];
	int tmp_value;
	int res;
	
	WPRINT_APP_INFO(("%s\n", url_parameters));

    /* get the App config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
    wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *dct_app ) );

	res = url_para_get_int(url_parameters, "dev_type", &tmp_value);
	if(res == 0) {
		dct_app->dev_type = (uint8_t)tmp_value;
		//this_dev.dev_type = dct_app->dev_type;
	}

	res = url_para_get_int(url_parameters, "dev_index", &tmp_value);
	if(res == 0) {
		dct_app->dev_index = (uint8_t)tmp_value;
		//this_dev.dev_index = dct_app->dev_index;
	}
	
	res = url_para_get_str(url_parameters, "dev_name", tmp_buf, sizeof(tmp_buf));
	if(res == 0) {
		memcpy(dct_app->dev_name, tmp_buf, sizeof(dct_app->dev_name));
		//strncpy( dct_app->dev_name, tmp_buf, sizeof(dct_app->dev_name));
	}
	
	dct_app->device_configured = WICED_TRUE;

    wiced_dct_write( (const void*) dct_app, DCT_APP_SECTION, 0, sizeof(smart_home_app_dct_t) );

    /* release the read lock */
    wiced_dct_read_unlock( dct_app, WICED_TRUE );


	/* get the wi-fi config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
    wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof( *dct_wifi_config ) );

	res = url_para_get_str(url_parameters, "ap_ssid", tmp_buf, sizeof(tmp_buf));
	if(res == 0) {
		memcpy(dct_wifi_config->soft_ap_settings.SSID.value, tmp_buf, strlen(tmp_buf) + 1);
		dct_wifi_config->soft_ap_settings.SSID.length = strlen(tmp_buf);
	}

	res = url_para_get_str(url_parameters, "ap_passwd", tmp_buf, sizeof(tmp_buf));
	if(res == 0) {
		memcpy(dct_wifi_config->soft_ap_settings.security_key, tmp_buf, strlen(tmp_buf) + 1);
		dct_wifi_config->soft_ap_settings.security_key_length = strlen(tmp_buf);
	}

	res = url_para_get_int(url_parameters, "hidden_ssid", &tmp_value);
	if(res == 0) {
		dct_wifi_config->soft_ap_settings.ssid_hide= (uint8_t)tmp_value;
	}

	res = url_para_get_str(url_parameters, "sta_ssid", tmp_buf, sizeof(tmp_buf));
	if(res == 0) {
		memcpy(dct_wifi_config->stored_ap_list[0].details.SSID.value, tmp_buf, strlen(tmp_buf) + 1);
		dct_wifi_config->stored_ap_list[0].details.SSID.length = strlen(tmp_buf);
		dct_wifi_config->stored_ap_list[0].details.SSID.value[dct_wifi_config->stored_ap_list[0].details.SSID.length] = 0;
	}

	res = url_para_get_str(url_parameters, "sta_passwd", tmp_buf, sizeof(tmp_buf));
	if(res == 0) {
		memcpy(dct_wifi_config->stored_ap_list[0].security_key, tmp_buf, strlen(tmp_buf) + 1);
		dct_wifi_config->stored_ap_list[0].security_key_length = strlen(tmp_buf);
		dct_wifi_config->stored_ap_list[0].security_key[dct_wifi_config->stored_ap_list[0].security_key_length] = 0;
	}
	
	wiced_dct_write( (const void*) dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );

    /* release the read lock */
    wiced_dct_read_unlock( dct_wifi_config, WICED_TRUE );

    #define CONFIG_SAVE_SUCCESS  "Config saved"
    wiced_tcp_stream_write(stream, CONFIG_SAVE_SUCCESS, sizeof(CONFIG_SAVE_SUCCESS)-1);

	/* Config has been set. Turn off HTTP server */
    //wiced_http_server_stop(http_server);
    return 0;
}

static int32_t process_reboot( const char* url_parameters, wiced_tcp_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body )
{
    UNUSED_PARAMETER( url_parameters );
    UNUSED_PARAMETER( arg );
    UNUSED_PARAMETER( http_message_body );

	//#define DEVICE_REBOOTING  "Device rebooting..."
    //wiced_tcp_stream_write(stream, DEVICE_REBOOTING, sizeof(DEVICE_REBOOTING)-1);
	
    /* Config has been set. Turn off HTTP server */
    wiced_http_server_stop(http_server);

	reboot();
	WPRINT_APP_INFO(("after_reboot\n"));

    return 0;
}

static int url_para_get_int(const char *url_str, const char *name, int *value)
{
	char *end_of_value;
	const char *url = url_str;
	char *ptr = NULL;
	while((ptr = strstr(url, name)) != NULL) {
		if(*(ptr + strlen(name)) != '=' || (*(ptr -1 ) != '&' && ptr != url_str)) {
			url += strlen(name);
			continue;
		}
		end_of_value = ptr + strlen(name) + 1;
        while ( ( *end_of_value != '&' ) && ( *end_of_value != '\n' ) )
        {
            ++end_of_value;
        }
		if(end_of_value != ptr + strlen(name) +1) {
			*value = atoi(ptr + strlen(name) +1);
			return 0;
		} else {
			return -1;
		}
	}
	return -1;
}

static int url_para_get_str(const char *url_str, const char *name, char *str_buf, int buf_size)
{
	int value_len = 0;
	char *end_of_value;
	const char *url = url_str;
	char *ptr = NULL;
	while((ptr = strstr(url, name)) != NULL) {
		if(*(ptr + strlen(name)) != '=' || (*(ptr -1 ) != '&' && ptr != url_str)) {
			url += strlen(name);
			continue;
		}
		end_of_value = ptr + strlen(name) + 1;
        while ( ( *end_of_value != '&' ) && ( *end_of_value != '\n' ) )
        {
            ++end_of_value;
        }
		value_len = end_of_value - (ptr + strlen(name) + 1);
		memset(str_buf, 0, buf_size);
		memcpy( str_buf, ptr + strlen(name) + 1, value_len);
		return 0;
	}
	return -1;
}
