#ifndef RTOS_h
#define RTOS_h

#define MAX_TASKS 20

extern int task_time[MAX_TASKS];
extern unsigned long last_task_time[MAX_TASKS];

void add_task (int time_, unsigned int index);
void remove_task(unsigned int index);
void set_task_time(int time_, unsigned int index);

#endif
