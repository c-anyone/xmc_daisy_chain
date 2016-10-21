/*
 * DaisyChain.c
 *
 *  Created on: Jul 6, 2016
 *      Author: Fabio Pungg
 */

#include <DAVE.h>
#include "DaisyChain.h"
#include "uart_cobs.h"
#define DAISY_MIN_FRAME_SIZE (4U)	//2 address bytes and 2 byte crc
#define DAISY_MAX_FRAME_SIZE (COBS_MAX_FRAME_SIZE-DAISY_MIN_FRAME_SIZE)
/*
 *
 */
static uint8_t daisy_address = 0xef;
static uint8_t framebuf[64];
//static uint8_t frameLength=0;
static UART_CONFIG_t *uart_config;

void daisyInit(UART_CONFIG_t *uartConfig) {
	if (uartConfig == NULL)
		return;
	uart_config = uartConfig;
	uartCobsInit(uart_config->channel);
}

void daisyWorker(void) {
	uartCobsPoll();
}

void daisySendData(uint8_t receiver, uint8_t sender, uint8_t *data,
		size_t length) {
	uint32_t crc = 0;
	size_t i = 0;
	framebuf[i++] = receiver;
	framebuf[i++] = sender;
	while (i < (length + 2)) {
		framebuf[i] = data[i - 2];
		++i;
	}

	CRC_SW_CalculateCRC(&CRC_SW_0, framebuf, length + 2);
	crc = CRC_SW_GetCRCResult(&CRC_SW_0);

	framebuf[i++] = (uint8_t) (crc >> 8) & 0xff;
	framebuf[i++] = (uint8_t) (crc & 0xff);

	uartCobsTransmit(framebuf, i);
}

/*
 *  Receives a Frame, calculates CRC, checks receiver address and
 *  handles command data
 */
void uartCobsFrameReceived(uint8_t *frame, size_t length) {
	uint8_t receive_address = 0;
	uint8_t sender_address = 0;
	uint8_t *data_start = NULL;
	size_t data_length = 0;
	uint32_t crc = 0;
	if (length < DAISY_MIN_FRAME_SIZE) {
		// broken frame, fail silent
		return;
	}
	crc = ((uint32_t) frame[length - 1]) << 8 | ((uint32_t) frame[length - 1]);
	CRC_SW_CalculateCRC(&CRC_SW_0, frame, length - 2);
	if (crc - CRC_SW_GetCRCResult(&CRC_SW_0) != 0) {
		// crc incorrect, broken data, fail silent
		return;
	}
	// first byte is the receive address
	receive_address = frame[0];
	// second byte is the sender address
	sender_address = frame[1];
	// data starts at 3rd byte
	data_start = frame + 2;
	// data length is the frame length-addresses-crc bytes
	data_length = length - 4;

	if (receive_address == daisy_address || receive_address == DAISY_BROADCAST) {
		// packet is for us to use, act now!
	} else {
		uartCobsTransmit(frame, length);
		// packet is not for us, retransmit
	}
}
