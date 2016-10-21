/*
 * uart_cobs.c
 *
 *  Created on: Oct 19, 2016
 *      Author: Fabio Pungg
 */

#include <xmc_usic.h>
#include <xmc_uart.h>
#include "uart_cobs.h"
#include "cobs.h"

// static UART_CONFIG_t *uart_cobs_config = NULL;
static XMC_USIC_CH_t *usic_channel = NULL;

static uint8_t txBuf[COBS_MAX_FRAME_SIZE];
static size_t tx_count = 0, curTxPos = 0;
static uint8_t rx_buf[COBS_MAX_FRAME_SIZE];	//buffer to poll the bytes into
uint8_t buf[COBS_MAX_FRAME_SIZE]; // buffer to hand to upper layer

typedef enum {
	UART_TX_IDLE, UART_TX_WORKING, UART_TX_ENDING
} tx_state_t;

static tx_state_t uart_tx_state = UART_TX_IDLE;

/*
 * Hand over control of the uart to uart_cobs module
 */
void uartCobsInit(XMC_USIC_CH_t *xmc_usic_ch) {
	if (xmc_usic_ch == NULL)
		return;

	usic_channel = xmc_usic_ch;
	XMC_UART_CH_Start(usic_channel);
}

static void uartPutData(void) {
	uint8_t tmp;
	while ((tmp = (!XMC_USIC_CH_TXFIFO_IsFull(usic_channel)))
			&& curTxPos <= tx_count) {
		XMC_USIC_CH_TXFIFO_PutData(usic_channel,
				(uint16_t) txBuf[curTxPos]);
		++curTxPos;
	}
	if (curTxPos >= tx_count) {
		uart_tx_state = UART_TX_ENDING;
		curTxPos = 0;
		tx_count = 0;
	} else {
		uart_tx_state = UART_TX_WORKING;
	}
}

void uartCobsTransmit(uint8_t *data, size_t length) {
	if (length > COBS_MAX_FRAME_SIZE - 1 || length == 0 || data == NULL) {
		return; //error, too much data or no data at all
	}
	// cobs encode the data before transmission
	tx_count = cobs_encode(data, length, txBuf);
	uartPutData();

}

void uartCobsTransmitIRQ(void) {
	switch (uart_tx_state) {
	case UART_TX_IDLE: // done transmitting
		break;
	case UART_TX_WORKING:
		uartPutData();
		break;
	case UART_TX_ENDING:
		uart_tx_state = UART_TX_IDLE;
		break;
	}
}

void uartCobsPoll(void) {
	uint8_t tmpval;
	static size_t count = 0;
	while (!XMC_USIC_CH_RXFIFO_IsEmpty(usic_channel)
			&& count < COBS_MAX_FRAME_SIZE) {
		tmpval = (uint8_t) (XMC_USIC_CH_RXFIFO_GetData(
				usic_channel) & 0xFF);
		rx_buf[count++] = tmpval;
		if (tmpval == '\0') {
			count = cobs_decode(rx_buf, count, buf); // decode the original data
			uartCobsFrameReceived(buf, count);		// pass to upper layer
			count = 0;
			return;
		}
	}
}
