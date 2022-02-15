#include "RTOS.h"

int task_time[MAX_TASKS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned long last_task_time[MAX_TASKS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void add_task(int time_, unsigned int index){
  task_time[index] = time_;
  last_task_time[index] = 0;
}

void remove_task(unsigned int index){
  task_time[index] = 0;
  last_task_time[index] = 0;
}

void set_task_time(int time_, unsigned int index){
  if(task_time[index] != 0) task_time[index] = time_;
}
