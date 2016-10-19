/*
 * uart_cobs.c
 *
 *  Created on: Oct 19, 2016
 *      Author: Faebsn
 */

#include "cobs.h"
#include "ringbuffer.h" // don't use the ringbuffer/fifo for receiving, use an
// array so cobs_decode can work it's magic.
// the upper layer of the program (daisy in this case) needs to give the uart
// handler a buffer, where the received data will be written to (outside the interrupt handler)
// this is the pointer named output in cobs_decode

#define UART_MAX_FRAME_SIZE 128

static UART_CONFIG_t *uart_cobs_config = NULL;

static ring_buffer_t rx_fifo;

static volatile uint8_t txBuf[UART_MAX_FRAME_SIZE];
static uint8_t tx_count = 0;
static volatile uint8_t rx_buf[UART_MAX_FRAME_SIZE];
//static uint8_t rxCount = 0;
static volatile uint8_t rx_flag = 0;
//static volatile size_t rxCount = 0;

/*
 * Hand over control of the uart to uart_cobs module
 */
void uartCobsInit(UART_CONFIG_t *uartConfig) {
	if (uartConfig == null || CRC_SW == NULL)
		return;

	rb_init(&rx_fifo);

	uart_cobs_config = uartConfig;

	XMC_UART_CH_Start(uart_cobs_config->channel);
}

void uart_cobs_transmit(uint8_t *data, size_t length) {

}

int uartCobsWorker() {
	uint8_t tmpval,count=0;
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

/*
 * when the uart fifo is filled, read data from fifo and put it into rbuf
 */

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

