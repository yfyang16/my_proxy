#include "sbuf.h"


void sbuf_init(sbuf_t *sp, int n) {
    sp->n = n;
    sp->buf = Malloc(n * sizeof(int));
    sp->front = -1;
    sp->rear = 0;

    sem_init(sp->mutex, 0, 1);
    sem_init(sp->slots, 0, n);
    sem_init(sp->items, 0, 0);
}

void sbuf_deinit(sbuf_t *sp) {
	Free(sp);
}

void sbuf_insert(sbuf_t *sp, int item) {
	// If there is no available slots, wait here.
	P(&(sp->slots));

		// Else we wait for the buf operation because there may be other thread operating
		// the buffer currently.
		P(&(sp->mutex));
	    
	    sp->buf[(sp->rear) % (sp->n)] = item;
	    sp->rear ++;

		V(&(sp->mutex));

	// When the item was inserted into the buffer, items ++
	V(&(sp->items));
}


int sbuf_remove(sbuf_t *sp) {
    int _fd;

	// See if there is any existing item
	P (&(sp->items));


		P(&(sp->mutex));

	    _fd = sp->buf[(sp->front + 1) % (sp->n)];
	    sp->front ++;

		V(&(sp->mutex));

	V(&(sp->slots)); // one more available slot

	return _fd;
}