#include "demos.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <signal.h>

#if (defined __linux__) || (defined __APPLE__) ||                              \
    ((defined(__unix__) || defined(__unix)) && !(defined __MINGW32__))

#include <unistd.h>

#ifdef __x86_64__

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

#elif defined(__aarch64__)
int test_trap_flag() {

  asm("brk 0xf000\n"
      "mov x0, 1\n"
      "mov x1, 2\n"
      "add x2, x0, x1");
end:
  return 0;
}
#endif

int trigger = 0;

void signalHandler(int32_t const code, siginfo_t *const si, void *const ptr) {
  ucontext_t *const uc = reinterpret_cast<ucontext_t *>(ptr);
  if (code == SIGTRAP) {
    printf("ptr is %p\n", ptr);
#ifdef __aarch64__
    if (trigger == 0) {
      uc->uc_mcontext.pc += 4;
      constexpr uint64_t ssMask = 1U << 21U;
      uc->uc_mcontext.pstate |= ssMask;
      trigger = 1;
    } else {
      exit(0);
    }

#endif

    return;
  } else {
    return;
  }

  return;
}

void hardware_single_step_demo() {
  struct sigaction sa = {};
  sa.sa_sigaction = signalHandler;
  sa.sa_flags = SA_SIGINFO;
  sigfillset(&sa.sa_mask);
  sigaction(SIGTRAP, &sa, nullptr);
  int res = test_trap_flag();
  printf("res = %d\n", res);
}

#elif defined _WIN32
#include <windows.h>
long signalHandler(PEXCEPTION_POINTERS pExceptionInfo) {
  DWORD const exceptionCode = pExceptionInfo->ExceptionRecord->ExceptionCode;
  
  if (exceptionCode == STATUS_BREAKPOINT) {
#ifdef _M_ARM64
    printf("Break point triggered at %p\n", pExceptionInfo->ExceptionRecord->ExceptionAddress);
    pExceptionInfo->ContextRecord->Pc += 4;
    pExceptionInfo->ContextRecord->Cpsr |= 0x200000;
#endif
  }
  else if (exceptionCode == STATUS_SINGLE_STEP) {
      printf("Single step triggered at %p\n", pExceptionInfo->ExceptionRecord->ExceptionAddress);
  }

  return EXCEPTION_CONTINUE_EXECUTION;
}
void hardware_single_step_demo() {
  AddVectoredExceptionHandler(
      1, reinterpret_cast<PVECTORED_EXCEPTION_HANDLER>(signalHandler));
  int a = 1;

#ifdef _M_ARM64
  asm("brk 0xF000\n");
#endif
  printf("a is %d\n", a);
}

#endif