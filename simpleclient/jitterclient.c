/*
 * jitterclient.c: Simple client to communicate to jitter server using TCP.
 */
#include <stdio.h>  // www.cplusplus.com/reference/cstdio/
#include <stdlib.h> // www.cplusplus.com/reference/cstdlib/
#include <string.h> // www.cplusplus.com/reference/cstring/
#include <time.h>   // www.cplusplus.com/reference/ctime/
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// May use CLOCK_MONOTONIC, CLOCK_REALTIME, CLOCK_PROCESS_CPUTIME_ID etc.
#define CLOCKTYPE CLOCK_REALTIME

#define SERVER_PORT 7000
//#define SERVER_ADDR "192.168.1.12"
#define SERVER_ADDR "127.0.0.1"
#define PACKET_SIZE 50
#define MAX_COUNT 1000
#define SLEEP_INTERVAL 20
#define PRIORITY 0

//#define DEBUG

long long timespecDiff(struct timespec *timeA, struct timespec *timeB)
{
  return ((timeA->tv_sec * (long long)1000000000) + timeA->tv_nsec) -
    ((timeB->tv_sec * (long long)1000000000) + timeB->tv_nsec);
}

int main(int argc, char *argv[])
{
  struct timespec timeBase, timeNow;
  long millis;
  int sockfd, count, flag;
  struct sockaddr_in addrServ;
  char buffLong[PACKET_SIZE], *label = "Connecting to: ";

  clock_gettime(CLOCKTYPE, &timeBase);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (0 > sockfd)
  {
    fprintf(stderr, "Error establishing socket.\n");
    exit(1);
  }

  flag = 1;
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

  bzero((char *) &addrServ, sizeof(addrServ));
  addrServ.sin_family = AF_INET;
  addrServ.sin_addr.s_addr = inet_addr(SERVER_ADDR); // Should use inet_aton
  addrServ.sin_port = htons(SERVER_PORT);

  // Set up a new connection
  if (0 != connect(sockfd, (struct sockaddr *) &addrServ, sizeof(addrServ)))
  {
    fprintf(stderr, "Error connecting to server at address %s on port %d.\n", 
     SERVER_ADDR, SERVER_PORT);
    exit(1);
  }

  // Put the length of the remaining buffer at the start of the buffer
  buffLong[0] = (char)((PACKET_SIZE - 2) % 256);
  buffLong[1] = (char)((PACKET_SIZE - 2) / 256);
  
  // Send packets in a loop
  for (count = 0; count < MAX_COUNT; count++)
  {
    printf("Sending message #%d\n", count);
    fflush(stdout);

    clock_gettime(CLOCKTYPE, &timeNow);
    millis = (long)(timeNow.tv_sec * 1000 + timeNow.tv_nsec / 1000000);

    sprintf(buffLong + 2, "%s%d:%ld", label, count, millis);
    write(sockfd, buffLong, sizeof(buffLong));

    sleep(SLEEP_INTERVAL);
  }
  close(sockfd);

  return 0;
}

