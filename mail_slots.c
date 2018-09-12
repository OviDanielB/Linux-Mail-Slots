
#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/slab.h>		     /* For kmalloc */
#include <linux/fs.h>
#include <linux/wait.h>        /* For wait queues */
#include <linux/semaphore.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>	     /* for put_user */

#include "utils/log.h"
#include "utils/conf.h"
#include "mail_structures.h"
#include "mailslot.h"


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

/* maximum message size; default is in conf.h. can be changed with ioctl */
static int max_mess_size = MESS_MAX_SIZE;
/* max storage for every mailslot instance */
static int mail_max_storage = MAIL_INSTANCE_MAX_STORAGE;

static int blocking_read = MS_BLOCK_ENABLED;
static int blocking_write = MS_BLOCK_ENABLED;


long ioctl_mail(struct file *file,
  unsigned int cmd, unsigned long arg){
    int err = 0, ret = 0;
    /*
     * If command type is different than this driver's or its
     * number doesn't exist, return ENOTTY (inappropriate ioctl)
     */
    if( _IOC_TYPE(cmd) != MS_IOC_MAGIC ) return -ENOTTY;
    if( _IOC_NR(cmd) > MS_IOC_MAX_NUM ) return -ENOTTY;

    /* ioctl direction is on the user's perspective */
    if( _IOC_DIR(cmd) & _IOC_READ)
      /* check if the location pointed to can be written */
      err = !access_ok(VERIFIY_WRITE, (void *)arg, _IOC_SIZE(cmd));
    if( _IOC_DIR(cmd) & _IOC_WRITE)
      /* check if the location pointed to can be read */
      err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
    if(err){
      log_error("User space address passed to ioctl is invalid");
      return -EFAULT;
    }

    switch (cmd) {

      case MS_IOC_SMESS_SIZE:
        log_debug("Called MAILSLOT_IOC_SMESS_SIZE");
        ret = __get_user(max_mess_size, (int *) arg);
        printk(KERN_INFO "Max mess size set to %d\n", max_mess_size);
        break;

      case MS_IOC_GMESS_SIZE:
        log_debug("Called MAILSLOT_IOC_GMESS_SIZE");
        ret = __put_user(max_mess_size, (int *) arg);
        break;

      case MS_IOC_SBLOCKING_READ:
        log_debug("Called MS_IOC_SBLOCKING_READ");
        ret = __get_user(blocking_read, (int *) arg);
        break;

      case MS_IOC_GBLOCKING_READ:
        log_debug("Called MS_IOC_GBLOCKING_READ");
        ret = __put_user(blocking_read, (int *) arg);
        break;

      case MS_IOC_SBLOCKING_WRITE:
        log_debug("Called MS_IOC_SBLOCKING_WRITE");
        ret = __get_user(blocking_write, (int *) arg);
        break;

      case MS_IOC_GBLOCKING_WRITE:
        log_debug("Called MS_IOC_GBLOCKING_WRITE");
        ret = __put_user(blocking_write, (int *) arg);
        break;

      default:
        ret = -ENOTTY;
    }
    return ret;
  }

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

  log_debug("Message filled");
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
  f_space = mail_max_storage - mail->size;
  if(f_space <= 0){
    /* if max_storage changed dynamically with ioctl, f_space can be < 0 */
    log_dev_err(Major, mail->minor, "Not enough space to write message");
    return 0;
  }
  return f_space;
}

int mess_params_compliant(int len){
  if(len > max_mess_size){
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

  log_debug("WRITE trying sem");
  if(down_interruptible(&mail->sem))
    return -ERESTARTSYS;

  log_debug("WRITE got sem");
  while(free_space(mail) == 0){  /* mail full */
    up(&mail->sem);
    log_debug("WRITE mail full");
    log_debug("WRITE waiting on event");
    if( wait_event_interruptible(mail->wq, free_space(mail) > 0))
      return -ERESTARTSYS;  /* signal: tell the fs layer to handle it */
    log_debug("WRITE woke from queue");
    log_debug("WRITE trying sem cycle");
    if( down_interruptible(&mail->sem) )
      return -ERESTARTSYS;
    log_debug("WRITE got sem cycle");
  }

  mess = kmalloc(sizeof(message), GFP_KERNEL);
  if(!mess){
    log_alert("Failed message struct alloc");
    return -ENOMEM;
  }
  log_debug("Message created");

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

void delete_mess_from_mail(message *mess, mailslot *mail){
  log_debug("Deleting node");
  log_debug(mess->content);
  /* delete and reconstruct list */
  list_del_init(&mess->list);
  mail->size = mail->size - mess->len;

  log_debug("New list without deleted element");
  message *m;
  list_for_each_entry(m, &mail->mess_list, list){
    log_debug(m->content);
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

    log_debug("READ trying sem");
    if( down_interruptible(&mail->sem))
      return -ERESTARTSYS;
    log_debug("READ got sem");
    //while(mess_to_read(mail) == 0){
    while(list_empty(&mail->mess_list)){
      log_debug("READ mail empty");
      up(&mail->sem);
      /* blocking if happens */
      log_debug("READ waiting");
      if( wait_event_interruptible(mail->rq, !list_empty(&mail->mess_list) ))
        return -ERESTARTSYS;
      log_debug("READ woke from queue ");
      log_debug("READ trying sem cycle");
      if( down_interruptible(&mail->sem))
        return -ERESTARTSYS;
      log_debug("READ got sem cycle");
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

    delete_mess_from_mail(mess, mail);

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
  .unlocked_ioctl = ioctl_mail,
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
