/*

  Copyright (c) 2015 Martin Sustrik

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom
  the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

#include <stdint.h>
#include <sys/param.h>

#include "cr.h"
#include "libmill.h"
#include "list.h"
#include "poller.h"
#include "timer.h"

/* Forward declarations for the functions implemented by specific poller
   mechanisms (poll, epoll, kqueue). */
void mill_poller_init(void);

/* If 1, mill_poller_init was already called. */
static int mill_poller_initialised = 0;

#define check_poller_initialised() \
do {\
    if(mill_slow(!mill_poller_initialised)) {\
        errno = 0;\
        mill_assert(errno == 0);\
        mill_main.fd = -1;\
        mill_main.timer.expiry = -1;\
        mill_poller_initialised = 1;\
    }\
} while(0)

static void mill_poller_callback(struct mill_timer *timer) {
    struct mill_cr *cr = mill_cont(timer, struct mill_cr, timer);
    mill_resume(cr, -1);
}

/* Pause current coroutine for a specified time interval. */
void mill_msleep_(int64_t deadline, const char *current) {
    check_poller_initialised();
    /* If required, start waiting for the timeout. */
    if(deadline >= 0)
        mill_timer_add(&mill_running->timer, deadline, mill_poller_callback);
    /* Do actual waiting. */
    mill_running->state = MILL_MSLEEP;
    mill_running->fd = -1;
    mill_running->events = 0;
    mill_set_current(&mill_running->debug, current);
    int rc = mill_suspend();
    /* Handle file descriptor events. */
    if(rc >= 0) {
        mill_assert(!mill_timer_enabled(&mill_running->timer));
        return;
    }
    /* Handle the timeout. */
    mill_assert(mill_running->fd == -1);
    return;
}

void mill_wait(int block) {
    check_poller_initialised();
    while(1) {
        /* Compute timeout for the subsequent poll. */
        int timeout = block ? mill_timer_next() : 0;
        /* Fire all expired timers. */
        int timer_fired = mill_timer_fire();
        /* Never retry the poll in non-blocking mode. */
        if(!block || timer_fired)
            break;
        /* If timeout was hit but there were no expired timers do the poll
           again. This should not happen in theory but let's be ready for the
           case when the system timers are not precise. */
    }
}