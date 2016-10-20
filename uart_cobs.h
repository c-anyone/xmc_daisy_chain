/*
 * uart_cobs.h
 *
 *  Created on: Oct 20, 2016
 *      Author: Faebsn
 */

#ifndef UART_COBS_H_
#define UART_COBS_H_

#define MAX_FRAME_SIZE 64 // including frame-delimiter

void uartCobsInit(XMC_USIC_CH_t *uartConfig);
void pollUartCobs(void);
void uart_cobs_transmit(uint8_t *data, size_t length);


extern void uartCobsFrameReceived(uint8_t *frame,size_t length);

#endif /* UART_COBS_H_ */
