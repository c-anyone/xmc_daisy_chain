/*
 * DaisyChain.h
 *
 *  Created on: Jul 6, 2016
 *      Author: Fabio Pungg
 */

#ifndef DAISYCHAIN_H_
#define DAISYCHAIN_H_

/*
 * special addresses for different purposes
 */
#define DAISY_ADDR_BROADCAST			(0xffu)
#define DAISY_ADDR_UNSET				(0xefu)
#define DAISY_ADDR_COUNT				(0x7fu)
#define DAISY_ADDR_ERROR				(0x8fu)
#define DAISY_ADDR_MASTER				(0x00u)


#ifdef DAISY_MASTER_DEVICE
#define DAISY_STARTING_ADDRESS DAISY_MASTER_DEVICE
#else
#define DAISY_STARTING_ADDRESS DAISY_ADDR_UNSET
#endif


 typedef enum {
 DAISY_NONE,
 DAISY_AUTO_DISCOVER,
 DAISY_PING,
 DAISY_SET_ALL,
 DAISY_RESET_ALL,
 DAISY_SET_SINGLE
 } DAISY_CHAIN_COMMANDS_t;

// typedef for data receive callback function
 typedef void (*daisyRxCallback_t)(uint8_t,uint8_t,uint8_t*);

void daisyInit(UART_CONFIG_t *uartConfig);

void daisySendData(uint8_t receiver,uint8_t sender,uint8_t* data, size_t length);

void daisySetRxCallback(daisyRxCallback_t);

void daisyWorker(void);

void daisyPacketReceived(uint8_t receive_address,uint8_t sender_address, uint8_t *buf, size_t length);

#endif /* DAISYCHAIN_H_ */
