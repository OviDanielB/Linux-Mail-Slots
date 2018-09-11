
#ifndef MAILSLOTS_UTILS_H
#define MAILSLOTS_UTILS_H

#include <linux/kernel.h>
#include "conf.h"

void log_info(char *msg);
void log_error(char *msg);
void log_warning(char *msg);
void log_debug(char *msg);
void log_alert(char *msg);
void log_dev(int maj, int min, char *msg);
void log_dev_err(int maj, int min, char *msg);

#endif
