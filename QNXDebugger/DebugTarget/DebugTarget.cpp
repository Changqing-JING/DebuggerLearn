#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

uint64_t num = 0x1122334455667788U;

void foo() { return; }

void writeProcessIdToPipe(pid_t processId) {
  void *fooAddress = reinterpret_cast<void *>(&foo);

  std::array<uint8_t, sizeof(pid_t) + sizeof(void *)> buf;

  memcpy(buf.data(), &processId, sizeof(pid_t));
  memcpy(buf.data() + sizeof(pid_t), &fooAddress, sizeof(void *));

  const char *fifoname = "/tmp/debugger.fifo";
  unlink(fifoname);

  int error = mkfifo(fifoname, S_IRWXU);

  if (error != 0) {
    perror("mkfifo failed");
    exit(1);
  }

  int fd = open(fifoname, 666);

  if (fd < 0) {
    perror("open pipe failed");
    exit(1);
  }

  ssize_t writeSize = write(fd, buf.data(), buf.size());

  if (writeSize != buf.size()) {
    perror("write pipe failed");
    exit(1);
  }
}

int main(int argc, const char *argv[]) {
  pid_t processId = getpid();
  printf("process id is %d, num address is %p, foo address is %p\n", processId,
         &num, &foo);

  writeProcessIdToPipe(processId);

  while (true) {
    foo();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    printf("Hello Debugger\n");
  }
  return 0;
}