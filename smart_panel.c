#include "wiced.h"
#include "cJSON.h"
#include "smart_panel.h"
#include "smart_panel_dct.h"
#include "list.h"
#include "uart_keypad.h"
#include "light_dev.h"
#include "curtain_dev.h"
#include "device_config.h"
#include "uart_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/



/******************************************************
 *                    Constants
 ******************************************************/
#define UDP_MAX_DATA_LENGTH         1024
#define RX_WAIT_TIMEOUT        (1*SECONDS)
#define PORTNUM                (50007)           /* UDP port */
#define USER_PORT              (8088)           /* UDP port */
#define SEND_UDP_RESPONSE
#define TCP_CONNECTION_NUMBER_OF_RETRIES  3


//#define PRE_DEV_IP_ADDRESS MAKE_IPV4_ADDRESS(192,168,0,1)
//#define UDP_BROADCAST_ADDR MAKE_IPV4_ADDRESS(192,168,0,255)


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
    {"configured", DCT_OFFSET(smart_panel_app_dct_t, device_configured),  4, CONFIG_UINT8_DATA },
	{"device type", DCT_OFFSET(smart_panel_app_dct_t, dev_type),  4, CONFIG_UINT8_DATA },
	{"device index", DCT_OFFSET(smart_panel_app_dct_t, dev_index),	 4, CONFIG_UINT8_DATA },
    {"device name", DCT_OFFSET(smart_panel_app_dct_t, dev_name),	 32, CONFIG_STRING_DATA },
	{"light status", DCT_OFFSET(smart_panel_app_dct_t, specific.light_config.light_status),   4, CONFIG_UINT8_DATA },
    {"calibrated", DCT_OFFSET(smart_panel_app_dct_t, specific.curtain_config.calibrated),  4, CONFIG_UINT8_DATA },
	{"curr_pos_ms", DCT_OFFSET(smart_panel_app_dct_t, specific.curtain_config.current_pos_ms),  4, CONFIG_UINT32_DATA },
	{"full_pos_ms", DCT_OFFSET(smart_panel_app_dct_t, specific.curtain_config.full_pos_ms),  4, CONFIG_UINT32_DATA },
    {0,0,0,0}
};
#endif
const static dev_id_t broadcast_id = "ffffffffffff";
static uart_keypad_t device_keypad;
glob_info_t glob_info;
static struct list_head dev_list;
static const wiced_ip_setting_t ap_ip_settings =
{
	INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,	1,	1 ) ),
	INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,	0 ) ),
	INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,	1,	1 ) ),
};

//wiced_ip_address_t INITIALISER_IPV4_ADDRESS( udp_broadcast_addr, UDP_BROADCAST_ADDR );

static wiced_timed_event_t sta_conn_check_event;
static wiced_timed_event_t test_event;
static wiced_timed_event_t udp_delay_event;
static wiced_udp_socket_t  next_socket;
static wiced_udp_socket_t  pre_socket;
static wiced_udp_socket_t  user_socket;
static int reconnected_status = 0;
static int alive_timeout = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/
static inline void print_ip_address(wiced_ip_address_t *ip_addr)
{
	WPRINT_APP_INFO (("%u.%u.%u.%u\n", (unsigned char) ( ( GET_IPV4_ADDRESS(*ip_addr) >> 24 ) & 0xff ),
										(unsigned char) ( ( GET_IPV4_ADDRESS(*ip_addr) >> 16 ) & 0xff ),
										(unsigned char) ( ( GET_IPV4_ADDRESS(*ip_addr) >>  8 ) & 0xff ),
										(unsigned char) ( ( GET_IPV4_ADDRESS(*ip_addr) >>  0 ) & 0xff )
										 ));
}

#if 0
static wiced_result_t tcp_connect_callback( void* socket)
{
	printf("tcp_connect_callback\n");
	
	return WICED_SUCCESS;
}

static wiced_result_t tcp_disconnect_callback( void* socket)
{
	wiced_result_t result;
	
	printf("tcp_disconnect_callback\n");
	dev_info.tcp_conn_state = 0;

	//wiced_rtos_deregister_timed_event(&dev_info.beacon_event);

	if((result = wiced_tcp_disconnect(socket)) != WICED_SUCCESS) {
		WPRINT_APP_INFO(("wiced_tcp_disconnect error. result = %d\n", result));
	}
	do
	{
		if((result = tcp_client_connect()) != WICED_SUCCESS) {
			WPRINT_APP_INFO(("tcp_client_connect failed, retry a later\n"));
			host_rtos_delay_milliseconds( 1000 );
		}
	}while(result != WICED_SUCCESS);
	
	return WICED_SUCCESS;
}

static void dev_login_sent(void* socket)
{
	char pbuf[64];

	WPRINT_APP_INFO(("dev_login_sent dev_id is %u\n", dev_info.device_id));

	memset(pbuf, 0x0, sizeof(pbuf));		 
	sprintf(pbuf, "device:%ueof", dev_info.device_id);
	
	wiced_tcp_send_buffer(socket, pbuf, strlen(pbuf));
}

wiced_result_t parse_tcp_msg(void *socket, char *buffer, unsigned short length)
{
	cJSON *json, *json_url, *json_type;
	char *url_buf;
	int url_len;

#if 0
	if(buffer[0] != '{') {
		return WICED_ERROR;
	}
#endif
    // 解析数据包  
    json = cJSON_Parse(buffer);  
    if (!json) {
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());
		return WICED_ERROR;
    }
	//wiced_tcp_send_buffer(socket, "OKeof", 5);
	//WPRINT_APP_INFO(("after send OKeof\n"));
	
    json_type = cJSON_GetObjectItem( json , "type");  
    if( json_type->type == cJSON_String && (strcmp(json_type->valuestring, "pic") == 0))  
    {
		json_url = cJSON_GetObjectItem(json, "msg");
		if( json_url->type == cJSON_String )  
		{
			url_len = strlen(json_url->valuestring);
			url_buf = malloc(url_len + 1);
			if(url_buf != NULL) {
				memset(url_buf, 0, url_len + 1);
				memcpy(url_buf, json_url->valuestring, url_len);
				WICED_VERIFY(wiced_rtos_push_to_queue(&dev_info.http_down_queue, &url_buf, WICED_NEVER_TIMEOUT));
				WPRINT_APP_INFO(("wiced_rtos_push_to_queue\n"));
				//wiced_rtos_send_asynchronous_event(&dev_info.http_download_thread, http_download_handler, NULL);
			}
        }
    }
    // 释放内存空间  
    cJSON_Delete(json);  
	return WICED_SUCCESS;
}

static wiced_result_t tcp_receive_callback(void* socket)
{
    wiced_result_t           result;
    //wiced_packet_t*          packet;
    wiced_packet_t*          rx_packet;
    char*                    rx_data;
    uint16_t                 rx_data_length;
    uint16_t                 available_data_length;

	WPRINT_APP_INFO(("tcp_receive_callback\n"));

    /* Receive a response from the server and print it out to the serial console */
    result = wiced_tcp_receive(socket, &rx_packet, TCP_CLIENT_RECEIVE_TIMEOUT);
   	if ( ( result == WICED_ERROR ) || ( result == WICED_TIMEOUT ) )
	{
		return result;
	}

    /* Get the contents of the received packet */
    wiced_packet_get_data(rx_packet, 0, (uint8_t**)&rx_data, &rx_data_length, &available_data_length);

	parse_tcp_msg(socket, rx_data, rx_data_length);

	wiced_tcp_send_buffer(socket, "OKeof", 5);

    /* Delete the packet */
    wiced_packet_delete(rx_packet);

    return WICED_SUCCESS;
}

static void tcp_client_enable()
{
	wiced_result_t result;
	int connection_retries;
	wiced_bool_t exit_flag = WICED_FALSE;

	while(!exit_flag) { 	
		/* Create a TCP socket */
		if (wiced_tcp_create_socket(&glob_info.tcp_client_socket, WICED_STA_INTERFACE) != WICED_SUCCESS)
		{
			WPRINT_APP_INFO(("TCP socket creation failed\n"));
			continue;
		}
		/* Connect to the remote TCP server, try several times */
		connection_retries = 0;
		do
		{ 
			if(wiced_hostname_lookup("api.yekertech.com", &glob_info.server_ip, 1000) != WICED_SUCCESS) {
				WPRINT_APP_INFO(("hostname look up failed\n"));
				continue;
			}
			result = wiced_tcp_connect( &glob_info.tcp_client_socket, &glob_info.server_ip, TCP_SERVER_PORT, TCP_CLIENT_CONNECT_TIMEOUT );
			connection_retries++;
		}
		while( ( result != WICED_SUCCESS ) && ( connection_retries < TCP_CONNECTION_NUMBER_OF_RETRIES ) );
		if( result != WICED_SUCCESS)
		{
			WPRINT_APP_INFO(("Unable to connect to the server, retry a later. result = %d\n", result));
			wiced_tcp_delete_socket(&glob_info.tcp_client_socket);
			break;
		} else {
			glob_info.tcp_conn_state = 1;
			wiced_tcp_register_callbacks(&glob_info.tcp_client_socket, tcp_connect_callback, tcp_receive_callback, tcp_disconnect_callback);
			dev_login_sent(&glob_info.tcp_client_socket);
			//wiced_rtos_register_timed_event( &glob_info.beacon_event, WICED_NETWORKING_WORKER_THREAD, &tcp_sent_beacon, 50*SECONDS, &glob_info.tcp_socket);
			return;
		}
	}
}

static void tcp_client_disable()
{
	glob_info.tcp_conn_state = 0;
	
	//wiced_rtos_deregister_timed_event(&glob_info.beacon_event);

	//wiced_rtos_deregister_timed_event(&glob_info.tcp_conn_event);

	wiced_tcp_unregister_callbacks(&glob_info.tcp_client_socket);
	
	wiced_tcp_delete_socket(&glob_info.tcp_client_socket);
}
#endif

wiced_result_t send_udp_packet(wiced_udp_socket_t* socket, const wiced_ip_address_t* ip_addr, const uint16_t udp_port, char *buffer, uint16_t length)
{
	wiced_packet_t* 		 packet;
	char*					 udp_data;
	uint16_t				 available_data_length;

	if(ip_addr == NULL || GET_IPV4_ADDRESS(*ip_addr) == 0) {
		WPRINT_APP_INFO( ("dest ip addr is NULL, start device finding and try again\n") );
		//handshake(socket, &udp_broadcast_addr, msg_handshake_syn);
		return WICED_ERROR;
	}
	
	/* Create the UDP packet */
	if ( wiced_packet_create_udp( socket, UDP_MAX_DATA_LENGTH, &packet, (uint8_t**) &udp_data, &available_data_length ) != WICED_SUCCESS )
	{
		WPRINT_APP_INFO( ("UDP tx packet creation failed\n") );
		return WICED_ERROR;
	}

	/* Write packet number into the UDP packet data */
	memcpy(udp_data, buffer, length);

	/* Set the end of the data portion */
	wiced_packet_set_data_end( packet, (uint8_t*) udp_data + length );

	/* Send the UDP packet */
	if ( wiced_udp_send( socket, ip_addr, udp_port, packet ) != WICED_SUCCESS )
	{
		WPRINT_APP_INFO( ("UDP packet send failed\n") );
		wiced_packet_delete( packet ); /* Delete packet, since the send failed */
		return WICED_ERROR;
	}
	return WICED_SUCCESS;
}

static void udp_send_data_frame(cJSON *json, wiced_udp_socket_t* socket, const wiced_ip_address_t* ip_addr, const uint16_t udp_port)
{
	char *out = cJSON_Print(json);
#if 0
	uart_frame_t *uart_frame = malloc(sizeof(uart_frame_t));
	if(uart_frame != NULL) {
		memset(uart_frame, 0, sizeof(uart_frame_t));
		sprintf(uart_frame->data, "%s\r\n\r\n", out);
		uart_frame->len = strlen(uart_frame->data);
	}
	send_udp_packet(socket, ip_addr, udp_port, uart_frame->data, uart_frame->len);
	free(uart_frame);
#else
	WPRINT_APP_INFO(("target ip is :"));
	print_ip_address(ip_addr);
	WPRINT_APP_INFO(("%s\n", out));
	send_udp_packet(socket, ip_addr, udp_port, out, strlen(out));
#endif
	free(out);
}

wiced_result_t discover_master_request(wiced_udp_socket_t* socket, const wiced_ip_address_t* ip_addr, const uint16_t udp_port)
{
	cJSON *root, *request;
	
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "method", "discover_master");
	cJSON_AddItemToObject(root, "request", request = cJSON_CreateObject());
	cJSON_AddStringToObject(request, "slave_id", (const char*)glob_info.dev_id);
	udp_send_data_frame(root, socket, ip_addr, udp_port);
	cJSON_Delete(root);
}

wiced_result_t discover_master_response(wiced_udp_socket_t* socket, const wiced_ip_address_t* ip_addr, const uint16_t udp_port)
{
	cJSON *root, *response;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "method", "discover_master");
	cJSON_AddItemToObject(root, "response", response = cJSON_CreateObject());
	cJSON_AddStringToObject(response, "master_id", (const char*)glob_info.dev_id);
	udp_send_data_frame(root, socket, ip_addr, udp_port);
	cJSON_Delete(root);
}

wiced_result_t discover_slave_request(wiced_udp_socket_t* socket, const wiced_ip_address_t* ip_addr, const uint16_t udp_port)
{
	cJSON *root, *request;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "method", "discover_slave");
	cJSON_AddItemToObject(root, "request", request = cJSON_CreateObject());
	cJSON_AddStringToObject(request, "master_id", (const char*)glob_info.dev_id);
	udp_send_data_frame(root, socket, ip_addr, udp_port);
	cJSON_Delete(root);
}

wiced_result_t discover_slave_response(wiced_udp_socket_t* socket, const wiced_ip_address_t* ip_addr, const uint16_t udp_port)
{
	cJSON *root, *response;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "method", "discover_slave");
	cJSON_AddItemToObject(root, "response", response = cJSON_CreateObject());
	cJSON_AddStringToObject(response, "slave_id", (const char*)glob_info.dev_id);
	udp_send_data_frame(root, socket, ip_addr, udp_port);
	cJSON_Delete(root);
}

static wiced_bool_t dev_id_equal(const dev_id_t devid1, const dev_id_t devid2)
{
	int i;
	char *p = (char *)devid1;
	char *q = (char *)devid2;
	for(i = 0; i < 12; i++){
		if(*p++ != *q++)
			return WICED_FALSE;
	}
	return WICED_TRUE;
}

static dev_info_t *find_dev_by_id(dev_id_t dev_id)
{
    dev_info_t *pos;
    list_for_each_entry(pos, &dev_list, list) {
        if (dev_id_equal(dev_id, pos->dev_id)) {
			WPRINT_APP_INFO(("find device success\n"));
            return pos;
        }
    }
    return NULL;
}

static wiced_ip_address_t* find_target_ip(dev_id_t dev_id)
{
	dev_info_t *dev = find_dev_by_id(dev_id);
	if(dev == NULL) {
		return NULL;
	}
	return &dev->dev_ip;
}

static wiced_result_t add_dev_to_list(dev_id_t dev_id, const wiced_ip_address_t* ip_addr)
{
	dev_info_t *dev = find_dev_by_id(dev_id);
	if(dev == NULL) {
		dev = malloc(sizeof(dev_info_t));
		if(dev != NULL) {
			//dev->dev_id = dev_id;
			memcpy(dev->dev_id, dev_id, sizeof(dev_id_t));
			list_add_tail(&dev->list, &dev_list);
		} else {
			return WICED_ERROR;
		}
	}
	dev->dev_ip = *ip_addr;
	WPRINT_APP_INFO(("dev_id is %s, dev_ip is ", dev->dev_id));
	print_ip_address(&dev->dev_ip);
	return WICED_SUCCESS;
}

static wiced_result_t parse_device_id(cJSON *root)
{
	cJSON *device_id;
	
	device_id = cJSON_GetObjectItem(root, "device_id");
	if(device_id != NULL && (dev_id_equal(device_id->valuestring, broadcast_id) \
		|| dev_id_equal(device_id->valuestring, glob_info.dev_id))) {
		if(dev_id_equal(device_id->valuestring, broadcast_id)) {
			cJSON_DeleteItemFromObject(root, "device_id");
			cJSON_AddStringToObject(root, "device_id", glob_info.dev_id);
		}
		return WICED_SUCCESS;
	}
	return WICED_ERROR;
}

static void response_control_panel(dev_id_t dev_id, char *dev_type, int key_code, int key_status)
{
	cJSON *root;
	char *out;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "dev_id", dev_id);
	cJSON_AddStringToObject(root, "dev_type", dev_type);
	cJSON_AddNumberToObject(root, "key_code", key_code);
	cJSON_AddNumberToObject(root, "key_status", key_status);

	out = cJSON_Print(root);
	uart_send_data_frame(out, strlen(out));
	free(out);
	cJSON_Delete(root);
}

wiced_result_t master_parse_udp_msg (void *socket, const wiced_ip_address_t* ip_addr, const uint16_t udp_port, char* buffer, uint16_t buffer_length)
{
	cJSON *root, *method, *params, *request, *response, *slave_id, *client_type, *light, *dev_id, *chan;
	int i, array_size, key_code, key_status;
	char *out;
	int result;

	root = cJSON_Parse(buffer);
	if (!root) {  
		printf("Error before: [%s]\n",cJSON_GetErrorPtr());
		return WICED_ERROR;
	}
	
	method = cJSON_GetObjectItem( root , "method");
	if(!method) {
		printf("no method\n");
		return WICED_ERROR;
	}
	if(strcmp(method->valuestring, "discover_master") == 0) {
		WPRINT_APP_INFO(("discover_master\n"));
		request = cJSON_GetObjectItem( root , "request");
		if(!request) {
			printf("no request\n");
			cJSON_Delete(root);
			return WICED_ERROR;
		}
		slave_id = cJSON_GetObjectItem( request , "slave_id");
		if(!slave_id) {
			printf("no slave_id\n");
			cJSON_Delete(root);
			return WICED_ERROR;
		}
		add_dev_to_list(slave_id->valuestring, ip_addr);
		discover_master_response(socket, ip_addr, udp_port);
	} else if(strcmp(method->valuestring, "discover_slave") == 0) {
		WPRINT_APP_INFO(("discover_slave\n"));
		response = cJSON_GetObjectItem( root , "response");
		if(!response) {
			printf("no response\n");
			cJSON_Delete(root);
			return WICED_ERROR;
		}
		slave_id = cJSON_GetObjectItem( response , "slave_id");
		if(!slave_id) {
			printf("no slave_id\n");
			cJSON_Delete(root);
			return WICED_ERROR;
		}
		add_dev_to_list(slave_id->valuestring, ip_addr);
	} else if(strcmp(method->valuestring, "report_device_status") == 0) {
		WPRINT_APP_INFO(("report_device_status\n"));
		dev_id = cJSON_GetObjectItem( root , "device_id");
		params = cJSON_GetObjectItem( root , "params");
		light = cJSON_GetObjectItem( params , "light");
		if(light != NULL) {
			if((chan = cJSON_GetObjectItem( light, "chan1"))) {
				response_control_panel(dev_id->valuestring, "light", 1, chan->valueint);
			}
			if((chan = cJSON_GetObjectItem( light, "chan2"))) {
				response_control_panel(dev_id->valuestring, "light", 2, chan->valueint);
			}
			if((chan = cJSON_GetObjectItem( light, "chan3"))) {
				response_control_panel(dev_id->valuestring, "light", 3, chan->valueint);
			}
			if((chan = cJSON_GetObjectItem( light, "chan4"))) {
				response_control_panel(dev_id->valuestring, "light", 4, chan->valueint);
			}
		}
		//response_local_user();
		//response_remote_user();
	} else {
		WPRINT_APP_INFO(("other method\n"));
		request = cJSON_GetObjectItem( root , "request");
		if(!request) {
			printf("no request\n");
			return WICED_ERROR;
		}
		client_type = cJSON_GetObjectItem( request , "client_type");
		if(!client_type) {
			printf("no client_type\n");
			return WICED_ERROR;
		}
		if(strcmp(client_type->valuestring, "control_panel") == 0) {
			WPRINT_APP_INFO(("forward to control panel\n"));
			//cJSON_DeleteItemFromObject(root, "request");
			//uart_send_data_frame(root, E200_UART);
		} else if(strcmp(client_type->valuestring, "local_user") == 0) {
			WPRINT_APP_INFO(("forward to local user\n"));
		} else if(strcmp(client_type->valuestring, "remote_user") == 0) {
			WPRINT_APP_INFO(("forward to remote user\n"));
		}
	}
	cJSON_Delete(root);
	return WICED_SUCCESS;
}

wiced_result_t slave_parse_udp_msg(void *socket, const wiced_ip_address_t* ip_addr, const uint16_t udp_port, char* buffer, uint16_t buffer_length)
{
	cJSON *root, *method, *request, *response, *params, *light, *curtain, *item, *chan, *master_id, *slave_id, *device_id, *client_type;
	int i, array_size;
	char *out;
	int result;

	root = cJSON_Parse(buffer);
	if (!root) {  
		printf("Error before: [%s]\n",cJSON_GetErrorPtr());
		return WICED_ERROR;
	}	
	method = cJSON_GetObjectItem( root , "method");
	if(!method) {
		printf("no method\n");
		return WICED_ERROR;
	}
	WPRINT_APP_INFO(("%s\n", method->valuestring));
	if(strcmp(method->valuestring, "discover_slave") == 0) {
		request = cJSON_GetObjectItem( root , "request");
		if(!request) {
			printf("no request\n");
			return WICED_ERROR;
		}
		master_id = cJSON_GetObjectItem( request , "master_id");
		if(!master_id) {
			printf("no master_id\n");
			return WICED_ERROR;
		}
		add_dev_to_list(master_id->valuestring, ip_addr);
		discover_slave_response(socket, ip_addr, udp_port);
	} else if(strcmp(method->valuestring, "discover_master") == 0) {
		response = cJSON_GetObjectItem( root , "response");
		if(!response) {
			printf("no response\n");
			return WICED_ERROR;
		}
		master_id = cJSON_GetObjectItem( response , "master_id");
		if(!master_id) {
			printf("no master_id\n");
			return WICED_ERROR;
		}
		add_dev_to_list(master_id->valuestring, ip_addr);
	} else {
		if(parse_device_id(root) != WICED_SUCCESS) {
			WPRINT_APP_INFO(("device_id error\n"));
			return WICED_ERROR;
		}
		if(strcmp(method->valuestring, "get_device_status") == 0) {
			cJSON_AddItemToObject(root, "params", params = cJSON_CreateObject());
			if(glob_info.dev_type == DEV_TYPE_LIGHT) {
				cJSON_AddItemToObject(params, "light", light = cJSON_CreateObject());
				cJSON_AddNumberToObject(light, "chan1", get_light_status(1));
				cJSON_AddNumberToObject(light, "chan2", get_light_status(2));
				cJSON_AddNumberToObject(light, "chan3", get_light_status(3));
				cJSON_AddNumberToObject(light, "chan4", get_light_status(4));
			} else if(glob_info.dev_type == DEV_TYPE_CURTAIN) {
				cJSON_AddItemToObject(params, "curtain", curtain = cJSON_CreateObject());
				cJSON_AddNumberToObject(curtain, "position", get_curtain_pos(curtain));
				//cJSON_AddNumberToObject(light, "cur_state", get_curtain_state());
			}
			udp_send_data_frame(root, socket, ip_addr, udp_port);
		} else if(strcmp(method->valuestring, "set_device_status") == 0) {
			params = cJSON_GetObjectItem( root , "params");
			if(!params) {
				printf("no params\n");
				return WICED_ERROR;
			}
			if(glob_info.dev_type == DEV_TYPE_LIGHT) {
				light = cJSON_GetObjectItem( params , "light");
				if(!light) {
					printf("no light\n");
					return WICED_ERROR;
				}
				if((chan = cJSON_GetObjectItem( light, "chan1"))) {
					set_light_status(1, chan->valueint);
				}
				if((chan = cJSON_GetObjectItem( light, "chan2"))) {
					set_light_status(2, chan->valueint);
				}
				if((chan = cJSON_GetObjectItem( light, "chan3"))) {
					set_light_status(3, chan->valueint);
				}
				if((chan = cJSON_GetObjectItem( light, "chan4"))) {
					set_light_status(4, chan->valueint);
				}
			} else if(glob_info.dev_type == DEV_TYPE_CURTAIN) {
				curtain = cJSON_GetObjectItem( params , "curtain");
				if(!curtain) {
					printf("no curtain\n");
					return WICED_ERROR;
				}
				if((item = cJSON_GetObjectItem(curtain, "position"))) {
					set_curtain_pos(glob_info.specific.curtain, item->valueint);
				}
			}
			cJSON_DeleteItemFromObject(root, "params");
			cJSON_AddItemToObject(root, "response", response = cJSON_CreateObject());
			cJSON_AddNumberToObject(response, "result", 0);
			udp_send_data_frame(root, socket, ip_addr, udp_port);
		}
	}
	cJSON_Delete(root);
	return WICED_SUCCESS;
}

static wiced_result_t udp_receive_callback(void *socket)
{
	wiced_packet_t* 		  packet;
	char*					  rx_data;
	static uint16_t 		  rx_data_length;
	uint16_t				  available_data_length;
	static wiced_ip_address_t udp_src_ip_addr;
	static uint16_t 		  udp_src_port;

	WPRINT_APP_INFO(("udp_receive_callback\n"));
	/* Wait for UDP packet */
	wiced_result_t result = wiced_udp_receive( socket, &packet, RX_WAIT_TIMEOUT );

	if ( ( result == WICED_ERROR ) || ( result == WICED_TIMEOUT ) )
	{
		return result;
	}

	/* Get info about the received UDP packet */
	wiced_udp_packet_get_info( packet, &udp_src_ip_addr, &udp_src_port );

	/* Extract the received data from the UDP packet */
	wiced_packet_get_data( packet, 0, (uint8_t**) &rx_data, &rx_data_length, &available_data_length );

	/* Null terminate the received data, just in case the sender didn't do this */
	//rx_data[ rx_data_length ] = '\x0';

	//print_ip_address(&udp_src_ip_addr);
	
	//parse_udp_msg(socket, &udp_src_ip_addr, udp_src_port, rx_data, rx_data_length);
	if(glob_info.dev_type == DEV_TYPE_MASTER) {
		master_parse_udp_msg(socket, &udp_src_ip_addr, udp_src_port, rx_data, rx_data_length);
	} else {
		slave_parse_udp_msg(socket, &udp_src_ip_addr, udp_src_port, rx_data, rx_data_length);
	}
	
	/* Delete the received packet, it is no longer needed */
	wiced_packet_delete( packet );

	return WICED_SUCCESS;
}

static void udp_client_enable()
{
	/* Create UDP socket */
	if ( wiced_udp_create_socket( &glob_info.udp_socket, PORTNUM, WICED_STA_INTERFACE ) != WICED_SUCCESS )
	{
		WPRINT_APP_INFO( ("UDP socket creation failed\n") );
		return;
	}

	wiced_udp_register_callbacks(&glob_info.udp_socket, udp_receive_callback);

	if(glob_info.dev_type == DEV_TYPE_MASTER) {
		discover_slave_request(&glob_info.udp_socket, WICED_IP_BROADCAST, PORTNUM);
	} else {
		discover_master_request(&glob_info.udp_socket, WICED_IP_BROADCAST, PORTNUM);
	}
}

static void udp_client_disable()
{
	wiced_udp_unregister_callbacks(&glob_info.udp_socket);
	
	wiced_udp_delete_socket(&glob_info.udp_socket);
}

static void link_up( void )
{

	WPRINT_APP_INFO( ("And we're connected again!\n") );
	
	/* Set a semaphore to indicate the link is back up */
	//wiced_rtos_set_semaphore( &link_up_semaphore );
	glob_info.wlan_status = 1;
#ifdef ENABLE_REMOTE_CONTROL
	tcp_client_enable();
	//wiced_rtos_register_timed_event( &dev_info.tcp_conn_event, WICED_NETWORKING_WORKER_THREAD, &tcp_connect_handler, 1*SECONDS, 0 );
#endif
}

static void link_down( void )
{
	WPRINT_APP_INFO( ("Network connection is down.\n") );

	glob_info.wlan_status = 0;
#ifdef ENABLE_REMOTE_CONTROL
	tcp_client_disable();
#endif
}

wiced_result_t client_enable()
{
	/* Register callbacks */
	wiced_network_register_link_callback( link_up, link_down );

	glob_info.retry_ms = 500;
	/* Bring up the network interface */
	while(wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL ) != WICED_SUCCESS) {
		host_rtos_delay_milliseconds(glob_info.retry_ms);
		glob_info.retry_ms *= 2;
		if(glob_info.retry_ms >= 60*1000) {
			glob_info.retry_ms = 500;
		}
	}
	udp_client_enable();
#ifdef ENABLE_REMOTE_CONTROL
	if(glob_info.dev_type == DEV_TYPE_MASTER) {
		tcp_client_enable();
	}
#endif
}

static void send_control_data(dev_id_t dev_id, char *dev_type, int key_code, int key_status, char *bin_data)
{
	cJSON *root, *request, *params, *light, *curtain;
	char *out;
	int position;
	char chan_str[6];
	wiced_ip_address_t *target_ip;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "method", "set_device_status");
	cJSON_AddStringToObject(root, "device_id", dev_id);
	cJSON_AddItemToObject(root, "params", params = cJSON_CreateObject());
#ifdef TRANSFER_BIN_DATA
	if(bin_data != NULL) {
		cJSON_AddStringToObject(root, "bin_data", bin_data);
	}
#endif
	if(strcmp(dev_type, "light") == 0) {
		cJSON_AddItemToObject(params, "light", light = cJSON_CreateObject());
		memset(chan_str, 0, sizeof(chan_str));
		sprintf(chan_str, "chan%d", key_code);
		cJSON_AddNumberToObject(light, chan_str, key_status);
	} else if(strcmp(dev_type, "curtain") == 0) {
		cJSON_AddItemToObject(params, "curtain", curtain = cJSON_CreateObject());		
		cJSON_AddNumberToObject(curtain, "position", key_code);
	}
	cJSON_AddItemToObject(root, "request", request = cJSON_CreateObject());
	cJSON_AddStringToObject(request, "client_type", "control_panel");
#if 1
	if(dev_id_equal(dev_id, broadcast_id)) {
		target_ip = WICED_IP_BROADCAST;
	} else {
		target_ip = find_target_ip(dev_id);
	}
	if(target_ip != NULL) {
		udp_send_data_frame(root, &glob_info.udp_socket, target_ip, PORTNUM);
	} else {
		discover_slave_request(&glob_info.udp_socket, WICED_IP_BROADCAST, PORTNUM);
	}
#else
	udp_send_data_frame(root, &glob_info.udp_socket, WICED_IP_BROADCAST, PORTNUM);
#endif
	cJSON_Delete(root);
}

static uart_handler_t master_parse_uart_msg(char *buffer, uint32_t length)
{
	cJSON *root, *dev_id, *dev_type, *key_code, *key_status, *bin_data;
	
    root = cJSON_Parse(buffer);
    if (!root) {  
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());
		return WICED_ERROR;
    }
    dev_id = cJSON_GetObjectItem( root , "dev_id");
	if(!dev_id) {
		printf("no dev_id\n");
		return WICED_ERROR;
	}
	dev_type = cJSON_GetObjectItem( root , "dev_type");
	key_code = cJSON_GetObjectItem( root , "key_code");
	key_status = cJSON_GetObjectItem( root , "key_status");
	bin_data = cJSON_GetObjectItem( root , "bin_data");
	WPRINT_APP_INFO(("dev_id = %s, dev_type = %s\n", dev_id->valuestring, dev_type->valuestring));
	send_control_data(dev_id->valuestring, dev_type->valuestring, \
		key_code->valueint, key_status->valueint, bin_data->valuestring);
    cJSON_Delete(root);
    return WICED_SUCCESS;
}

static void light_keypad_handler( uart_key_code_t key_code, uart_key_event_t event )
{
	int i;
	light_dev_t *light_dev = glob_info.specific.light_dev;
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
				curtain_open(glob_info.specific.curtain);
				break;

            case CURTAIN_KEY_CLOSE:
				curtain_close(glob_info.specific.curtain);
				break;

			case CURTAIN_KEY_STOP:
				curtain_stop(glob_info.specific.curtain);
				break;
				
            default:
                break;
        }
    }
	else if( event == KEY_EVENT_LONG_LONG_PRESSED) {
		WPRINT_APP_INFO(("KEY_EVENT_LONG_LONG_PRESSED\n"));
		if (code == CURTAIN_KEY_OPEN) {
			curtain_cali_start(glob_info.specific.curtain);
		}
	}
	return;
}

static void get_device_status(wiced_udp_socket_t* socket, const wiced_ip_address_t* ip_addr, const uint16_t udp_port)
{
	cJSON *root, *params, *light, *curtain;
	char *out;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "method", "get_device_status");
	cJSON_AddStringToObject(root, "device_id", glob_info.dev_id);
	cJSON_AddItemToObject(root, "params", params = cJSON_CreateObject());
	if(glob_info.dev_type == DEV_TYPE_LIGHT) {
		cJSON_AddItemToObject(params, "light", light = cJSON_CreateObject());
		cJSON_AddNumberToObject(light, "chan1", get_light_status(1));
		cJSON_AddNumberToObject(light, "chan2", get_light_status(2));
		cJSON_AddNumberToObject(light, "chan3", get_light_status(3));
		cJSON_AddNumberToObject(light, "chan4", get_light_status(4));
	} else if(glob_info.dev_type == DEV_TYPE_CURTAIN) {
		cJSON_AddItemToObject(params, "curtain", curtain = cJSON_CreateObject());
		cJSON_AddNumberToObject(curtain, "position", get_curtain_pos(curtain));
		//cJSON_AddNumberToObject(light, "cur_state", get_curtain_state());
	}
	udp_send_data_frame(root, socket, ip_addr, udp_port);
	cJSON_Delete(root);
}

static void report_device_status(void *arg)
{
	cJSON *root, *params, *light, *curtain;
	char *out;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "method", "report_device_status");
	cJSON_AddStringToObject(root, "device_id", glob_info.dev_id);
	cJSON_AddItemToObject(root, "params", params = cJSON_CreateObject());
	if(glob_info.dev_type == DEV_TYPE_LIGHT) {
		cJSON_AddItemToObject(params, "light", light = cJSON_CreateObject());
		cJSON_AddNumberToObject(light, "chan1", get_light_status(1));
		cJSON_AddNumberToObject(light, "chan2", get_light_status(2));
		cJSON_AddNumberToObject(light, "chan3", get_light_status(3));
		cJSON_AddNumberToObject(light, "chan4", get_light_status(4));
	} else if(glob_info.dev_type == DEV_TYPE_CURTAIN) {
		cJSON_AddItemToObject(params, "curtain", curtain = cJSON_CreateObject());
		cJSON_AddNumberToObject(curtain, "position", get_curtain_pos(curtain));
		//cJSON_AddNumberToObject(light, "cur_state", get_curtain_state());
	}
	udp_send_data_frame(root, &glob_info.udp_socket, WICED_IP_BROADCAST, PORTNUM);
	cJSON_Delete(root);
}

static wiced_result_t device_basic_init()
{
	wiced_mac_t mac;
	char ssid[64] = {0};
	//smart_panel_app_dct_t* dct_app;
	wiced_result_t res;
	
	//get mac addr
	wwd_wifi_get_mac_address( &mac, WWD_STA_INTERFACE );

	//init_device_id
	memset(glob_info.dev_id, 0, sizeof(dev_id_t));

	sprintf(glob_info.dev_id, "%02x%02x%02x%02x%02x%02x", \
		mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5]);

	WPRINT_APP_INFO(("device_id is %s\n", glob_info.dev_id));
	
	//init_ssid
	sprintf(ssid, "smart_panel_%02x%02x%02x", mac.octet[3], mac.octet[4], mac.octet[5]);
	load_wifi_data(&glob_info.wifi_dct);
	memcpy(glob_info.wifi_dct.soft_ap_settings.SSID.value, ssid, strlen(ssid) + 1);
	glob_info.wifi_dct.soft_ap_settings.SSID.length = strlen(ssid);
	store_wifi_data(&glob_info.wifi_dct);
	//memcpy(glob_info.wifi_dct->soft_ap_settings.security_key, passwd, strlen(passwd) + 1);
	//glob_info.wifi_dct->soft_ap_settings.security_key_length = strlen(passwd);

	load_app_data(&glob_info.app_dct);
	glob_info.configured = glob_info.app_dct.device_configured;
	glob_info.dev_type = glob_info.app_dct.dev_type;
	//glob_info.dev_index = glob_info.app_dct.dev_index;
	strncpy(glob_info.dev_name, glob_info.app_dct.dev_name, sizeof(glob_info.dev_name));
	WPRINT_APP_INFO(("glob_info.dev_name is %s\n", glob_info.dev_name));
	return WICED_SUCCESS;
}

static wiced_result_t device_function_init()
{
	wiced_result_t res;

	if(glob_info.configured == WICED_FALSE) {
		return WICED_ERROR;
	}		

	if(glob_info.dev_type == DEV_TYPE_MASTER) {
		//glob_info.parse_socket_msg_fun = master_parse_socket_msg;
		//uart_receive_enable(master_process_uart_msg);
		uart_transceiver_enable(master_parse_uart_msg);
	}else if(glob_info.dev_type == DEV_TYPE_LIGHT || glob_info.dev_type == DEV_TYPE_CURTAIN) {
		if(glob_info.dev_type == DEV_TYPE_LIGHT) {
			res = light_dev_init(&glob_info.specific.light_dev, WICED_HARDWARE_IO_WORKER_THREAD, report_device_status);
			if(res != WICED_SUCCESS) {
				return WICED_ERROR;
			}
			//glob_info.parse_socket_msg_fun = light_parse_socket_msg;
			//glob_info.device_keypad_handler = light_keypad_handler;
			uart_keypad_enable( &device_keypad, WICED_HARDWARE_IO_WORKER_THREAD, light_keypad_handler, 3000);
		} else if(glob_info.dev_type == DEV_TYPE_CURTAIN) {
			res = curtain_init(&glob_info.specific.curtain, WICED_HARDWARE_IO_WORKER_THREAD, report_device_status);
			if( res != WICED_SUCCESS) {
				return WICED_ERROR;
			}
			//glob_info.parse_socket_msg_fun = curtain_parse_socket_msg;
			//glob_info.device_keypad_handler = curtain_keypad_handler;
			uart_keypad_enable( &device_keypad, WICED_HARDWARE_IO_WORKER_THREAD, curtain_keypad_handler, 3000);
		}
	}
	//udp_receive_enable();
	INIT_LIST_HEAD(&dev_list);
	client_enable();
	return WICED_SUCCESS;
}

void application_start( )
{
    /* Initialise the device and WICED framework */
    wiced_init( );

	device_basic_init();
	
	/* Configure the device */
    startup_configuration_page();

	device_function_init();

	WPRINT_APP_INFO(("end ...\n"));
}
