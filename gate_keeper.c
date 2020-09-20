/**
 * This program is designed to reduce the amount of brute force attacks on ssh.
 *
 * Copyright Panu Simolin
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "gate.h"

#define MAXLINE 2048
#define EXPIRATION 33  // TODO: make configurable

/* Check that the amount of args works. When not print usage. */
int check_args(int argc, char *argv[], struct gate *dest)
{
  if (geteuid() != 0)
    {
      printf("\tYou must be root for this to work!\n");
      return -1;
    }

  if (argc == 3 || argc == 2)
    {
      printf("Implement the configuration file\n");
      return -3;
    }
  else if (argc == 5 || argc == 4)
    {
      dest->port_1.no = atoi(argv[1]);
      dest->port_2.no = atoi(argv[2]);
      dest->port_3.no = atoi(argv[3]);
      return 0;
    }

  print_usage(argv[0]);
  return -1;
}

/**
 * Forks the process. The child process will be replaced with service ssh
 * process image
 *
 * @param the gate context
 * @param operation to be done to the sshd (start/stop)
 *
 * Return 1 when successful, 0 otherwise
 */
static inline void change_state_sshd(struct gate *my_gate, char *operation)
{
  int ret;
  pid_t child_pid;

  printf("%s\n", operation);
  child_pid = fork();
  if (child_pid == 0)
    {
      if (strncmp(operation, "stop", 4) == 0)
        {
          execl("/usr/sbin/service", "service", "ssh", "stop", NULL);
        }
      else if (strncmp(operation, "start", 4) == 0)
        {
          execl("/usr/sbin/service", "service", "ssh", "start", NULL);
        }
      return;
  }

  child_pid = waitpid(child_pid, &ret, 0);
  printf("child pid: %d\n", child_pid);
  if (WIFEXITED(ret))
    {
      printf("sshd %s successful\n", operation);
    }
  else
    {
      printf("Problems with sshd %s\n", operation);
    }

}

static inline void start_sshd(struct gate *my_gate)
{
  change_state_sshd(my_gate, "start");
  my_gate->port_open = true;
}

static inline void stop_sshd(struct gate *my_gate)
{
  change_state_sshd(my_gate, "stop");
  my_gate->port_open = false;
}

/* Bind the current port with the included port number. */
bool assign_ipv6_addr_port(struct port_data *current_port)
{
  int ret;
  struct sockaddr_in6 serv_addr;

  current_port->sock_fd_6 = socket(AF_INET6, SOCK_DGRAM, 0);
  if (current_port->sock_fd_6 < 0)
    {
      perror("IPv6 socket creation failed");
      return false;
    }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin6_family = AF_INET6;
  serv_addr.sin6_port = htons(current_port->no);

  ret = bind(current_port->sock_fd_6, (struct sockaddr *) &serv_addr,
	     sizeof(serv_addr));
  if (ret < 0)
    {
      perror("IPv6 bind failed");
      return false;
    }

  return true;
}

/* Bind the current port with the included port number. */
static bool assign_ipv4_addr_port(struct port_data *current_port)
{
  int ret;
  struct sockaddr_in serv_addr;

  current_port->sock_fd_4 = socket(AF_INET, SOCK_DGRAM, 0);
  if (current_port->sock_fd_4 < 0)
    {
      perror("socket creation failed");
      return false;
    }

  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(current_port->no);

  ret = bind(current_port->sock_fd_4, (struct sockaddr *) &serv_addr,
	     sizeof(serv_addr));
  if (ret < 0)
    {
      perror("bind failed");
      return false;
    }

  return true;
}

static bool preconditions_met(struct gate *my_gate)
{
  /* check that no timestamp is zero  */
  if (my_gate->port_1.timestamp == 0 ||
      my_gate->port_2.timestamp == 0 ||
      my_gate->port_3.timestamp == 0)
    {
      printf("At least one port has not been accessed.\n");
      return false;
    }

  /* checking that the gates were accessed in the correct order */
  if (my_gate->port_1.timestamp > my_gate->port_2.timestamp ||
      my_gate->port_2.timestamp > my_gate->port_3.timestamp)
    {
      printf("ports accessed in wrong order\n");
      return false;
    }

  return true;
}

/**
 * Checks for the preconditions. When those are met forks the process. The child
 * will execute service ssh start and shut down.
 *
 * @param the gate context
 */
static void check_and_start_service(struct gate *my_gate)
{
  if (preconditions_met(my_gate))
    {
      if (my_gate->port_open)
        {
          printf("service already running..\n");
          return;
        }
      printf("starting...\n");
      start_sshd(my_gate);
    }
  else
    {
      printf("conditions failed\n");

      // Close the SSH to prevent accidental openings
      if (my_gate->port_open)
        {
          stop_sshd(my_gate);
        }

      printf("continuing operation\n");
    }
}

static void handle_incoming_conn(fd_set *rfds, struct gate *my_gate)
{
  struct sockaddr_in og_sender;
  struct sockaddr_in second_sender;
  socklen_t addr_len;
  char buf[MAXLINE];

  addr_len = sizeof(og_sender);
  if (FD_ISSET(my_gate->port_1.sock_fd_4, rfds))
    {
      printf("hit my first port\n");
      my_gate->port_1.timestamp = clock();
      printf("timestamp: %ld\n", my_gate->port_1.timestamp);
      recvfrom(my_gate->port_1.sock_fd_4, buf, MAXLINE, 0,
	       (struct sockaddr *)&og_sender, &addr_len);
    }

  if (FD_ISSET(my_gate->port_2.sock_fd_4, rfds))
    {
      printf("hit my second port\n");
      recvfrom(my_gate->port_2.sock_fd_4, buf, MAXLINE, 0,
	       (struct sockaddr *)&second_sender, &addr_len);

      /* Update the time stamp only when the sender matches to the OG */
      if (second_sender.sin_family == og_sender.sin_family &&
	        second_sender.sin_addr.s_addr == og_sender.sin_addr.s_addr)
        {
          my_gate->port_2.timestamp = clock();
        }
      else
        {
          printf("the sender addresses do not match\n");
        }
    }

  if (FD_ISSET(my_gate->port_3.sock_fd_4, rfds))
    {
      printf("hit my third port\n");
      recvfrom(my_gate->port_3.sock_fd_4, buf, MAXLINE, 0,
	             (struct sockaddr *)&second_sender, &addr_len);
      /* Update the time stamp only when the sender matches to the OG */
      if (second_sender.sin_family == og_sender.sin_family &&
	        second_sender.sin_addr.s_addr == og_sender.sin_addr.s_addr)
        {
          my_gate->port_3.timestamp = clock();
        }
      else
        {
          printf("the sender addresses do not match\n");
        }

      check_and_start_service(my_gate);
    }
}

bool daemonize()
{
  int pid;

  pid = fork();
  if (pid != 0)
    {
      return true;
    }

  pid = fork();
  if (pid != 0)
    {
      return true;
    }

  /* Change the working directory to the root. */
  chdir("/");

  fclose(stdout);
  fclose(stderr);

  return false;
}

int main(int argc, char *argv[])
{
  int ret;
  bool running = true;

  fd_set rfds;
  struct gate my_gate = {};
  struct timeval tv;

  my_gate.port_open = false;
  ret = check_args(argc, argv, &my_gate);
  if (ret != 0)
    {
      return ret;
    }

  if (argc == 5 && daemonize())
    {
      printf("Fork successful\n");
      return 0;
    }

  if (!assign_ipv4_addr_port(&my_gate.port_1))
    {
      return -1;
    }

  if (!assign_ipv4_addr_port(&my_gate.port_2))
    {
      return -1;
    }

  if (!assign_ipv4_addr_port(&my_gate.port_3))
    {
      return -1;
    }

  while(running)
    {
      tv.tv_sec = EXPIRATION;
      tv.tv_usec = 0;
      FD_ZERO(&rfds);
      FD_SET(my_gate.port_1.sock_fd_4, &rfds);
      FD_SET(my_gate.port_2.sock_fd_4, &rfds);
      FD_SET(my_gate.port_3.sock_fd_4, &rfds);

      ret = select((my_gate.port_3.sock_fd_4 + 2), &rfds, NULL, NULL, &tv);
      if (ret == -1)
        {
          perror("select()");
          return -1;
        }
      else if (ret)
        {
          handle_incoming_conn(&rfds, &my_gate);
        }
      else
        {
          printf("timeout: ");
          if (my_gate.port_open)
            {
              printf("closing gate\n");
              stop_sshd(&my_gate);
            }
          else
            {
              printf("gate already closed\n");
            }

          my_gate.port_1.timestamp = 0;
          my_gate.port_2.timestamp = 0;
          my_gate.port_3.timestamp = 0;

	    }

    }

  return 0;
}
