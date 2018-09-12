
#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/slab.h>		     /* For kmalloc */
#include <linux/fs.h>
#include <linux/wait.h>        /* For wait queues */
#include <linux/semaphore.h>
#include <asm/uaccess.h>	     /* for put_user */

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
  log_dev(Major, minor, "Opened");
  return 0;
}

int release_mail(struct inode *node, struct file *filp){
  int minor;
  log_debug("Release");
  minor = MINOR(node->i_rdev);
  log_dev(Major, minor, "Released");
  return 0;
}

mailslot *mail_of(int minor){
  return &(instances.instances[minor]);
}

int fill_message(message *mess, const char *buff, int len){
  size_t m_len;

  if(buff[len-1] == '\0'){
    m_len = len;
  } else {
    m_len = len + 1;
  }

  mess->content = kmalloc(sizeof(char) * m_len, GFP_KERNEL);
  if(!mess->content){
    log_alert("Failed message content alloc");
    return -ENOMEM;
  }

  if(copy_from_user(mess->content, buff, len)){
    log_error("Copy from user");
    return -EFAULT;
  }
  mess->content[m_len - 1] = '\0';
  mess->len = m_len;

  log_debug("Message created");
  return 0;
}

void add_mess_to_mail(mailslot *mail, message *mess){
  int new_size;

  //printk(KERN_INFO "%d %s\n", 0, mess->content);
  list_add_tail(&mess->list, &mail->mess_list);
  new_size = mail->size + mess->len;
  mail->size = new_size;
  mail->n_mess++;

  int j = 0;
  message *m;
  list_for_each_entry(m, &mail->mess_list, list){
    printk(KERN_INFO "%d %s\n", j, m->content);
    j++;
  }
}

int free_space(mailslot *mail){
  int f_space;
  f_space = MAIL_INSTANCE_MAX_STORAGE - mail->size;
  if(f_space <= 0){ /* if max_storage changed dynamically, f_space can be < 0 */
    log_dev_err(Major, mail->minor, "Not enough space to write message");
    return 0;
  }
  return f_space;
}

int mess_params_compliant(int len){
  if(len > MESS_MAX_SIZE){
    log_dev_err(Major, -1 , "Trying to write message bigger than MESS_MAX_SIZE");
    return 0;
  }
  return 1;
}

ssize_t write_mail(struct file *filp,
  const char *buff, size_t len, loff_t *off){

  int minor, ret;
  mailslot *mail;
  message *mess;

  if(!mess_params_compliant(len)){
    return -EOVERFLOW;   /* mess too big */
  }

  log_debug("Write");
  minor = iminor(filp->f_path.dentry->d_inode);
  mail = mail_of(minor);

  if(down_interruptible(&mail->sem))
    return -ERESTARTSYS;

  while(free_space(mail) == 0){  /* mail full */
    up(&mail->sem);

    if( wait_event_interruptible(mail->wq, free_space(mail) > 0))
      return -ERESTARTSYS;  /* signal: tell the fs layer to handle it */

    if( down_interruptible(&mail->sem))
      return -ERESTARTSYS;
  }

  mess = kmalloc(sizeof(message), GFP_KERNEL);
  if(!mess){
    log_alert("Failed message struct alloc");
    return -ENOMEM;
  }

  ret = fill_message(mess,buff,len);
  if(ret){
    up(&mail->sem);
    return ret;
  }
  add_mess_to_mail(mail, mess);
  up(&mail->sem);

  /* wake up any reader */
  wake_up_interruptible(&mail->rq);

  return len;
}

int mess_to_read(mailslot *mail){
  return mail->n_mess;
}

message *first_message(mailslot *mail){
  message *m;
  list_for_each_entry(m, &mail->mess_list, list){
    log_debug("Getting first message in list");
    log_debug(m->content);
    return m; /* first element */
  }
}

ssize_t read_mail(struct file *filp,
  char *buff, size_t len, loff_t *off){
    int minor;
    mailslot *mail;
    message *mess;

    log_debug("Read");
    minor = iminor(filp->f_path.dentry->d_inode);
    mail = mail_of(minor);

    if( down_interruptible(&mail->sem))
      return -ERESTARTSYS;

    //while(mess_to_read(mail) == 0){
    while(list_empty(&mail->mess_list)){
      up(&mail->sem);
      /* blocking if happens */
      if( wait_event_interruptible(mail->rq, !list_empty(&mail->mess_list) ))
        return -ERESTARTSYS;

      if( down_interruptible(&mail->sem))
        return -ERESTARTSYS;
    }

    mess = first_message(mail);
    if(len < mess->len){
      up(&mail->sem);
      /* wake other readers and see if their request can be satisfied */
      wake_up_interruptible(&mail->rq);
      return -EINVAL;
    }

    if( copy_to_user(buff, mess->content, mess->len) ){
      up(&mail->sem);
      return -EFAULT;
    }

    log_debug("Deleting node");
    log_debug(mess->content);

    list_del_init(&mess->list);

    log_debug("New list without deleted element");
    message *m;
    list_for_each_entry(m, &mail->mess_list, list){
      log_debug(m->content);
      return m; /* first element */
    }

    up(&mail->sem);
    /* wake up writers */
    wake_up_interruptible(&mail->wq);

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
      mail = mail_of(i);
      mail->minor = i;
      mail->size = 0;
      mail->n_mess = 0;
      INIT_LIST_HEAD(&mail->mess_list);
      sema_init(&mail->sem, 1);           /* initially unlocked */
      init_waitqueue_head(&mail->rq);
      init_waitqueue_head(&mail->wq);
  }
  atomic_set(&(instances.n_occupied), 0);


  log_info("Module ready");
  return 0;
}

void __exit cleanup_module(void){

  unregister_chrdev(Major, DEVICE_NAME);
  log_dev(Major, -1, "Unregistered successfully");
  log_info("Module released");
}
