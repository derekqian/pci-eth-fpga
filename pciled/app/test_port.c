/*
 * This code does nothing useful, just a port write, a pause,
 * and a port read. Compile with `gcc -O2 -o test_port test_port.c',
 * and run as root with `./test_port'.
 */

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
// #include <asm/io.h>
#include <sys/io.h>

#define BASEPORT 0x200

int kbhit() {
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

int main()
{
  /* Get access to the ports */
  if (ioperm(BASEPORT, 1, 1)) {
    perror("ioperm"); 
    exit(1);
  }

  printf("Press the keyboard to quit!\n");
  while(!kbhit()) {
    /* Set the data signals (D0-7) of the port to all low (0) */
    outb(0, BASEPORT);
    usleep(1000000);

    /* Set the data signals (D0-7) of the port to all low (0) */
    outb(1, BASEPORT);
    usleep(1000000);
  }

  /* Read from the status port (BASE+1) and display the result */
  printf("status: %d\n", inb(BASEPORT));

  /* We don't need the ports anymore */
  if (ioperm(BASEPORT, 3, 0)) {
    perror("ioperm"); 
    exit(1);
  }

  exit(0);
}
