#include "demos.hpp"
#include <cstdint>
#include <cstdio>
#include <signal.h>

#if (defined __linux__) || (defined __APPLE__) ||                              \
    ((defined(__unix__) || defined(__unix)) && !(defined __MINGW32__))

#include <unistd.h>

#endif

void test_int3() {
  asm(".intel_syntax noprefix");
  asm("int3");
  asm(".att_syntax prefix");
  return;
}

int test_trap_flag() {
  asm(".intel_syntax noprefix");
  asm("pushf\n"
      "or word ptr [rsp],0x0100\n"
      "popf\n"
      "mov rax, 5\n"
      "add rax, 10\n"
      "add rax, 20\n"
      "pushf\n"
      "and word ptr [rsp],0xfeff\n"
      "popf\n");
  asm(".att_syntax prefix");
end:
  return 0;
}

void signalHandler(int32_t const code, siginfo_t *const si, void *const ptr) {
  ucontext_t *const uc = reinterpret_cast<ucontext_t *>(ptr);
  if (code == SIGTRAP) {

    return;
  } else {
    return;
  }

  return;
}

void hardware_single_step_demo() {
  struct sigaction sa = {};
  sa.sa_sigaction = signalHandler;
  constexpr uint32_t sa_flags_basic = SA_SIGINFO | SA_NODEFER;
  sigfillset(&sa.sa_mask);
  sigaction(SIGTRAP, &sa, nullptr);
  int res = test_trap_flag();
  printf("res = %d\n", res);
}