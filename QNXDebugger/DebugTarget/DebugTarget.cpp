#include <array>
#include <cerrno>
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

  const char *fifoname = "/tmp/debug.fifo";

  int fd = open(fifoname, O_WRONLY | O_CREAT | O_TRUNC, 666);

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
  uint32_t counter = 0;

  while (true) {
    foo();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    printf("Hello Debugger %d\n", counter++);
  }
  return 0;
}