#ifndef MAIL_STRUCTURES_H
#define MAIL_STRUCTURES_H

#include <linux/list.h>
#include "utils/conf.h"

typedef struct message {
  char *content;
  int len;
  struct list_head list;

} message;

typedef struct mailslot {
  struct list_head mess_list;
  int max_mess_size, max_storage;           /* max mess size for specific mail instance */
  int size;                                 /* storage occupied by all messages */
  int minor;                                /* minor number associated with mailslot */
  struct semaphore sem;                     /* mutual exclusion semaphore */
  wait_queue_head_t rq, wq;                 /* read and write wait queues */
  struct list_head fifo_r, fifo_w;
} mailslot;

typedef struct fifo_task {
  struct task_struct *task;
  struct list_head list;
} fifo_task;

typedef struct mail_instances {
  mailslot instances[MAIL_INSTANCES];

} mail_instances;

typedef struct session_opt {
  int bl_r;
  int bl_w;
} session_opt;



#endif
