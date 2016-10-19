/* Implements a simple ringbuffer
	elements get pushed to head and are
	popped from tail.
*/


#ifndef RING_BUFF_H
#define RING_BUFF_H

/*
#define CTS PE2
#define RTS PE3
#define UART_PORT PORTE
*/

#include <stdint.h>

#define RBUFSIZE 64

typedef struct ring_buffer
{
	uint8_t buffer[RBUFSIZE];
	uint8_t *bufferend;
	uint8_t count;
	uint8_t *head;
	uint8_t *tail;
} ring_buffer;

extern void rb_init(ring_buffer *rb);
extern void rb_push(ring_buffer *rb,uint8_t value);
extern uint8_t rb_pop(ring_buffer *rb);


#endif
