#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include "dragon.h"

int main (int ac, char **av) {
  int fd, nw, nr;
  unsigned int value = 0;
  int mode;

  if (ac < 3) {
    fprintf (stderr, "Usage: %s device --switch [value]\n", av[0]);
    goto clean_open;
  }

  if ((fd = open (av[1], O_RDWR)) < 0) {
    fprintf (stderr, "Open %s fail\n", av[1]);
    goto clean_open;
  }

  if (strcmp(av[2], "--get-mode") == 0) {
    ioctl(fd, IOCTL_GET_MODE, &mode);
    printf ("Current mode: %s\n\n", mode?"MEM":"IO");
  } else if (strcmp(av[2], "--set-mode") == 0) {
    // Initial value to write
    if (ac < 4) {
      fprintf (stderr, "Need parameter for write\n");
      goto clean_operate;
    }
    sscanf (av[3], "%d", &mode);
    ioctl(fd, IOCTL_SET_MODE, &mode);
    printf ("Change to mod: %s\n\n", mode?"MEM":"IO");
  } else if (strcmp(av[2], "--write") == 0) {
    // Initial value to write
    if (ac < 4) {
      fprintf (stderr, "Need parameter for write\n");
      goto clean_operate;
    }
    sscanf (av[3], "%x", &value);
    // Write values
    lseek (fd, 0, SEEK_SET);
    nw = write (fd, (char *)&value, sizeof(unsigned int));
    printf ("Wrote 0x%08X\n\n", value);
  } else if (strcmp(av[2], "--read") == 0) {
    // Read values
    lseek (fd, 0, SEEK_SET);
    nr = read (fd, (char *)&value, sizeof(unsigned int));
    printf ("Read 0x%08X\n\n", value);
  } else if (strcmp(av[2], "--get-id") == 0) {
    ioctl(fd, IOCTL_GET_ID, &value);
    printf ("ID: 0x%08X\n\n", value);
  } if (strcmp(av[2], "--get-led") == 0) {
    ioctl(fd, IOCTL_GET_LED, &value);
    printf ("Get LED: 0x%08X\n\n", value);
  } else if (strcmp(av[2], "--set-led") == 0) {
    // Initial value to write
    if (ac < 4) {
      fprintf (stderr, "Need parameter for set-led\n");
      goto clean_operate;
    }
    sscanf (av[3], "%x", &value);
    ioctl(fd, IOCTL_SET_LED, &value);
    printf ("Set LED: 0x%08X\n\n", value);
  } if (strcmp(av[2], "--get-status") == 0) {
    ioctl(fd, IOCTL_GET_STATUS, &value);
    printf ("Get STATUS: 0x%08X\n\n", value);
  } else if (strcmp(av[2], "--set-status") == 0) {
    // Initial value to write
    if (ac < 4) {
      fprintf (stderr, "Need parameter for set-status\n");
      goto clean_operate;
    }
    sscanf (av[3], "%x", &value);
    ioctl(fd, IOCTL_SET_STATUS, &value);
    printf ("Set STATUS: 0x%08X\n\n", value);
  } else if (strcmp(av[2], "--get-switch") == 0) {
    ioctl(fd, IOCTL_GET_SWITCH, &value);
    printf ("Get SWITCH: 0x%08X\n\n", value);
  } else if (strcmp(av[2], "--set-low") == 0) {
    // Initial value to write
    if (ac < 4) {
      fprintf (stderr, "Need parameter for set-low\n");
      goto clean_operate;
    }
    sscanf (av[3], "%x", &value);
    ioctl(fd, IOCTL_SET_LOW, &value);
    printf ("Set LOW: 0x%08X\n\n", value);
  } else if (strcmp(av[2], "--get-low") == 0) {
    ioctl(fd, IOCTL_GET_LOW, &value);
    printf ("Get LOW: 0x%08X\n\n", value);
  }

clean_operate:
  close (fd);
clean_open:
  return 1;
}
