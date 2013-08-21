/*
 * Test program for Dragon PCI driver
 *
 * Pierre Ficheux (pierre.ficheux@openwide.fr)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>

main (int ac, char **av)
{
  int fd, i, nw, nr, ndata, inc = 0x11111111;
  unsigned int addr = 0, data = 0x11111111, *buf;

  if (ac <= 3) {
    fprintf (stderr, "Usage: %s dragon_device_name addr count [data inc]\n", av[0]);
    exit (1);
  }

  if ((fd = open (av[1], O_RDWR)) < 0) {
    perror (av[1]);
    exit (1);
  }

  // Address (32 bits)
  sscanf (av[2], "%x", &addr);
  addr *= 4;

  // # of data (32 bits)
  ndata = atoi(av[3]);
  
  // Initial value to write
  if (ac >= 5)
    sscanf (av[4], "%x", &data);

  // Increment
  if (ac >= 6)
    sscanf (av[5], "%x", &inc);

  printf ("Write+read %d long-word(s) starting @ 0x%08X, value is 0x%08X, increment is 0x%08X\n\n", ndata, addr, data, inc);

  // Build datas
  if ((buf = (unsigned int *) malloc (ndata * sizeof(unsigned int))) == NULL) {
    perror ("malloc");
    exit (1);
  }

  for (i = 0 ; i < ndata ; i++) {
    *(buf+i) = data;
    printf ("buf[%d] = 0x%08X\n", i, *(buf+i));
    data += inc;
  }

  // Write datas
  lseek (fd, addr, SEEK_SET);
  nw = write (fd, (char *)buf, ndata * sizeof(unsigned int));
  printf ("Wrote %d chars @%08X\n\n", nw, addr);

  // Read datas
  lseek (fd, addr, SEEK_SET);
  nr = read (fd, (char *)buf, nw);
  printf ("Read %d chars @%08X\n", nr, addr);

  for (i = 0 ; i < nr / sizeof (unsigned int) ; i++)
    printf ("buf[%d] = 0x%08X\n", i, *(buf+i));


  free (buf);

  close (fd);
}
