#define _POSIX_C_SOURCE 200809L
/**
 * Just a quick client side program for the gate keeper project.
 *
 * Copyright Panu Simolin
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define PORT_CONF_SIZE 4

int main(int argc, char *argv[])
{
  struct timespec interval;

  int sock_fd, nBytes;
  int ret;
  char buffer[1024] = "FFFF\n";
  int port_conf[PORT_CONF_SIZE];
  int current_port = 0;
  int i = 0;
  struct sockaddr_in server_addr, my_addr;
  socklen_t addr_size;

  if (argc < 5)
    {
      printf("TODO: help\n");
      return -1;
    }

  for (i = 0; i < 3; i++)
    {
      port_conf[i] = atoi(argv[i + 2]);
    }
  port_conf[PORT_CONF_SIZE - 1] = 0;

  /*Create UDP socket*/
  sock_fd = socket(PF_INET, SOCK_DGRAM, 0);

  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  /*Configure settings in address struct*/
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(7891);
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);

  memset((char *)&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  my_addr.sin_port = htons(0);

  if (bind(sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
      perror("cannot bind");
      return 0;
    }
  printf("Bind successful\n");

  /* Initialize size variable to be used later on*/
  addr_size = sizeof server_addr;

  /* Set interval to 0.1 sec -> 100 000 000 ns */
  interval.tv_sec = 0;
  interval.tv_nsec = 100000000;

  i = 0;
  for (current_port = port_conf[i]; current_port != 0;
       current_port = port_conf[++i])
    {
      printf("You typed: %s",buffer);
      server_addr.sin_port = htons(current_port);

      nBytes = strlen(buffer) + 1;
      /*Send message to server*/
      ret = sendto(sock_fd, buffer, nBytes, 0,
                  (struct sockaddr *)&server_addr, addr_size);
      if (ret == -1)
        {
          perror("Failed to send");
          break;
        }
      printf("ret: %d\n", ret);
      nanosleep(&interval, NULL);
    }

  return 0;
}
