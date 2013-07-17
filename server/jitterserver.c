/*
 * jitterserver.c: Simple server to log timestamps of inbound packets.
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

//#define DEBUG

long long timespecDiff(struct timespec *timeA, struct timespec *timeB)
{
  return ((timeA->tv_sec * (long long)1000000000) + timeA->tv_nsec) -
    ((timeB->tv_sec * (long long)1000000000) + timeB->tv_nsec);
}

int main(int argc, char *argv[])
{
  struct timespec timeBase, timeNow;
  long long delta, deltaPrevious;
  int sockfd, sockfdClient, buffLen, readLen, readIdx, count, port;
  struct sockaddr_in addrServ, addrClient;
  socklen_t addrClientLen;
  char buff[2], *buffLong, label[21];

  clock_gettime(CLOCKTYPE, &timeBase);

  port = SERVER_PORT;
  if (2 <= argc)
  {
    port = atoi(argv[1]);
    if (0 >= port)
    {
      port = SERVER_PORT;
    }
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (0 > sockfd)
  {
    fprintf(stderr, "Error establishing socket.\n");
    exit(1);
  }

  bzero((char *) &addrServ, sizeof(addrServ));
  addrServ.sin_family = AF_INET;
  addrServ.sin_addr.s_addr = INADDR_ANY;
  addrServ.sin_port = htons(port);
  if (0 > bind(sockfd, (struct sockaddr *) &addrServ, sizeof(addrServ)))
  {
    fprintf(stderr, "Error binding socket to server port %d.\n", port);
    exit(1);
  }

  listen(sockfd, 5);
  printf("SERVER LISTENING ON PORT %d\n", port);
  fflush(stdout);

  // Enter loop accepting new connections
  for (count = 0;; count++)
  {
    addrClientLen = sizeof(addrClient);
    sockfdClient = accept(sockfd, (struct sockaddr *) &addrClient, &addrClientLen);
    if (0 > sockfdClient)
    {
      close(sockfd);
      fprintf(stderr, "Error accepting connection from port %d.\n", port);
      exit(1);
    }
  
    printf("NEW CONNECTION (%d)\n", count);
    fflush(stdout);
    deltaPrevious = -1;
    // Enter loop handling packets from this connection
    for (;;)
    {
      readLen = read(sockfdClient, buff, sizeof(buff));
      if (0 == readLen)
      {
        break;
      }
      if (0 > readLen)
      {
        close(sockfdClient);
        close(sockfd);
        fprintf(stderr, "Error reading from connection on port %d.\n", port);
        exit(1);
      }
#ifdef DEBUG
      printf("Read %d bytes\n", readLen);
#endif
      buffLen = (unsigned int)buff[0] + 256 * (unsigned int)buff[1];
#ifdef DEBUG
      printf("Allocating %d bytes\n", buffLen);
      fflush(stdout);
#endif
      buffLong = (char *) malloc((unsigned int)buffLen);
      if (NULL == buffLong)
      {
        close(sockfdClient);
        close(sockfd);
        fprintf(stderr, "Error allocating buffer for %d bytes.\n", buffLen);
        exit(1);
      }
      for (readIdx = 0; readIdx < buffLen; readIdx += readLen)
      {
        readLen = read(sockfdClient, buffLong + readIdx, buffLen - readIdx);
        if (0 == readLen)
        {
          break;
        } else if (0 > readLen)
        {
          close(sockfdClient);
          close(sockfd);
          fprintf(stderr, "Error reading from connection on port %d.\n", port);
          exit(1);
        }
      }
      if (0 == readLen)
      {
        break;
      }
      if (readIdx > sizeof(label) - 1)
      {
        readIdx = sizeof(label) - 1;
      }
      strncpy(label, buffLong, readIdx);
      label[readIdx] = 0;
      clock_gettime(CLOCKTYPE, &timeNow);
      delta = timespecDiff(&timeNow, &timeBase);
      if (-1 == deltaPrevious)
      {
        printf(":%d:%ld::%s:\n", count, (long)(delta / 1000000), label);
      } else
      {
        printf(":%d:%ld:%ld:%s:\n", count, (long)(delta / 1000000), (long)((delta - deltaPrevious) / 1000000), label);
      }
      fflush(stdout);
      deltaPrevious = delta;
      free(buffLong);
    }
  
    close(sockfdClient);
    printf("CONNECTION CLOSED (%d)\n", count);
    fflush(stdout);
  }
  close(sockfd);

  return 0;
}

