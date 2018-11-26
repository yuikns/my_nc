#include <cassert>
#include <cstdio>
#include <cstdlib>  // malloc
#include <cstring>

#include <netdb.h>
#include <unistd.h>

#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/unistd.h>

typedef struct sockaddr SA;

#define kDataPkgSize 1024

/**
 * const char * host : host name  or IP
 * const char * serv : port or service
 */
int TcpConn(const char *host, const char *serv) {
  int sockfd;
  int n;
  struct addrinfo hints;
  struct addrinfo *res;
  struct addrinfo *ressave;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((n = getaddrinfo(host, serv, &hints, &res)) != 0) {
    fprintf(stderr, "TcpConn error for %s , %s:%s \n", gai_strerror(n), host,
            serv);
    return 0;
  }

  ressave = res;

  do {
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // error , try next one
    if (sockfd < 0) continue;

    // connect , if return zero ,
    // it will be judged as success and break the loop
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) break;

    // connect error ,
    // close this one and try next
    close(sockfd);

  } while ((res = res->ai_next) != NULL);

  if (res == NULL)  // errno from final socket() or connect()
  {
    fprintf(stderr, "TcpConn error for %s:%s \n", host, serv);
    return 0;
  }

  freeaddrinfo(ressave);
  return sockfd;
}

void DataStreamIO(int sockfd, char *host, FILE *fp) {
  fd_set connset;
  FD_ZERO(&connset);
  int nfds = sockfd + 1;
  while (true) {
    FD_SET(STDIN_FILENO, &connset);
    FD_SET(sockfd, &connset);
    int isl = select(nfds, &connset, NULL, NULL, NULL);
    if (isl < 0) {
      fprintf(stderr, "select() error: %d\n", isl);
      return;
    }
    if (FD_ISSET(STDIN_FILENO, &connset) != 0) {
      char buf[kDataPkgSize];
      int n;
      memset(buf, 0, sizeof(char) * kDataPkgSize);
      if ((n = read(STDIN_FILENO, buf, kDataPkgSize)) > 0) {
        size_t send_len = write(sockfd, buf, n);
        fprintf(fp, "[%zu]> ", send_len);
        fwrite(buf, send_len, sizeof(char), fp);
        fprintf(fp, "\n");
        fflush(nullptr);
      }
    }
    if (FD_ISSET(sockfd, &connset) != 0) {
      int n;
      char recvline[kDataPkgSize];
      memset(recvline, 0, sizeof(char) * kDataPkgSize);
      if ((n = read(sockfd, recvline, kDataPkgSize)) > 0) {
        // recvline[n] = '\0';
        fprintf(fp, "[%d]< ", n);
        fwrite(recvline, n, sizeof(char), fp);
        fwrite(recvline, n, sizeof(char), stdout);
        fprintf(fp, "\n");
        fflush(nullptr);
      } else {
        return;
      }
    }
    usleep(1000 * 20);
  }
}

int main(int argc, char *argv[]) {
  if(argc < 3) {
    return -1;
  }
  char *host = argv[1];
  char *serv = argv[2];

  char log_file_name[1024];
  if(argc > 3) {
    snprintf(log_file_name, 1023, "/tmp/nc-%s.log",argv[3]);
  } else {
    snprintf(log_file_name, 1023, "/tmp/nc.log");
  }
  FILE *fp = fopen(log_file_name, "w+");
  
  
  int sockfd;
  struct sockaddr_storage ss;
  sockfd = TcpConn(host, serv);
  if(sockfd <= 0) {
    return -1;
  }
  socklen_t len = sizeof(ss);
  getpeername(sockfd, (SA *)&ss, &len);

  // fprintf(stdout, "connected to %s:%s\n", argv[1], argv[2]);

  DataStreamIO(sockfd, host, fp);

  return 0;
}