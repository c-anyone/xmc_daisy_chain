/*
 * daisy_cobs.c
 *
 *  Created on: Oct 19, 2016
 *      Author: Faebsn
 */


#include "cobs.h"

#define UART_MAX_FRAME_SIZE 128

static UART_CONFIG_t *daisyUart = NULL;

static volatile uint8_t txBuf[UART_MAX_FRAME_SIZE];
static uint8_t txCount = 0;
static volatile uint8_t rxBuf[UART_MAX_FRAME_SIZE];
static uint8_t rxCount = 0;
static volatile uint8_t rxFlag = 0;
static volatile size_t rxCount = 0;

/*
 * Hand over control of the uart to uart_cobs module
 */
void uart_cobs_init(UART_CONFIG_t *uartConfig) {
	if(uartConfig == null || CRC_SW == NULL)
		return;

	daisyUart = uartConfig;
	XMC_UART_CH_Start(daisyUart->channel);
}


void uard_cobs_worker() {
	if(rxFlag != 0) {
		// cobs decode here

	}
}

void uartReceiveIRQ() {
	uint16_t rxData = 0;
	while(!XMC_USIC_CH_RXFIFO_IsEmpty(daisyUart->channel) && count < UART_MAX_FRAME_SIZE) {
		rxData = XMC_UART_CH_GetReceivedData(daisyUart->channel);
		rxBuf[count] = rxData & 0xFF;
		++rxCount;
		if(rxData == 0) {
			rxFlag = 1;
		}
	}
}


void uart_cobs_send(uint8_t *data, size_t length) {

}
