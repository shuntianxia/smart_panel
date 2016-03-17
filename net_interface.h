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

#include "wiced_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#define	MSG_BST_CONTROL "bst"
#define MSG_HANDSHAKE_SYN "ff1",
#define MSG_HANDSHAKE_ASK "ff2"

/******************************************************
 *                    Constants
 ******************************************************/


/******************************************************
 *                   Enumerations
 ******************************************************/
enum dev_type{
	DEV_TYPE_MASTER = 0x00,
	DEV_TYPE_LIGHT = 0x01,
	DEV_TYPE_CURTAIN = 0x02,
};

enum ctrl_flag{
	CTRL_FLAG_NONE = 0x00,
	CTRL_FLAG_PANEL = 0x01,
	CTRL_FLAG_PHONE = 0x02,
};

/******************************************************
 *                 Type Definitions
 ******************************************************/


/******************************************************
 *                    Structures
 ******************************************************/
#pragma pack(1)
	
typedef uint8_t msg_type_t[3];

typedef struct {
	msg_type_t	msg_type;
	uint8_t 	msg_no;
	uint8_t 	data_len;
	uint8_t 	src_dev_index;
	uint8_t 	dst_dev_index;
	uint8_t 	fun_type;
} msg_head_t;

typedef struct {
	msg_head_t	h;
	uint8_t 	byte8;
	uint8_t 	byte9;
	uint8_t 	byte10;
	uint8_t 	byte11;
	uint8_t 	byte12;
	uint8_t 	byte13;
	uint8_t 	byte14;
	uint8_t 	byte15;
	uint8_t 	byte16;
	uint8_t 	byte17;
	uint8_t 	byte18;
	uint8_t 	byte19;
	uint8_t 	byte20;
	uint8_t 	byte21;
	uint8_t 	byte22;
	uint8_t 	byte23;
	uint8_t 	byte24;
	uint8_t 	byte25;
	uint8_t 	byte26;
	uint8_t 	byte27;
	uint8_t 	byte28;
	uint8_t 	byte29;
	uint8_t 	byte30;
	uint8_t 	byte31;
	uint8_t 	byte32;
	uint8_t 	byte33;
	uint8_t 	byte34;
	uint8_t 	byte35;
	uint8_t 	byte36;
	uint8_t 	byte37;
	uint8_t 	byte38;
	uint8_t 	byte39;
	uint8_t 	byte40;
	uint8_t 	byte41;
} msg_t;

typedef struct {
	uint16_t	prefix;
	uint8_t 	data_len;
	uint16_t	cmd_type;
	uint8_t 	status;
} keypress_cmd_t;
	
#pragma pack()


/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
wiced_result_t next_receive_enable();
wiced_result_t pre_receive_enable();
wiced_result_t user_receive_enable();
wiced_result_t send_udp_packet(wiced_udp_socket_t* socket, const wiced_ip_address_t* ip_addr, const uint16_t udp_port, char *buffer, uint16_t length);
wiced_result_t master_parse_socket_msg (char* buffer, uint16_t buffer_length);
wiced_result_t curtain_parse_socket_msg(char *rx_data, uint16_t rx_data_length);
wiced_result_t light_parse_socket_msg(char *rx_data, uint16_t rx_data_length);
wiced_result_t send_to_pre_dev(char *buffer, uint16_t length);
wiced_result_t send_to_next_dev(char *buffer, uint16_t length);

#ifdef __cplusplus
} /* extern "C" */
#endif
