#ifndef MAILSLOT_CONF_H
#define MAILSLOT_CONF_H

#define DEBUG 1
#define MOD_NAME "MailSlots"
#define VERSION "0.1"

#define DEVICE_NAME "mailslotdev"
#define DEVICE_MAJOR 0 /* use 0 if want to get assigned an available major */

#define MAIL_INSTANCES 256
#define MINOR_RANGE_BOTTOM 0
#define MINOR_RANGE_TOP (MINOR_RANGE_BOTTOM + MAIL_INSTANCES)

#define MESS_MAX_SIZE 32
#define MAIL_INSTANCE_MAX_STORAGE 30


#endif
