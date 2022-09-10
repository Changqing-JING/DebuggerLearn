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

static constexpr unsigned char INT3 = 0xCC;

static constexpr int RegisterSize = sizeof(void *);

void *breakPointAddress;

int ctl_fd;

static int counter = 0;

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

static void setBreakPoint() {
  procfs_break brk;

  brk.type = _DEBUG_BREAK_EXEC;
  brk.addr = reinterpret_cast<uint64_t>(breakPointAddress);
  brk.size = 0;
  int error = devctl(ctl_fd, DCMD_PROC_BREAK, &brk, sizeof(brk), 0);

  if (error != 0) {
    perror("setBreakPoint devctl DCMD_PROC_BREAK failed");
    exit(1);
  }
}

static void removeBreakPoint() {
  procfs_break brk;

  brk.type = _DEBUG_BREAK_EXEC;
  brk.addr = reinterpret_cast<uint64_t>(breakPointAddress);
  brk.size = -1;
  int error = devctl(ctl_fd, DCMD_PROC_BREAK, &brk, sizeof(brk), 0);

  if (error != 0) {
    perror("removeBreakPoint devctl DCMD_PROC_BREAK failed");
    exit(1);
  }
}

static void readMemory(void *address) {
  off_t offset = lseek(ctl_fd, reinterpret_cast<off_t>(address), SEEK_SET);

  if (offset != reinterpret_cast<off_t>(address)) {
    perror("lseek failed");
    exit(1);
  }

  char buff[5];

  ssize_t readSize = read(ctl_fd, buff, sizeof(buff));
  if (readSize == 0) {
    perror("read failed");
    exit(1);
  }
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
  procfs_status status{};
  errno = devctl(ctl_fd, DCMD_PROC_STATUS, &status, sizeof(status), 0);
  if (errno != 0) {
    perror("resume: devctl DCMD_PROC_STATUS failed\n");
    exit(1);
  }

  procfs_run run{};
  memset(&run, 0, sizeof(run));

  sigset_t *run_fault = (sigset_t *)(void *)&run.fault;

  sigemptyset(run_fault);
  sigaddset(run_fault, FLTBPT);
  sigaddset(run_fault, FLTTRACE);
  sigaddset(run_fault, FLTILL);
  sigaddset(run_fault, FLTPRIV);
  sigaddset(run_fault, FLTBOUNDS);
  sigaddset(run_fault, FLTIOVF);
  sigaddset(run_fault, FLTIZDIV);
  sigaddset(run_fault, FLTFPE);
  /* Peter V will be changing this at some point.  */
  sigaddset(run_fault, FLTPAGE);

  run.flags = (_DEBUG_RUN_FAULT | _DEBUG_RUN_TRACE | _DEBUG_RUN_ARM |
               _DEBUG_RUN_CLRSIG | _DEBUG_RUN_CLRFLT);

  if (counter == 0) {
    run.flags |= _DEBUG_RUN_STEP;
  }
  counter++;

  errno = devctl(ctl_fd, DCMD_PROC_RUN, &run, sizeof(run), 0);

  if (errno != 0) {
    perror("devctl DCMD_PROC_RUN failed\n");
    exit(1);
  } else {
    printf("DCMD_PROC_RUN success\n");
  }
}

static void setBreakPoint(pid_t pid) {}

static void contienue(pid_t pid) {}

int main() {
  pid_t targetProcessId = getDebugInfoFromPipe();
  attach(targetProcessId);
  resume();
  setBreakPoint();
  while (true) {
    procfs_status status;
    int error = devctl(ctl_fd, DCMD_PROC_STATUS, &status, sizeof(status), 0);

    if (error != 0) {
      perror("devctl DCMD_PROC_STATUS failed\n");
    }

    if (status.info.si_signo == SIGTRAP) {
      printf("SigTrap happend\n");
      if (counter == 0) {
        removeBreakPoint();
      }

      resume();
    }
  }
  while (true) {
  }
  return 0;
}