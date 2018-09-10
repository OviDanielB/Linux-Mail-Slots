
#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/slab.h>		/* For kmalloc */
#include <linux/fs.h>
#include <asm/uaccess.h>	/* for put_user */

#include "utils/log.h"
#include "utils/conf.h"
#include "mail_structures.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ovidiu Daniel Barba <ovi.daniel.b@gmail.com>");
MODULE_DESCRIPTION("Brief Modules Description");

static int myint = 0;
static char *string = "";
static int myArray[2] = {-1, +1};
static int arrlen = 0;

module_param(myint, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(myint, "Visible and writable int");
module_param(string, charp, 0000);
MODULE_PARM_DESC(string, "Hidden string");

module_param_array(myArray, int, &arrlen, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(myArray, "Array of ints");

static int major;

static struct file_operations fops = {
  .read = NULL,
  .write = NULL,
  .open = NULL,
  .release = NULL
};

int __init init_module(void){
  log_info("Module init");

  major = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &fops);
  if(major < 0){
    log_alert("Registering char device failed.");
    log_alert("Module not registered");
    return 1;
  }
  printk(KERN_INFO "Device successfully registered with MAJOR %d.\n", major);

  mailslot *m = kmalloc(sizeof(mailslot), GFP_KERNEL);
  if(!m){
    log_error("Error in allocating maislot");
    return 1;
  }
  INIT_LIST_HEAD(&m->mess_list);

  message m1 = {
    .content = "Hello",
    .len = 1,
  };

  message m2 = {
    .content = "World",
    .len = 2,
  };

  message m3 = {
    .content = "I'm Ovi",
    .len = 2,
  };

  list_add_tail(&m1.list, &m->mess_list);
  list_add_tail(&m2.list, &m->mess_list);
  list_add_tail(&m3.list, &m->mess_list);

  message *mess;
  list_for_each_entry(mess, &m->mess_list, list){
    log_info(mess->content);
  }

  log_info("Module ready");
  return 0;
}

void __exit cleanup_module(void){

  unregister_chrdev(major, DEVICE_NAME);
  log_info("Device unregistered successfully");
  log_info("Module released");
}
