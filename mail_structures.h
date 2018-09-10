#ifndef MAIL_STRUCTURES_H
#define MAIL_STRUCTURES_H

#include <linux/list.h>

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

#endif
