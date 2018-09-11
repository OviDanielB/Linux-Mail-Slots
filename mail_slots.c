
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

static int Major;
static mail_instances instances;

int open_mail(struct inode *node, struct file *filp){
  int minor;

  log_debug("Open");
  minor = MINOR(node->i_rdev);
  log_dev(Major, minor, "opened");
  return 0;
}

int release_mail(struct inode *node, struct file *filp){
  int minor;
  log_debug("Release");
  minor = MINOR(node->i_rdev);
  log_dev(Major, minor, "released");
  return 0;
}

ssize_t write_mail(struct file *filp,
  const char *buff, size_t len, loff_t *off){
  int minor;
  mailslot *mail;
  message *mess;

  log_debug("Write");
  minor = iminor(filp->f_path.dentry->d_inode);

  if(len > MESS_MAX_SIZE){
    log_dev_err(Major, minor, "Trying to write message bigger than MAX_SIZE");
    return 0;
  }

  mail = &(instances.instances[minor]);
  mess = kmalloc(sizeof(message), GFP_KERNEL);
  if(!mess){
    log_alert("Failed message struct alloc");
    return 0;
  }

  mess->content = kmalloc(sizeof(char) * MESS_MAX_SIZE, GFP_KERNEL);
  if(!mess->content){
    log_alert("Failed message content alloc");
    return 0;
  }

  copy_from_user(mess->content, buff, len);
  log_info(mess->content);

  return len;

}

ssize_t read_mail(struct file *filp,
  char *bugg, size_t len, loff_t *off){
    int minor;

    log_debug("Read");
    minor = iminor(filp->f_path.dentry->d_inode);
    return len;
}


static struct file_operations fops = {
  .read = read_mail,
  .write = write_mail,
  .open = open_mail,
  .release = release_mail,
};
/* more portable solution */
//SET_MODULE_OWNER(&fops);

int __init init_module(void){
  int i;
  mailslot *mail;
  log_info("Module init");

  Major = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &fops);
  if(Major < 0){
    log_dev(DEVICE_MAJOR, -1, "registration failed");
    log_alert("Module not registered");
    return 1;
  }
  log_dev(Major, -1, "registration succedded");

  for(i = 0; i < MAIL_INSTANCES; i++){
      mail = &(instances.instances[i]);
      atomic_set(&(mail->len), 0);
      INIT_LIST_HEAD(&mail->mess_list);
  }

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

  unregister_chrdev(Major, DEVICE_NAME);
  log_dev(Major, -1, "Unregistered successfully");
  log_info("Module released");
}
