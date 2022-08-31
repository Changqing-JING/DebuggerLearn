#include <array>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
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

static void setBreakPoint(pid_t pid) {
  unsigned long const data =
      ptrace(PTRACE_PEEKDATA, pid, breakPointAddress, NULL);

  breakPointInfo.address = breakPointAddress;
  breakPointInfo.instruction = data;

  unsigned long writeData = (data & LowByte0Mask) | INT3;

  ptrace(PTRACE_POKEDATA, pid, breakPointAddress, writeData);
}

static void contienue(pid_t pid) {

  long const rip = ptrace(PTRACE_PEEKUSER, pid, RegisterSize * RIP, NULL);
  if (rip == (reinterpret_cast<long>(breakPointInfo.address) + 1)) {
    printf("contienue from break point %p\n", breakPointInfo.address);
    ptrace(PTRACE_POKEDATA, pid, breakPointInfo.address,
           breakPointInfo.instruction);
    ptrace(PTRACE_POKEUSER, pid, RegisterSize * RIP, rip - 1);
    ptrace(PTRACE_CONT, pid, NULL, NULL);
  } else {
    printf("unknow error at %lx\n", rip);
    exit(1);
  }
}

int main() {

  pid_t targetProcessId = getDebugInfoFromPipe();

  int error = ptrace(PTRACE_ATTACH, targetProcessId);

  if (error != 0) {
    perror("ptrace attach failed\n");
    exit(1);
  }

  while (true) {
    int status;
    waitpid(targetProcessId, &status, WUNTRACED);
    if (WIFEXITED(status)) {
      break;
    } else if (WIFSTOPPED(status)) {
      const int signalNum = WSTOPSIG(status);
      switch (signalNum) {
      case (SIGSTOP): {
        setBreakPoint(targetProcessId);
        ptrace(PTRACE_CONT, targetProcessId, NULL, NULL);
        break;
      }
      case (SIGTRAP): {
        contienue(targetProcessId);
        break;
      }
      default: {
        printf("unknow signal from child recevied %d\n", signalNum);
        exit(1);
      }
      }
    }
  }

  return 0;
}