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
  atomic_t len;
  spinlock_t lock;

} mailslot;

typedef struct mail_instances {
  mailslot instances[MAIL_INSTANCES];
  atomic_t n_occupied;

} mail_instances;

#endif
