/*
 * DaisyChain.h
 *
 *  Created on: Jul 6, 2016
 *      Author: Fabio Pungg
 *
 *	This module needs uart_cobs.h and a configured and initialized UART app to function and makes
 *	use of a CRC app it assumes to be called CRC_SW_0.
 *	Daisy packets are stuffed by Consistent Overhead Byte Stuffing as described in
 *	http://conferences.sigcomm.org/sigcomm/1997/papers/p062.pdf
 *
 */

#ifndef DAISYCHAIN_H_
#define DAISYCHAIN_H_

#include "uart_cobs.h"

/*
 * reserved addresses
 */
// BROADCAST packets will be passed on from slave to slave and always end at the master
#define DAISY_ADDR_BROADCAST			(0xffu)
// UNSET is the initial address of a slave, before address enumeration takes place
#define DAISY_ADDR_UNSET				(0xefu)
// COUNT forces a slave to set it's address to the one found in sender field and passes the incremented
// 		 value on to the next daisychained device
#define DAISY_ADDR_COUNT				(0x7fu)
// ERROR unused for now
#define DAISY_ADDR_ERROR				(0x8fu)
// MASTER address of the master
#define DAISY_ADDR_MASTER				(0x00u)

//	this amounts to 2 address bytes and 2 crc  bytes
#define DAISY_MIN_PACKET_SIZE 		(4U)
#define DAISY_MAX_PACKET_SIZE (COBS_MAX_FRAME_SIZE-DAISY_MIN_PACKET_SIZE)



#ifdef DAISY_MASTER_DEVICE
#define DAISY_STARTING_ADDRESS DAISY_MASTER_DEVICE
#else
#define DAISY_STARTING_ADDRESS DAISY_ADDR_UNSET
#endif


/*
 * */
 //typedef void (*daisyRxCallback_t)(uint8_t,uint8_t,uint8_t*);

/**
 * Sets the pointer to the USIC channel handle for use by the UART
 * the channel is assumed to be initialized
 * @param uartConfig pointer to USIC channel handle
 */
void daisyInit(UART_CONFIG_t *uartConfig);

/**
 * @param receiver	address of the message receiver Messages always end
 * @param sender	address of the message sender
 * @param data		pointer to the buffer containing the packet data
 * @param size		number of bytes to send
 */
void daisySendData(uint8_t receiver,uint8_t sender,uint8_t* data, size_t size);

/**
 * Worker function that has to be called in the main loop
 * continually calls uartCobsPoll and calls daisyPacketReceived on frame
 * reception.
 */
void daisyWorker(void);

/**
 * returns the address of the daisy device it's running on
 */
uint8_t daisyGetAddress(void);

/**
 * This function will be called on reception of a valid daisy packet. On Slave devices
 * it will be called if receive_address matches either the address appointed by the address
 * setup or DAISY_ADDR_BROADCAST
 */
extern void daisyPacketReceived(uint8_t receive_address,uint8_t sender_address, uint8_t *buf, size_t size);

#endif /* DAISYCHAIN_H_ */
