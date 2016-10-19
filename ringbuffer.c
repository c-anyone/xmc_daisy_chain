#include "ringbuffer.h"
#include <stdio.h>
#include <string.h>

void rb_init(ring_buffer *rb)
{
  if(rb==NULL)
	return;

  (void)memset(rb->buffer,0,RBUFSIZE);

  rb->bufferend = (rb->buffer + RBUFSIZE);
  rb->count = 0;
  rb->head=rb->tail=rb->buffer; //initialize, head and tail both point to beginning of block

  return;
}

/*
	stores an element at the head of the ringbuffer
*/


void rb_push(ring_buffer *rb,uint8_t value)
{
  if(rb->count >= RBUFSIZE) {
		return;
	} //error handling here

  rb->count++;
  
  if(rb->head == rb->bufferend){
	rb->head = rb->buffer;
}

  *(rb->head) = value;
  rb->head+=sizeof(uint8_t);
}

/*
	retrieves an element from the tail of the ringbuffer
*/
uint8_t rb_pop(ring_buffer *rb)
{
  uint8_t value;

  if(rb==NULL)
	return 0;
	
  if(rb->count==0)
	return 0;
	
 rb->count--;

  value = *(rb->tail);	// fetch the value from the buffer
  *(rb->tail)=0;		// put a zero in place of the value
  rb->tail += sizeof(uint8_t);

  return value;
}
