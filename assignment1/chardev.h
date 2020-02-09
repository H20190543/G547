#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>


#define MAGIC 'E'
#define CHANNEL_NUMBER 0
#define ALLIGN 1
#define CHANNEL _IOW(MAGIC, CHANNEL_NUMBER, int )
#define ALLIGNMENT _IOW(MAGIC, ALLIGN, int)

#endif
