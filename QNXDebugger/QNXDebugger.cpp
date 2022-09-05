#include <array>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <sys/debug.h>
#include <sys/procfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static unsigned long LowByte0Mask = ~(0xFF);

static constexpr unsigned char INT3 = 0xCC;

static constexpr int RegisterSize = sizeof(void *);

void *breakPointAddress;

int ctl_fd;

static procfs_run run;

struct BreakPointInfo {
  void *address;
  unsigned long instruction;
};

BreakPointInfo breakPointInfo;

static pid_t getDebugInfoFromPipe() {
  const char *fifoname = "/tmp/debug.fifo";

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

static void attach(pid_t pid) {
  std::stringstream pathStream;
  pathStream << "/proc/" << pid << "/as";
  std::string path = pathStream.str();
  ctl_fd = open(path.c_str(), O_RDWR);
  if (ctl_fd < 0) {
    perror("open ctl_fd failed");
    exit(1);
  }

  procfs_status status;
  int error = devctl(ctl_fd, DCMD_PROC_STOP, &status, sizeof(status), 0);

  if (error != 0) {
    perror("devctl DCMD_PROC_STOP failed");
    exit(1);
  }
}

static void resume() {
  int error = devctl(ctl_fd, DCMD_PROC_RUN, &run, sizeof(run), 0);
}

static void setBreakPoint(pid_t pid) {}

static void contienue(pid_t pid) {}

int main() {
  pid_t targetProcessId = getDebugInfoFromPipe();
  attach(targetProcessId);
  resume();
  return 0;
}