/*
 * uart_cobs.c
 *
 *  Created on: Oct 19, 2016
 *      Author: Faebsn
 */

#include <DAVE.h>
#include "uart_cobs.h"
#include "cobs.h"

static UART_CONFIG_t *uart_cobs_config = NULL;

static uint8_t txBuf[MAX_FRAME_SIZE];
static size_t tx_count = 0;
static uint8_t *sendPosition;
static uint8_t rx_buf[MAX_FRAME_SIZE];	//buffer to poll the bytes
uint8_t buf[MAX_FRAME_SIZE]; // buffer to hand to upper layer

static size_t txFifoSize = 0;

typedef enum {
	UART_TX_IDLE, UART_TX_WORKING, UART_TX_ENDING
} tx_state_t;

static tx_state_t uart_tx_state = UART_TX_IDLE;

/*
 * Hand over control of the uart to uart_cobs module
 */
void uartCobsInit(UART_CONFIG_t *uartConfig) {
	if (uartConfig == NULL)
		return;

	uart_cobs_config = uartConfig;
	txFifoSize = 1U << uartConfig->config->tx_fifo_size;
	XMC_UART_CH_Start(uart_cobs_config->channel);
}

static void uartPutData(void) {
	size_t i, length;
	if (tx_count > txFifoSize) {
		uart_tx_state = UART_TX_WORKING;
		length = txFifoSize;
		tx_count -= txFifoSize;
	} else {
		uart_tx_state = UART_TX_ENDING;
		length = tx_count;
		tx_count = 0;
	}
	for (i = 0; i < length; ++i) {
		XMC_USIC_CH_TXFIFO_PutData(uart_cobs_config->channel, (uint16_t) sendPosition[i]);
	}
	sendPosition += length;
}

void uart_cobs_transmit(uint8_t *data, size_t length) {
	if (length > MAX_FRAME_SIZE - 1 || length == 0 || data == NULL) {
		return; //error, too much data or no data at all
	}
	// cobs encode the data before transmission
	tx_count = cobs_encode(data, length, txBuf);
	// set the sendPosition to start of the buffer
	sendPosition = txBuf;

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

void pollUartCobs(void) {
	uint8_t tmpval;
	static size_t count = 0;
	while (!XMC_USIC_CH_RXFIFO_IsEmpty(uart_cobs_config->channel)
			&& count < MAX_FRAME_SIZE) {
		tmpval = (uint8_t) XMC_UART_CH_GetReceivedData(
				uart_cobs_config->channel);
		rx_buf[count++] = tmpval;
		if (tmpval == '\0') {
			count = cobs_decode(rx_buf, count, buf); // decode the original data
			uartCobsFrameReceived(buf, count);		// pass to upper layer
			count = 0;
		}
	}
}

/*
 int uartCobsWorker() {
 uint8_t tmpval;
 static size_t count=0;
 if (rx_flag != 0) {
 rx_flag = 0;
 while((tmpval = rb_pop(&rx_fifo)) != 0) {
 rx_buf[count] = tmpval;
 ++count;
 }
 return 0;
 }
 return 1;
 }
 */
size_t uart_cobs_frame_received(uint8_t *buf, size_t length);

/*
 * when the uart fifo is filled, read data from fifo and put it into rbuf
 */
/*
 void uartReceiveIRQ() {
 uint16_t rxData = 0;
 while (!XMC_USIC_CH_RXFIFO_IsEmpty(uart_cobs_config->channel)
 && count < UART_MAX_FRAME_SIZE) {
 rxData = XMC_UART_CH_GetReceivedData(uart_cobs_config->channel);
 rb_push(rx_fifo, rxData & 0xFF);
 if (rxData == 0) {
 rx_flag = 1;
 return;
 }
 }
 }

 */
