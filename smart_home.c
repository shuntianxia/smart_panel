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
 * WICED Configuration Mode Application
 *
 * This application demonstrates how to use WICED Configuration Mode
 * to automatically configure application parameters and Wi-Fi settings
 * via a softAP and webserver
 *
 * Features demonstrated
 *  - WICED Configuration Mode
 *
 * Application Instructions
 *   1. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *   2. After the download completes, the terminal displays WICED startup
 *      information and starts WICED configuration mode.
 *
 * In configuration mode, application and Wi-Fi configuration information
 * is entered via webpages using a Wi-Fi client (eg. your computer)
 *
 * Use your computer to step through device configuration using WICED Config Mode
 *   - Connect the computer using Wi-Fi to the config softAP "WICED Config"
 *     The config AP name & passphrase is defined in the file <WICED-SDK>/include/default_wifi_config_dct.h
 *     The AP name/passphrase is : Wiced Config / 12345678
 *   - Open a web browser and type wiced.com in the URL
 *     (or enter 192.168.0.1 which is the IP address of the softAP interface)
 *   - The Application configuration webpage appears. This page enables
 *     users to enter application specific information such as contact
 *     name and address details for device registration
 *   - Change one of more of the fields in the form and then click 'Save settings'
 *   - Click the Wi-Fi Setup button
 *   - The Wi-Fi configuration page appears. This page provides several options
 *     for configuring the device to connect to a Wi-Fi network.
 *   - Click 'Scan and select network'. The device scans for Wi-Fi networks in
 *     range and provides a webpage with a list.
 *   - Enter the password for your Wi-Fi AP in the Password box (top left)
 *   - Find your Wi-Fi AP in the list, and click the 'Join' button next to it
 *
 * Configuration mode is complete. The device stops the softAP and webserver,
 * and attempts to join the Wi-Fi AP specified during configuration. Once the
 * device completes association, application configuration information is
 * printed to the terminal
 *
 * The wiced.com URL reference in the above text is configured in the DNS
 * redirect server. To change the URL, edit the list in
 * <WICED-SDK>/Library/daemons/dns_redirect.c
 * URLs currently configured are:
 *      # http://www.broadcom.com , http://broadcom.com ,
 *      # http://www.facebook.com , http://facebook.com ,
 *      # http://www.google.com   , http://google.com   ,
 *      # http://www.bing.com     , http://bing.com     ,
 *      # http://www.apple.com    , http://apple.com    ,
 *      # http://www.wiced.com    , http://wiced.com    ,
 *
 *  *** IMPORTANT NOTE ***
 *   The config mode API will be integrated into Wi-Fi Easy Setup when
 *   WICED-SDK-3.0.0 is released.
 *
 */

#include "wiced.h"
#include "smart_home.h"
#include "smart_home_dct.h"
//#include "uart_master_ctrl.h"
#include "uart_interface.h"
#include "net_interface.h"
#include "uart_keypad.h"
#include "light_dev.h"
#include "curtain_dev.h"
#include "device_config.h"

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
static void light_keypad_handler( uart_key_code_t code, uart_key_event_t event );
static void curtain_keypad_handler( uart_key_code_t code, uart_key_event_t event );
static wiced_result_t device_init();
static void report_curtain_pos(void *arg);
static void report_light_status(void *arg);

/******************************************************
 *               Variable Definitions
 ******************************************************/
#if 0
static const configuration_entry_t app_config[] =
{
    {"configured", DCT_OFFSET(smart_home_app_dct_t, device_configured),  4, CONFIG_UINT8_DATA },
	{"device type", DCT_OFFSET(smart_home_app_dct_t, dev_type),  4, CONFIG_UINT8_DATA },
	{"device index", DCT_OFFSET(smart_home_app_dct_t, dev_index),	 4, CONFIG_UINT8_DATA },
    {"device name", DCT_OFFSET(smart_home_app_dct_t, dev_name),	 32, CONFIG_STRING_DATA },
	{"light status", DCT_OFFSET(smart_home_app_dct_t, specific.light_config.light_status),   4, CONFIG_UINT8_DATA },
    {"calibrated", DCT_OFFSET(smart_home_app_dct_t, specific.curtain_config.calibrated),  4, CONFIG_UINT8_DATA },
	{"curr_pos_ms", DCT_OFFSET(smart_home_app_dct_t, specific.curtain_config.current_pos_ms),  4, CONFIG_UINT32_DATA },
	{"full_pos_ms", DCT_OFFSET(smart_home_app_dct_t, specific.curtain_config.full_pos_ms),  4, CONFIG_UINT32_DATA },
    {0,0,0,0}
};
#endif

static uart_keypad_t device_keypad;
glob_info_t this_dev;
//static wiced_semaphore_t link_up_semaphore;
static const msg_type_t msg_bst_control = {0x42, 0x53, 0x54};

/******************************************************
 *               Function Definitions
 ******************************************************/
void application_start( )
{
    /* Initialise the device */
    wiced_init( );

	//WPRINT_APP_INFO(("Version data & time 2015/09/06 10:00\n"));

	/* Configure the device */
    configure_device();

	if(device_init() != WICED_SUCCESS){
		WPRINT_APP_INFO(("device_init failed\n"));
		return;
	}
	
	WPRINT_APP_INFO(("end ...\n"));
}

static wiced_result_t device_init()
{
	smart_home_app_dct_t* dct_app;
	wiced_result_t res;
	
	if(	wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *dct_app ) ) != WICED_SUCCESS)
	{
        return WICED_ERROR;
    }
	this_dev.configured = dct_app->device_configured;
	this_dev.dev_type = dct_app->dev_type;
	this_dev.dev_index = dct_app->dev_index;
	strncpy(this_dev.dev_name, dct_app->dev_name, sizeof(this_dev.dev_name));
	WPRINT_APP_INFO(("this_dev.dev_name is %s\n", this_dev.dev_name));
	wiced_dct_read_unlock( dct_app, WICED_TRUE );

	if(this_dev.configured == WICED_FALSE) {
		return WICED_ERROR;
	}		

	if(this_dev.dev_type == DEV_TYPE_MASTER) {
		this_dev.parse_socket_msg_fun = master_parse_socket_msg;
		uart_receive_enable(master_process_uart_msg);
		user_receive_enable();
	}else if(this_dev.dev_type == DEV_TYPE_LIGHT || this_dev.dev_type == DEV_TYPE_CURTAIN) {
		if(this_dev.dev_type == DEV_TYPE_LIGHT) {
			res = light_dev_init(&this_dev.specific.light_dev, WICED_HARDWARE_IO_WORKER_THREAD, report_light_status);
			if(res != WICED_SUCCESS) {
				return WICED_ERROR;
			}
			this_dev.parse_socket_msg_fun = light_parse_socket_msg;
			//this_dev.device_keypad_handler = light_keypad_handler;
			uart_keypad_enable( &device_keypad, WICED_HARDWARE_IO_WORKER_THREAD, light_keypad_handler, 3000);
		} else if(this_dev.dev_type == DEV_TYPE_CURTAIN) {
			res = curtain_init(&this_dev.specific.curtain, WICED_HARDWARE_IO_WORKER_THREAD, report_curtain_pos);
			if( res != WICED_SUCCESS) {
				return WICED_ERROR;
			}
			this_dev.parse_socket_msg_fun = curtain_parse_socket_msg;
			//this_dev.device_keypad_handler = curtain_keypad_handler;
			uart_keypad_enable( &device_keypad, WICED_HARDWARE_IO_WORKER_THREAD, curtain_keypad_handler, 3000);
		}
		pre_receive_enable();
	}
	next_receive_enable();
	return WICED_SUCCESS;
}

static void light_keypad_handler( uart_key_code_t key_code, uart_key_event_t event )
{
	int i;
	light_dev_t *light_dev = this_dev.specific.light_dev;
	//WPRINT_APP_INFO(("light_keypad_handler: key_code = 0x%.2x, key_event = %d\n", code, event));

    if ( event == KEY_EVENT_RELEASED )
    {
		for(i = 0; i < light_dev->light_count; i++) {
			if(key_code == light_dev->light_list[i].key_code) {
				switch_light_status(&light_dev->light_list[i]);
			}
		}
    }
}

static void curtain_keypad_handler( uart_key_code_t code, uart_key_event_t event )
{
	//WPRINT_APP_INFO(("curtain_keypad_handler: key_code = 0x%.2x, key_event = %d\n", code, event));

    if ( event == KEY_EVENT_RELEASED )
    {
        switch ( code )
        {
            case CURTAIN_KEY_OPEN:
				curtain_open(this_dev.specific.curtain);
				break;

            case CURTAIN_KEY_CLOSE:
				curtain_close(this_dev.specific.curtain);
				break;

			case CURTAIN_KEY_STOP:
				curtain_stop(this_dev.specific.curtain);
				break;
				
            default:
                break;
        }
    }
	else if( event == KEY_EVENT_LONG_LONG_PRESSED) {
		WPRINT_APP_INFO(("KEY_EVENT_LONG_LONG_PRESSED\n"));
		if (code == CURTAIN_KEY_OPEN) {
			curtain_cali_start(this_dev.specific.curtain);
		}
	}
	return;
}

static void report_curtain_pos(void *arg)
{
	msg_t msg;
	curtain_t *curtain = (curtain_t *)arg;

	WPRINT_APP_INFO(("report_curtain_pos\n"));
	
	memset(&msg, 0, sizeof(msg_t));
	memcpy(msg.h.msg_type, msg_bst_control, sizeof(msg.h.msg_type));
	msg.h.src_dev_index = this_dev.dev_index;
	msg.h.dst_dev_index = 0;
	msg.h.fun_type = CURTAIN_FUN_REPORT_POS;
	msg.byte8 = get_curtain_pos(curtain);
	msg.h.data_len = 1;
	//send_udp_packet(&sta_socket, this_dev.pre_dev_ip, &msg, sizeof(msg));
	send_to_pre_dev((char *)&msg, sizeof(msg_head_t) + msg.h.data_len);
	return;
}

static void report_light_status(void *arg)
{
	msg_t msg;
	light_t *light = (light_t *)arg;

	WPRINT_APP_INFO(("report_light_status\n"));
	memset(&msg, 0, sizeof(msg_t));
	memcpy(msg.h.msg_type, msg_bst_control, sizeof(msg.h.msg_type));
	msg.h.src_dev_index = this_dev.dev_index;
	msg.h.dst_dev_index = 0;
	msg.h.fun_type = LIGHT_FUN_REPORT_STATE;
	msg.h.data_len = 2;
	get_lights_status(&msg.byte8, &msg.byte9);
	send_to_pre_dev((char *)&msg, sizeof(msg_head_t) + msg.h.data_len);
	//send_udp_packet(&sta_socket, this_dev.pre_dev_ip, &msg, sizeof(msg));
	return;
}

