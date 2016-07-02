#pragma once

#include "wiced_rtos.h"

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


/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef wiced_result_t (*uart_handler_t)(char *buffer, uint32_t length);


/******************************************************
 *                    Structures
 ******************************************************/



/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
wiced_result_t uart_transceiver_enable(uart_handler_t handler);
void uart_send_data_frame(char *buffer, uint32_t length);

#ifdef __cplusplus
} /* extern "C" */
#endif
