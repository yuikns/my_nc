#include <cstdio>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include <sys/select.h>

int kbhit() {
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);  // STDIN_FILENO is 0
  select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &fds);
}

int main(int argc, char const *argv[]) {
  char buf[1001];
  memset(buf, 0, sizeof(char) * 1001);
  while (true) {
    if (kbhit()) {
      if (nullptr != fgets(buf, 1000, stdin)) {
        int n = strlen(buf);
        printf("In:[%d][%s]\n", n, buf);
      }
    } else {
      usleep(200 * 1000);  // sleep 200 ms
    }
  }
}