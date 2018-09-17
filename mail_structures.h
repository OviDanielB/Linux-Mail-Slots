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
  int max_mess_size;           /* max mess size for specific mail instance */
  int n_mess;                  /* number of messages present */
  int size;                    /* storage occupied by all messages */
  int minor;                   /* minor number associated with mailslot */
  struct semaphore sem;        /* mutual exclusion semaphore */
  wait_queue_head_t rq, wq;    /* read and write wait queues */
} mailslot;

typedef struct mail_instances {
  mailslot instances[MAIL_INSTANCES];

} mail_instances;

typedef struct session_opt {
  int bl_r;
  int bl_w;
} session_opt;

#endif
