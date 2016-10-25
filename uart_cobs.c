/*
 * uart_cobs.c
 *
 *  Created on: Oct 19, 2016
 *      Author: Fabio Pungg
 */

#include <Dave.h>
#include "uart_cobs.h"
#include "cobs.h"

/**
 * Local pointer to the USIC channel
 */
static XMC_USIC_CH_t *usic_channel = NULL;

/**
 * local transmission buffer, takes the stuffed bytes before
 * putting them into the transmit FIFO
 */
static uint8_t tx_buf[COBS_MAX_FRAME_SIZE];

static size_t tx_count = 0, curTxPos = 0;
static uint8_t rx_buf[COBS_MAX_FRAME_SIZE];	//buffer to poll the bytes into
uint8_t buf[COBS_MAX_FRAME_SIZE]; // buffer to hand to upper layer

/**
 * local function to write data from the transmit buffer to
 * the UART transmit FIFO
 */
static void uartPutData(void);

typedef enum {
	UART_TX_IDLE, UART_TX_WORKING, UART_TX_ENDING
} tx_state_t;


static tx_state_t uart_tx_state = UART_TX_IDLE;

void uartCobsInit(XMC_USIC_CH_t *xmc_usic_ch) {
	if (xmc_usic_ch == NULL)
		return;

	usic_channel = xmc_usic_ch;
	XMC_UART_CH_Start(usic_channel);
}

static void uartPutData(void) {
	uint8_t tmp;
	while ((tmp = (!XMC_USIC_CH_TXFIFO_IsFull(usic_channel))) && curTxPos < tx_count) {
		XMC_USIC_CH_TXFIFO_PutData(usic_channel,(uint16_t) tx_buf[curTxPos]);
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
	/**
	 * call the cobs bytes stuffing algorithm before transmission
	 */
	tx_count = cobs_encode(data, length, tx_buf);
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
		tmpval = (uint8_t) (XMC_USIC_CH_RXFIFO_GetData(usic_channel) & 0xFF);
		rx_buf[count++] = tmpval;

		/**
		 * if the received byte is an end of frame marker, unstuff
		 * the frame and pass it to the upper layer
		 */
		if (tmpval == '\0') {
			count = cobs_decode(rx_buf, count, buf); // decode the original data
			uartCobsFrameReceived(buf, count);		// pass to upper layer
			count = 0;
			return;
		}
	}
}
