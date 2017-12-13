// Copyright 2015, 2016 by Peter Ohler, All Rights Reserved

#ifndef __OPO_QUEUE_H__
#define __OPO_QUEUE_H__

#include <stdatomic.h>
#include <stdbool.h>

#include "val.h"

typedef struct _Queue {
    opoMsg		*q;
    opoMsg		*end;
    _Atomic(opoMsg*)	head;
    _Atomic(opoMsg*)	tail;
    atomic_flag		pop_lock; // set to true when ppo in progress
    atomic_int		wait_state;
    int			rsock;
    int			wsock;
} *Queue;

extern void	queue_init(Queue q, size_t qsize);

extern void	queue_cleanup(Queue q);
extern void	queue_push(Queue q, opoMsg item);
extern opoMsg	queue_pop(Queue q, double timeout);
extern bool	queue_empty(Queue q);
extern int	queue_count(Queue q);

#endif /* __OPO_QUEUE_H__ */
