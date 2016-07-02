#include "wiced.h"
#include "uart_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define UART_RECEIVE_STACK_SIZE (6200)
#define RX_BUFFER_SIZE    256
#define UART_QUEUE_DEPTH	5
#define UART_RECEIVE_THREAD_STACK_SIZE      5*1024
#define UART_SEND_THREAD_STACK_SIZE    5*1024
#define UART_FRAME_INTERVAL	5
#define UART_FRAME_TIMEOUT	(UART_FRAME_INTERVAL/2)
#define UART_BUFF_SIZE 1024
//#define DEBUG_UART_RECEIVE
#define FRAME_END_FLAG "\r\n\r\n"

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef struct uart_frame{
	uint32_t len;
	char data[UART_BUFF_SIZE];
}uart_frame_t;

typedef enum {
	HEAD_SECTION,
	FIRST_FLAG_CR,
	SECOND_FLAG_LN,
	THIRD_FLAG_CR,
	FOURTH_FLAG_LN,
}frame_state_t;

typedef struct uart_info {
	wiced_uart_t uart;
	wiced_thread_t uart_send_thread;
	wiced_thread_t uart_recv_thread;
	wiced_queue_t uart_send_queue;
	uart_handler_t uart_handler;
}uart_info_t;

/******************************************************
 *                    Structures
 ******************************************************/


/******************************************************
 *               Static Function Declarations
 ******************************************************/



/******************************************************
 *               Variable Definitions
 ******************************************************/
static wiced_uart_config_t uart_config =
{
    .baud_rate    = 9600,
    .data_width   = DATA_WIDTH_8BIT,
    .parity       = NO_PARITY,
    .stop_bits    = STOP_BITS_1,
    .flow_control = FLOW_CONTROL_DISABLED,
};

wiced_ring_buffer_t rx_buffer;
uint8_t             rx_data[RX_BUFFER_SIZE];
static uart_info_t uart_info;

/******************************************************
 *               Function Definitions
 ******************************************************/
void uart_send_data_frame(char *buffer, uint32_t length)
{
	uart_frame_t *uart_frame;

	uart_frame = malloc(sizeof(uart_frame_t));
	if(uart_frame != NULL) {
		memset(uart_frame, 0, sizeof(uart_frame_t));
		memcpy(uart_frame->data, buffer, length);
		strcpy((char *)uart_frame->data + length, FRAME_END_FLAG);
		uart_frame->len = length + strlen(FRAME_END_FLAG);
	}
	wiced_uart_transmit_bytes( uart_info.uart, uart_frame->data, uart_frame->len);
	WPRINT_APP_INFO(("send to uart\n %s", uart_frame->data));
	free(uart_frame);
}

static void uart_receive_handler(uint32_t arg)
{
	char c;
	wiced_result_t result;
	frame_state_t state = HEAD_SECTION;
	char recv_buf[512];
	unsigned int recv_len = 0;
	
	while(1)
	{
		if(state != FOURTH_FLAG_LN) {
			result = wiced_uart_receive_bytes( uart_info.uart, &c, 1, UART_FRAME_TIMEOUT);
			if(result != WICED_SUCCESS) {
				continue;
			}
#ifdef DEBUG_UART_RECEIVE
			WPRINT_APP_INFO(("%c", c));
#endif
			recv_buf[recv_len++] = c;
			if(c == '\r') {
				if(state == HEAD_SECTION) {
					state = FIRST_FLAG_CR;
				} else if(state == SECOND_FLAG_LN) {
					state = THIRD_FLAG_CR;
				}
			} else if(c == '\n') {
				if(state == FIRST_FLAG_CR) {
					state = SECOND_FLAG_LN;
				} else if(state == THIRD_FLAG_CR) {
					state = FOURTH_FLAG_LN;
				}
			} else {
				state = HEAD_SECTION;
			}
		} else {
			state = HEAD_SECTION;
			recv_buf[recv_len] = '\0';
			if(uart_info.uart_handler != NULL) {
				uart_info.uart_handler(recv_buf, recv_len);
			}
			recv_len = 0;
			memset(recv_buf, 0, sizeof(recv_buf));
		}
	}
}

void uart_send_handler()
{
	uart_frame_t *uart_frame;

	while(1) {
		if(wiced_rtos_pop_from_queue(&uart_info.uart_send_queue, &uart_frame, WICED_NEVER_TIMEOUT) == WICED_SUCCESS) {
			wiced_uart_transmit_bytes( uart_info.uart, uart_frame->data, uart_frame->len);
#ifdef DEBUG_UART_SEND
			WPRINT_APP_INFO(("uart send: uart_frame->len = %lu\n", uart_frame->len));
#endif
			free(uart_frame);
			uart_frame = NULL;
			host_rtos_delay_milliseconds(200);
		}
	}
}

wiced_result_t uart_transceiver_enable(uart_handler_t handler)
{
	uart_info.uart = WICED_UART_2;
	
	/* Initialise ring buffer */
	ring_buffer_init(&rx_buffer, rx_data, RX_BUFFER_SIZE );

	/* Initialise UART. A ring buffer is used to hold received characters */
	wiced_uart_init( uart_info.uart, &uart_config, &rx_buffer );

	uart_info.uart_handler = handler;
	WICED_VERIFY( wiced_rtos_init_queue( &uart_info.uart_send_queue, NULL, sizeof(uart_frame_t *), UART_QUEUE_DEPTH ) );
	WPRINT_APP_INFO(("create uart receive thread\n"));
	wiced_rtos_create_thread(&uart_info.uart_recv_thread, WICED_APPLICATION_PRIORITY, "uart receive thread", uart_receive_handler, UART_RECEIVE_THREAD_STACK_SIZE, 0);
	WPRINT_APP_INFO(("create uart send thread\n"));
	wiced_rtos_create_thread(&uart_info.uart_send_thread, WICED_APPLICATION_PRIORITY, "uart send thread", uart_send_handler, UART_SEND_THREAD_STACK_SIZE, 0);
	return WICED_SUCCESS;
}

