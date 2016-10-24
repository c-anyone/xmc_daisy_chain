/*
 * DaisyChain.c
 *
 *  Created on: Jul 6, 2016
 *      Author: Fabio Pungg
 */

#include <DAVE.h>
#include "DaisyChain.h"
#include "uart_cobs.h"
#define DAISY_MIN_PACKET_SIZE (4U)	//2 address bytes and 2 byte crc
#define DAISY_MAX_PACKET_SIZE (COBS_MAX_FRAME_SIZE-DAISY_MIN_PACKET_SIZE)

/*
 *
 */
#ifndef DAISY_MASTER_DEVICE
static uint8_t daisy_address = DAISY_STARTING_ADDRESS;
#endif
static uint8_t framebuf[64];
//static uint8_t frameLength=0;
static UART_CONFIG_t *uart_config;

static inline uint32_t crc_check(uint8_t *data, size_t length);

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
	uint8_t receive_addr = 0;
	uint8_t sender_addr = 0;
	uint8_t *data = NULL;
	size_t data_length = 0;
	if (length < DAISY_MIN_PACKET_SIZE || length > DAISY_MAX_PACKET_SIZE) {
		// broken frame, fail silent
		return;
	}
	if (crc_check(frame, length) != 0) {
		// crc incorrect, broken data, fail silent
		return;
	}
	// first byte is the receive address
	receive_addr = frame[0];
	// second byte is the sender address
	sender_addr = frame[1];
	// data starts at 3rd byte
	data = frame + 2;
	// data length is the frame length-addresses-crc bytes
	data_length = length - 4;

	// this is the master handler
#ifdef DAISY_MASTER_DEVICE
	if ((receive_addr == DAISY_ADDR_BROADCAST) 		// broadcasts stop here
			|| (receive_addr == DAISY_ADDR_MASTER)	// packet for us
			|| (sender_addr == DAISY_ADDR_MASTER)	// we sent the packet
			|| (receive_addr == DAISY_ADDR_COUNT))	// addr count on the way
			{
		// packet is for us to use, act now!
		daisyPacketReceived(receive_addr, sender_addr, data, data_length);
	}

	else {
		uartCobsTransmit(frame, length);
		// packet is not for us, retransmit
		// can only happen if we are not the master or if
		// slave to slave sending is implemented
	}
//	daisyPacketReceived(receive_addr, sender_addr, data, data_length);

#else  // this is the slave handler
	if (receive_addr == DAISY_ADDR_BROADCAST) {
		// packet is for us and everyone else
		daisyPacketReceived(receive_addr, sender_addr, data, data_length);
	} else if (receive_addr == daisy_address && receive_addr != DAISY_ADDR_UNSET) {
		// packet is only for us, do not retransmit
		daisyPacketReceived(receive_addr, sender_addr, data, data_length);
		return;
	} else if (receive_addr == DAISY_ADDR_COUNT) {
		// second byte of the data frame (sender_address)
		// contains our new address, save, increment, resend
		daisy_address = ++sender_addr;
		frame[1] = sender_addr;
	}

	uartCobsTransmit(frame, length);

#endif
}

static inline uint32_t crc_check(uint8_t *data, size_t length) {
	uint32_t crc_received = 0, crc_calculated = 0;
	crc_received = ((uint32_t) data[length - 2]) << 8
			| ((uint32_t) data[length - 1]);
	CRC_SW_CalculateCRC(&CRC_SW_0, data, length - 2);

	crc_calculated = CRC_SW_GetCRCResult(&CRC_SW_0);

	return crc_calculated - crc_received;
}
