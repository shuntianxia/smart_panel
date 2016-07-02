#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_Smart_panel

$(NAME)_SOURCES := smart_panel.c \
                   smart_panel_dct.c \
				   device_config.c \
				   device_config_content.c \
				   uart_keypad.c \
				   light_dev.c \
				   curtain_dev.c \
				   cJSON/cJSON.c \
				   list.h \
				   uart_interface.c \
				   comm.c
				   
$(NAME)_INCLUDES := ./cJSON ./proto ./easy_setup

$(NAME)_RESOURCES  := apps/smart_panel/device_config.html \
                      images/brcmlogo.png \
                      images/brcmlogo_line.png \
                      images/favicon.ico \
                      scripts/general_ajax_script.js
                      
GLOBAL_DEFINES :=

WIFI_CONFIG_DCT_H := wifi_config_dct.h

APPLICATION_DCT := smart_panel_dct.c
