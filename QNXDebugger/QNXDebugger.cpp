#include <array>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

static unsigned long LowByte0Mask = ~(0xFF);

static constexpr unsigned char INT3 = 0xCC;

static constexpr int RegisterSize = sizeof(void *);

void *breakPointAddress;

struct BreakPointInfo {
  void *address;
  unsigned long instruction;
};

BreakPointInfo breakPointInfo;

static pid_t getDebugInfoFromPipe() {
  const char *fifoname = "/tmp/debugger.fifo";

  int const fd = open(fifoname, O_RDONLY);

  if (fd < 0) {
    perror("open pipe failed");
    exit(1);
  }

  std::array<uint8_t, sizeof(pid_t) + sizeof(void *)> buffer{};

  ssize_t readSize = read(fd, buffer.data(), buffer.size());

  if (readSize != buffer.size()) {
    perror("read pipe failed");
    exit(1);
  }

  pid_t processId;
  memcpy(&processId, buffer.data(), sizeof(pid_t));
  memcpy(&breakPointAddress, buffer.data() + sizeof(pid_t), sizeof(void *));

  close(fd);
  return processId;
}

static void setBreakPoint(pid_t pid) {}

static void contienue(pid_t pid) {}

int main() {
  pid_t targetProcessId = getDebugInfoFromPipe();
  return 0;
}