
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

static int max_instances = MAIL_INSTANCES;
static int minor_bott = MINOR_RANGE_BOTTOM;
static int minor_top = MINOR_RANGE_TOP;

/*module_param(max_instances, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(myint, "Max number of mailslot instances handled by the driver"); */

module_param(minor_bott, int, S_IRUSR | S_IRGRP );
MODULE_PARM_DESC(minor_bott, "Driver minor number range bottom");

module_param(minor_top, int, S_IRUSR | S_IRGRP );
MODULE_PARM_DESC(minor_top, "Driver minor number range top");

static int Major;
static mail_instances instances;

/* maximum message size; default is in conf.h. can be changed with ioctl */
static int max_mess_size = MESS_MAX_SIZE;
/* max storage for every mailslot instance */
static int mail_max_storage = MAIL_INSTANCE_MAX_STORAGE;

static int blocking_read = MS_BLOCK_ENABLED;
static int blocking_write = MS_BLOCK_ENABLED;

mailslot *mail_of(int minor){
  return &(instances.instances[minor - minor_bott]);
}

int dev_minor(struct file *filp){
  return iminor(filp->f_path.dentry->d_inode);
}

int minor_ok(int minor){
  if(minor >= minor_bott && minor <= minor_top)
    return 1;
  return 0;
}

int is_read_blocking(struct file *filp){
  session_opt *so;

  so = (session_opt *) filp->private_data;
  if(!so){
    log_error("Session options are null");
    return MS_BLOCK_ENABLED;       /* return default blocking mode */
  }
  return so->bl_r;
}

int is_write_blocking(struct file *filp){
  session_opt *so;

  so = (session_opt *) filp->private_data;
  if(!so){
    log_error("Session options are null");
    return MS_BLOCK_ENABLED;       /* return default blocking mode */
  }
  return so->bl_w;
}

void print_mess_list(struct list_head *head){
  int j = 0;
  message *m;
  list_for_each_entry(m, head, list){
    printk(KERN_INFO " pos: %d - cont: %s - len: %d\n", j, m->content, m->len);
    j++;
  }
}

long ioctl_mail(struct file *file,
  unsigned int cmd, unsigned long arg){

    int err = 0, ret = 0, minor, tmp_mess_size, tmp_blocking;
    mailslot *mail;
    session_opt *so;
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

    minor = dev_minor(file);
    mail = mail_of(minor);
    so = (session_opt *) file->private_data;

    switch (cmd) {

      case MS_IOC_SMESS_SIZE:
        log_debug("Called MAILSLOT_IOC_SMESS_SIZE");
        ret = __get_user(tmp_mess_size, (int *) arg);
        if(ret) return ret;
        if(tmp_mess_size <= 0){
          log_dev_err(Major, minor, "Trying to set max mess size <= 0");
          ret = -EINVAL;
          break;
        }
        if(down_interruptible(&mail->sem))
          return -ERESTARTSYS;
        mail->max_mess_size = tmp_mess_size;
        printk(KERN_INFO "Max mess size set to %d for mail instance \n", mail->max_mess_size, minor);
        up(&mail->sem);
        break;

      case MS_IOC_GMESS_SIZE:
        log_debug("Called MAILSLOT_IOC_GMESS_SIZE");
        if(down_interruptible(&mail->sem))
          return -ERESTARTSYS;
        ret = __put_user(mail->max_mess_size, (int *) arg);
        up(&mail->sem);
        break;

      case MS_IOC_SBLOCKING_READ:
        log_debug("Called MS_IOC_SBLOCKING_READ");
        ret = __get_user(tmp_blocking, (int *) arg);
        if(ret) return ret;
        if(ret != 0 && ret != 1){
          log_dev_err(Major,minor, "Read blocking mode should be a boolean");
          ret = -EINVAL;
          break;
        }
        so->bl_r = tmp_blocking;
        printk(KERN_INFO "Read blocking mode enabled is now %d (boolean).\n", so->bl_r);
        break;

      case MS_IOC_GBLOCKING_READ:
        log_debug("Called MS_IOC_GBLOCKING_READ");
        ret = __put_user(so->bl_r, (int *) arg);
        break;

      case MS_IOC_SBLOCKING_WRITE:
        log_debug("Called MS_IOC_SBLOCKING_WRITE");
        ret = __get_user(tmp_blocking, (int *) arg);
        if(ret) return ret;
        if(ret != 0 && ret != 1){
          log_dev_err(Major,minor, "Write blocking mode should be a boolean");
          ret = -EINVAL;
          break;
        }
        so->bl_w = tmp_blocking;
        printk(KERN_INFO "Write blocking mode enabled is now %d (boolean).\n", so->bl_w);
        break;

      case MS_IOC_GBLOCKING_WRITE:
        log_debug("Called MS_IOC_GBLOCKING_WRITE");
        ret = __put_user(so->bl_w, (int *) arg);
        break;

      default:
        ret = -ENOTTY;
    }
    return ret;
  }

int open_mail(struct inode *node, struct file *filp){
  int minor, occupied_instances;
  mailslot *mail;
  session_opt *so;

  log_debug("Open");
  minor = MINOR(node->i_rdev);

  if(!minor_ok(minor)){
    log_error("Trying to open device with minor not in correct range");
    return -EINVAL;
  }

  mail = mail_of(minor);

  so = kmalloc(sizeof(session_opt), GFP_KERNEL);
  if(!so){
    log_dev_err(Major, minor, "Allocating file_options struct");
    return -ENOMEM;
  }

  if(filp->f_flags & O_NONBLOCK){
    log_debug("Dev opened with O_NONBLOCK");
    so->bl_r = 0; so->bl_w = 0;
  } else {
    log_debug("Dev opened with blocking mode");
    so->bl_r = 1; so->bl_w = 1;
  }
  /* set session options */
  filp->private_data = so;

  log_dev(Major, minor, "Opened successfully");
  return 0;
}

int release_mail(struct inode *node, struct file *filp){
  int minor;
  session_opt *so;

  log_debug("Release");
  minor = MINOR(node->i_rdev);
  so = (session_opt *) filp->private_data;
  if(!so){
    /* debug purpose */
    log_dev_err(Major, minor, "Session options should not be null");
  } else {
    log_debug("Freeing session_opt struct");
    /* private data won't be freed by kernel . manual freeing required */
    kfree(so);
  }

  log_dev(Major, minor, "Released successfully");
  return 0;
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

  list_add_tail(&mess->list, &mail->mess_list);
  new_size = mail->size + mess->len;
  mail->size = new_size;
  mail->n_mess++;

  print_mess_list(&mail->mess_list);
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

message *alloc_mess(void){
  message *mess;
  mess = kmalloc(sizeof(message), GFP_KERNEL);
  if(!mess){
    log_alert("Failed message struct alloc");
    return NULL;
  }
  log_debug("Message created");
  return mess;
}

/* free previously allocated kernel memory */
void dealloc_mess(message *mess){
  kfree(mess->content);
  kfree(mess);
}


ssize_t write_mail(struct file *filp,
  const char *buff, size_t len, loff_t *off){

  int ret;
  mailslot *mail;
  message *mess;

  if(!mess_params_compliant(len)){
    return -EOVERFLOW;   /* mess too big */
  }

  log_debug("Write");
  mail = mail_of(dev_minor(filp));

  /* message creation and copy from user buffer is done
   * before taking the semaphore;
   * it would occupy the mailslot without reason
   */
  mess = alloc_mess();
  if(!mess)
    return -ENOMEM;

  ret = fill_message(mess,buff,len);
  if(ret)
    return ret;


  if(down_interruptible(&mail->sem))
    return -ERESTARTSYS;

  while(free_space(mail) == 0){  /* mail full */
    up(&mail->sem);

    if(is_write_blocking(filp))
      return -EAGAIN;

    if(wait_event_interruptible(mail->wq, free_space(mail) > 0))
      return -ERESTARTSYS;  /* signal: tell the fs layer to handle it */

    if(down_interruptible(&mail->sem))
      return -ERESTARTSYS;

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

void detach_mess_from_mail(message *mess, mailslot *mail){
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
    mailslot *mail;
    message *mess;

    mail = mail_of(dev_minor(filp));

    if(down_interruptible(&mail->sem))
      return -ERESTARTSYS;

    while(list_empty(&mail->mess_list)){
      up(&mail->sem);
      /* blocking if happens */
      if(is_read_blocking(filp))
        return -EAGAIN;
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
    detach_mess_from_mail(mess, mail);
    up(&mail->sem);
    /* wake up writers */
    wake_up_interruptible(&mail->wq);
    /* copy mess to user after releasing sempahore to allow
     * others to take it faster
     */
    if(copy_to_user(buff, mess->content, mess->len) ){
      up(&mail->sem);
      return -EFAULT;
    }
    dealloc_mess(mess);

    return mess->len;
}


static struct file_operations fops = {
  .read =           read_mail,
  .write =          write_mail,
  .open =           open_mail,
  .release =        release_mail,
  .unlocked_ioctl = ioctl_mail,
  .owner =          THIS_MODULE,
};
/* more portable solution */
//SET_MODULE_OWNER(fops);

int __init init_module(void){
  int i, range;
  mailslot *mail;
  log_info("Module init");

  if(minor_bott < 0 || minor_top < 0 || max_instances < 0){
    log_error("No MailSlot module param can be negative");
    return -EINVAL;
  }
  range = minor_top - minor_bott;
  if(range <= 0 ){
    log_error("Minor range can't <= 0");
    return -EINVAL;
  }

  /* max num of handled instances is TOP - BOTTOM ;
   * if params not passed to module init , defult config values
   * are used
   */
  max_instances = range;

  printk(KERN_INFO "%s-%s: Device minor number ranges from %d to %d with %d max instances.\n", MOD_NAME,VERSION,minor_bott, minor_top, max_instances);

  Major = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &fops);
  if(Major < 0){
    log_dev(DEVICE_MAJOR, -1, "Registration failed");
    log_alert("Module not registered");
    return 1;
  }
  log_dev(Major, -1, "Registration succedded");

  for(i = 0; i < max_instances; i++){
      mail = mail_of(i);
      mail->minor = i;
      mail->size = 0;
      mail->n_mess = 0;
      mail->max_mess_size = MESS_MAX_SIZE;    /* default max value used */
      INIT_LIST_HEAD(&mail->mess_list);
      sema_init(&mail->sem, 1);               /* initially unlocked */
      init_waitqueue_head(&mail->rq);
      init_waitqueue_head(&mail->wq);
  }


  log_info("Module ready");
  return 0;
}

void __exit cleanup_module(void){

  unregister_chrdev(Major, DEVICE_NAME);
  log_dev(Major, -1, "Unregistered successfully");
  log_info("Module released");
}
