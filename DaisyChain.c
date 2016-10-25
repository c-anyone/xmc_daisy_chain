/*
 * DaisyChain.c
 *
 *  Created on: Jul 6, 2016
 *      Author: Fabio Pungg
 */

#include <DAVE.h>
#include "DaisyChain.h"

/*
 *
 */
#ifdef DAISY_MASTER_DEVICE
static uint8_t daisy_address = DAISY_ADDR_MASTER;
#else
static uint8_t daisy_address = DAISY_STARTING_ADDRESS;
#endif

/**
 *  transmission buffer, filled with data and passed to uartCobsTransmit
 */
static uint8_t framebuf[64];

/**
 * pointer to Dave UART App configuration struct
 */
static UART_CONFIG_t *uart_config;

/**
 * Inline function for checking the CRC of a received packet
 * @param data ptr to the buffer to calculate the crc from
 * @param size buffer size
 */
static inline uint32_t crc_check(uint8_t *data, size_t size);

void daisyInit(UART_CONFIG_t *uartConfig) {
	if (uartConfig == NULL)
		return;
	uart_config = uartConfig;
	/*
	 * Initialize the USIC channel used by uartCobs
	 */
	uartCobsInit(uart_config->channel);
}

void daisyWorker(void) {
	uartCobsPoll();
}

uint8_t daisyGetAddress() {
	return daisy_address;
}

void daisySendData(uint8_t receiver, uint8_t sender, uint8_t *data,
		size_t length) {
	uint32_t crc = 0;
	size_t i = 0;

	/**
	 * the first to bytes amount to the addresses
	 */
	framebuf[i++] = receiver;
	framebuf[i++] = sender;
	while (i < (length + 2)) {
		framebuf[i] = data[i - 2];
		++i;
	}

	CRC_SW_CalculateCRC(&CRC_SW_0, framebuf, length + 2);
	crc = CRC_SW_GetCRCResult(&CRC_SW_0);

	/**
	 * the last two bytes amouont to the crc checksum
	 */
	framebuf[i++] = (uint8_t) (crc >> 8) & 0xff;
	framebuf[i++] = (uint8_t) (crc & 0xff);

	/**
	 * call transmission function
	 */
	uartCobsTransmit(framebuf, i);
}

/**
 * Implementation of the receive callback. This function is called
 * from uartCobsPoll on correct reception of a frame
 * @param frame pointer to the data frame
 * @param length size of the framebuf
 */
void uartCobsFrameReceived(uint8_t *frame, size_t length) {
	uint8_t receive_addr = 0;
	uint8_t sender_addr = 0;
	uint8_t *data = NULL;
	size_t data_length = 0;
	if (length < DAISY_MIN_PACKET_SIZE || length > DAISY_MAX_PACKET_SIZE) {
		/**
		 * not a correct daisy packet, fail silent
		 */
		return;
	}
	if (crc_check(frame, length) != 0) {
		// crc incorrect, broken data, fail silent
		return;
	}
	/**
	 * first byte is the receive address
	 */
	receive_addr = frame[0];
	/**
	 * second byte is the sender address
	 */
	sender_addr = frame[1];
	/**
	 * data starts at 3rd byte
	 */
	data = frame + 2;
	// data length is the frame length-addresses-crc bytes
	data_length = length - 4;

#ifdef DAISY_MASTER_DEVICE
	/**
	 *  packets are processed differently on master and slave
	 */
	if ((receive_addr == DAISY_ADDR_BROADCAST) // broadcasts stop here
			|| (receive_addr == DAISY_ADDR_MASTER)// packet for us
			|| (sender_addr == DAISY_ADDR_MASTER)// we sent the packet
			|| (receive_addr == DAISY_ADDR_COUNT))// addr count on the way
	{
		// packet is for us to use, act now!
		daisyPacketReceived(receive_addr, sender_addr, data, data_length);
	}

	else {
		// uartCobsTransmit(frame, length);
		// packet is not for us, retransmit
		// can only happen if we are not the master or if
		// slave to slave sending is implemented
		return;
	}

#else
	/**
	 *  this is the slave handler, it retransmits broadcasts, sets the address
	 *  on daisy addr count and does not retransmit by itself if it was the specified
	 *  device
	 */

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
	/*
	 * retransmit frame, including crc calculation
	 */
	daisySendData(receive_addr, sender_addr, data, data_length);

#endif
}

static inline uint32_t crc_check(uint8_t *data, size_t size) {
	uint32_t crc_received = 0, crc_calculated = 0;
	crc_received = ((uint32_t) data[size - 2]) << 8
			| ((uint32_t) data[size - 1]);
	CRC_SW_CalculateCRC(&CRC_SW_0, data, size - 2);

	crc_calculated = CRC_SW_GetCRCResult(&CRC_SW_0);

	return crc_calculated - crc_received;
}
