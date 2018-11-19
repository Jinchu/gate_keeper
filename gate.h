/**
 * This program is designed to reduce the amount of brute force attacks on ssh.
 *
 * Copyright Panu Simolin
 */

#include <stdio.h>
#include <time.h>
#include <stdbool.h>

struct port_data {
  int no;
  int sock_fd_4;  //for IPv4 connections
  int sock_fd_6;  //for IPv6 connections
  clock_t timestamp;
};

/* Contains the ports in the order the user has to knock on them. */
struct gate {
  bool port_open;
  struct port_data port_1;
  struct port_data port_2;
  struct port_data port_3;
};

static inline void print_usage(char *program)
{
  printf("\nUSAGE: sudo %s <configuration path>\n", program);
  printf("USAGE: sudo %s <port 1> <port 2> <port 3>\n\n", program);
  printf("\t <configuration path> defines a path to configuration file\n");
  printf("\t <port 1> defines the first port to be knocked on\n");
  printf("\t <port 2> defines the second port\n");
  printf("\t <port 3> defines the third port to knock\n\n");
}

int check_args(int argc, char *argv[], struct gate *dest);
