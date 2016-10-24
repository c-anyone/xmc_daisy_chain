/*
 * uart_cobs.c
 *
 *  Created on: Oct 19, 2016
 *      Author: Fabio Pungg
 */

#include <Dave.h>
#include <xmc_usic.h>
#include <xmc_uart.h>
#include "uart_cobs.h"
#include "cobs.h"

#define COBS_TX_BUF_COUNT	(4u)

// static UART_CONFIG_t *uart_cobs_config = NULL;
static XMC_USIC_CH_t *usic_channel = NULL;

typedef struct {
	uint8_t data[COBS_MAX_FRAME_SIZE];
	size_t length;
	size_t pos;
} tx_struct_t;

static tx_struct_t tx_buf[COBS_TX_BUF_COUNT];
//static uint8_t txBuf[COBS_MAX_FRAME_SIZE];
static tx_struct_t *cur_buf = tx_buf;
static tx_struct_t *last_buf = tx_buf;
static size_t tx_count = 0;
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

	tx_count = 0;

	usic_channel = xmc_usic_ch;
	XMC_UART_CH_Start(usic_channel);
}

#define INC_STRUCT_POINTER(ptr,base)	((ptr) = (((ptr)+1) >= ((base)+COBS_TX_BUF_COUNT ? ptr+1 : base)))
#define INC_TX_COUNT(x)	(x = (x+1) < COBS_TX_BUF_COUNT ? (x+1) : 0)

static void uartPutData(void) {
	size_t position = cur_buf->pos;
	static size_t curTxPos = 0;
	while (((!XMC_USIC_CH_TXFIFO_IsFull(usic_channel)))	&& (position < cur_buf->length)) {
		XMC_USIC_CH_TXFIFO_PutData(usic_channel, (uint16_t) cur_buf->data[position]);
		position++;
	}
	if (cur_buf->pos < cur_buf->length) {
		cur_buf->pos = position;
	} else {
		cur_buf->pos = 0;
		cur_buf->length = 0;
		INC_TX_COUNT(curTxPos);
		cur_buf = &tx_buf[curTxPos];
		if (cur_buf == last_buf) {
			uart_tx_state = UART_TX_ENDING;
		}
	}
}

void uartCobsTransmit(uint8_t *data, size_t length) {
	if (length > COBS_MAX_FRAME_SIZE - 1 || length == 0 || data == NULL) {
		return; //error, too much data or no data at all
	}
	// cobs encode the data before transmission

	last_buf->length = cobs_encode(data, length, last_buf->data);
	INC_TX_COUNT(tx_count);
	last_buf = &tx_buf[tx_count];
	uart_tx_state = UART_TX_WORKING;
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
		if (tmpval == '\0') {
			count = cobs_decode(rx_buf, count, buf); // decode the original data
			uartCobsFrameReceived(buf, count);		// pass to upper layer
			count = 0;
			return;
		}
	}
}
