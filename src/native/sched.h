#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_SCHED_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_SCHED_H_

typedef struct { int sched_priority; } sched_param;

extern int sched_yield();

#endif
