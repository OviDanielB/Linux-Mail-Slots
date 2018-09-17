
#define MS_IOC_MAGIC '4'

#define MS_IOC_RESET _IO(MAILSLOT_IOCTL_MAGIC, 0)

/*
 * After _IOC_:
 *  -  S is set value
 *  -  G is get value
 */
#define MS_IOC_SBLOCKING_READ     _IOW(MS_IOC_MAGIC, 1, int)
#define MS_IOC_GBLOCKING_READ     _IOR(MS_IOC_MAGIC, 2, int)

#define MS_IOC_SBLOCKING_WRITE    _IOW(MS_IOC_MAGIC, 3, int)
#define MS_IOC_GBLOCKING_WRITE    _IOR(MS_IOC_MAGIC, 4, int)

#define MS_IOC_SMESS_SIZE         _IOW(MS_IOC_MAGIC, 5, int)
#define MS_IOC_GMESS_SIZE         _IOR(MS_IOC_MAGIC, 6, int)

#define MS_IOC_SMAX_STORAGE       _IOW(MS_IOC_MAGIC, 7, int)
#define MS_IOC_GMAX_STORAGE       _IOR(MS_IOC_MAGIC, 8, int)

/* used to check incoming commands */
#define MS_IOC_MAX_NUM            8

/* blocking or non blocking I/O */
#define MS_BLOCK_ENABLED          1
#define MS_BLOCK_DISABLED         0
