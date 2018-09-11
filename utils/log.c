#include "log.h"


void log_info(char *msg){
  printk(KERN_INFO "%s-%s: %s.\n", MOD_NAME, VERSION, msg );
}

void log_warning(char *msg){
  printk(KERN_WARNING "%s-%s: %s.\n", MOD_NAME, VERSION, msg );
}

void log_error(char *msg){
  printk(KERN_ERR "%s-%s: %s.\n", MOD_NAME, VERSION, msg );
}

void log_debug(char *msg){
  if(DEBUG){
    printk(KERN_DEBUG "%s-%s: %s.\n", MOD_NAME, VERSION, msg );
  }
}

void log_alert(char *msg){
  printk(KERN_ALERT "%s-%s: %s.\n", MOD_NAME, VERSION, msg );
}


void log_dev(int maj, int min, char *msg){
  if(min < 0){
    printk(KERN_INFO "%s-%s: Device (Maj: %d) %s.\n", MOD_NAME, VERSION, maj, msg );
  } else {
    printk(KERN_INFO "%s-%s: Device (Maj: %d,min: %d) %s.\n", MOD_NAME, VERSION, maj, min, msg );
  }
}

void log_dev_err(int maj, int min, char *msg){
  if(min < 0){
    printk(KERN_ERR "%s-%s: Device (Maj: %d) %s.\n", MOD_NAME, VERSION, maj, msg );
  } else {
    printk(KERN_ERR "%s-%s: Device (Maj: %d,min: %d) %s.\n", MOD_NAME, VERSION, maj, min, msg );
  }
}
