/*
 * uart_cobs.h
 *
 *  Created on: Oct 20, 2016
 *      Author: Faebsn
 *
 * Implementation of a simple UART handler using constant overhead byte stuffing. It
 * implements UART framing with a constant overhead of 2 byte. A '\0' Byte in the stream  always
 * represents the end of a frame. If the data contains a zero byte, it is stuffed with a
 * counter value till the next zero bytes arrives.
 * The first byte of a cobs frame is a phantom zero, indicating the position of the
 * first stuffed zero byte.
 *
 * uartCobsTransmitIRQ has to be setup in the uart_config app as the transmission
 * interrupt routine
 */

#ifndef UART_COBS_H_
#define UART_COBS_H_

/**
 * maximum size of a cobs frame, arbitrarily choosen to keep the uart handler simple
 */
#define COBS_MAX_FRAME_SIZE (32u)


/**
 * Starts the UART and saves a pointer to the USIC handle
 * @param uartConfig pointer to the initialized USIC channel
 */
void uartCobsInit(XMC_USIC_CH_t *usic_config);

/**
 * function to poll the receive FIFO of the USIC channel. Calls
 * uartCobsFrameReceived, which has to be implemented by the upper
 * transmission layer.
 */
void uartCobsPoll(void);

/**
 * Transmits a COBS frame via UART
 * The data is stuffed into a local transmission buffer
 */
void uartCobsTransmit(uint8_t *data, size_t size);


extern void uartCobsFrameReceived(uint8_t *frame,size_t size);

#endif /* UART_COBS_H_ */
