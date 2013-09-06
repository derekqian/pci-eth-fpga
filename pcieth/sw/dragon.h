#ifndef DRAGONDEV_H
#define DRAGONDEV_H

#include <linux/ioctl.h>

// _IO(MAGIC, SEQ_NO)
// _IOW(MAGIC, SEQ_NO, TYPE)

/*
 * sample code:
 * typedef struct
 * {
 *     int status, dignity, ego;
 * } query_arg_t;
 * #define QUERY_GET_VARIABLES _IOR('q', 1, query_arg_t *)
 * #define QUERY_CLR_VARIABLES _IO('q', 2)
 * #define QUERY_SET_VARIABLES _IOW('q', 3, query_arg_t *)
*/

/* 
 * The major device number.
 */
#define MAGIC_NUM 100

#define MODE_IO 0
#define MODE_MEM 1
#define IOCTL_GET_MODE _IOR(MAGIC_NUM, 0, unsigned int)
#define IOCTL_SET_MODE _IOW(MAGIC_NUM, 1, unsigned int)
#define IOCTL_GET_ID _IOR(MAGIC_NUM, 2, unsigned int)
#define IOCTL_GET_LED _IOR(MAGIC_NUM, 3, unsigned int)
#define IOCTL_SET_LED _IOW(MAGIC_NUM, 4, unsigned int)
#define IOCTL_GET_STATUS _IOR(MAGIC_NUM, 5, unsigned int)
#define IOCTL_SET_STATUS _IOW(MAGIC_NUM, 6, unsigned int)
#define IOCTL_GET_SWITCH _IOR(MAGIC_NUM, 7, unsigned int)
#define IOCTL_SET_LOW _IOW(MAGIC_NUM, 8, unsigned int)
#define IOCTL_GET_LOW _IOW(MAGIC_NUM, 9, unsigned int)


#endif
