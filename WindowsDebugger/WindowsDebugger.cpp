// clang-format off
#include <windows.h>
#include <DbgHelp.h>
// clang-format on
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <array>

#pragma comment(lib, "dbghelp.lib")

const char *debugTargetPath =
    "D:\\code\\workspace\\LearningProjects\\DebuggerLearn\\build_"
    "win\\WindowsDebugger\\DebugTarget\\Debug\\DebugTarget.exe";

DWORD continueStatus = DBG_EXCEPTION_NOT_HANDLED;
ULONGLONG targetBaseAddr;
ULONGLONG targetEntry;

void *breakPointAddress = reinterpret_cast<void *>(0x00007FF6ABE31000);

typedef struct {
  void *addr;
  uint8_t firstByte;
} Instruction;

Instruction instruction{};

static HANDLE getThreadHandleByID(DWORD threadId) {
  HANDLE handle = OpenThread(THREAD_ALL_ACCESS, 0, threadId);
  if (handle == INVALID_HANDLE_VALUE) {
      printf("OpenThread failed %ld\n", GetLastError());
      exit(1);
  }
  return handle;
}

static HANDLE getProcessHandleByID(DWORD processId) {
  HANDLE handle = OpenProcess(THREAD_ALL_ACCESS, 0, processId);
  if (handle == INVALID_HANDLE_VALUE) {
      printf("OpenProcess failed %ld\n", GetLastError());
      exit(1);
  }
  return handle;
}

static void setBreakPoint(HANDLE processHandle, void *address) {
  SIZE_T dwRead;

  BOOL success =
      ReadProcessMemory(processHandle, address, &instruction.firstByte, sizeof(instruction.firstByte), &dwRead);

  if (success == 0) {
    printf("ReadProcessMemory failed\n");
    exit(1);
  }

  printf("process created\n");

  instruction.addr = address;

  byte int3 = 0xCC;

  success =
      WriteProcessMemory(processHandle, address, &int3, sizeof(byte), &dwRead);

  if (success == 0) {
    printf("WriteProcessMemory failed\n");
    exit(1);
  }
}

void onProcessCreated(const CREATE_PROCESS_DEBUG_INFO *const debugInfo) {

  setBreakPoint(debugInfo->hProcess, breakPointAddress);

  CloseHandle(debugInfo->hFile);
  CloseHandle(debugInfo->hProcess);
  CloseHandle(debugInfo->hThread);
  return;
}

void onException(DEBUG_EVENT const &debugEvent) {
  const EXCEPTION_DEBUG_INFO *const exceptionInfo = &debugEvent.u.Exception;
  const DWORD exceptionCode = exceptionInfo->ExceptionRecord.ExceptionCode;
  std::cout << "Debug target exception happens address: "
            << exceptionInfo->ExceptionRecord.ExceptionAddress
            << ", code: " << std::hex << exceptionCode << std::endl;
  if (exceptionInfo->dwFirstChance == TRUE) {
    if (exceptionInfo->ExceptionRecord.ExceptionAddress == instruction.addr &&
        exceptionCode == EXCEPTION_BREAKPOINT) {
      SIZE_T dwWrite;
      HANDLE threadHandle = getThreadHandleByID(debugEvent.dwThreadId);
      HANDLE processHandle = getProcessHandleByID(debugEvent.dwProcessId);
      WriteProcessMemory(processHandle, instruction.addr,
                         &instruction.firstByte, sizeof(byte), &dwWrite);

      CONTEXT context{};
      context.ContextFlags = CONTEXT_CONTROL;
      BOOL success = GetThreadContext(threadHandle, &context);
      if (success == 0) {
        perror("GetThreadContext failed");
        exit(1);
      }
      context.Rip--;
      success = SetThreadContext(threadHandle, &context);
      if (success == 0) {
        perror("SetThreadContext failed");
        exit(1);
      }
      CloseHandle(threadHandle);
      CloseHandle(processHandle);
      continueStatus = DBG_CONTINUE;
    } else {
      continueStatus = DBG_EXCEPTION_NOT_HANDLED;
    }
  } else {
    continueStatus = DBG_CONTINUE;
  }

  return;
}

void getStartAddress() {
  std::ifstream file(debugTargetPath, std::ios::binary);
  if (!file) {
    printf("open file failed\n");
    exit(1);
  }

  std::vector<uint8_t> fileBytes;
  fileBytes.assign(std::istreambuf_iterator<char>(file),
                   std::istreambuf_iterator<char>());
  uint8_t *buf = fileBytes.data();

  IMAGE_NT_HEADERS *stNT =
      (IMAGE_NT_HEADERS *)(buf + ((IMAGE_DOS_HEADER *)buf)->e_lfanew);

  targetBaseAddr = stNT->OptionalHeader.ImageBase;
  targetEntry = stNT->OptionalHeader.AddressOfEntryPoint;
}

void createDebugTargetProcess() {
  STARTUPINFO si{};
  si.cb = sizeof(STARTUPINFO);
  PROCESS_INFORMATION pi{};
  BOOL success = CreateProcessA(debugTargetPath, NULL, NULL, NULL, false,
                                DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &si, &pi);
  if (success == 0) {
    printf("CreateProcess failed %s\n", debugTargetPath);
    exit(-1);
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
}

static DWORD getDebugInfoFromPipe() {
  HANDLE hPipe =
      CreateFileA(TEXT("\\\\.\\pipe\\Pipe"), GENERIC_READ | GENERIC_WRITE, 0,
                  NULL, OPEN_EXISTING, 0, NULL);

  if (hPipe == INVALID_HANDLE_VALUE) {
    perror("CreateFileA failed");
    exit(1);
  }

  std::array<uint8_t, sizeof(DWORD) + sizeof(void*)> buffer;
  DWORD dwRead = 0;

  while (dwRead == 0) {
    ReadFile(hPipe, buffer.data(), buffer.size(), &dwRead, NULL);
  }
  DWORD processId;
  memcpy(&processId, buffer.data(), sizeof(DWORD));
  memcpy(&breakPointAddress, buffer.data() + sizeof(DWORD), sizeof(void*));

  CloseHandle(hPipe);
  return processId;
}

void attachToProcess(DWORD processID) {
  BOOL success = DebugActiveProcess(processID);
  if (success == 0) {
    printf("DebugActiveProcess");
    exit(1);
  }
}

int main(int argc, const char *argv[]) {

    DWORD processId = getDebugInfoFromPipe();
    attachToProcess(processId);
  
  while (true) {
    DEBUG_EVENT debugEvent{};
    BOOL waitEvent = TRUE;
    BOOL success = WaitForDebugEvent(&debugEvent, INFINITE);
    if (success == 0) {
      break;
    }
    switch (debugEvent.dwDebugEventCode) {
    case (CREATE_PROCESS_DEBUG_EVENT): {
      onProcessCreated(&debugEvent.u.CreateProcessInfo);
      break;
    }
    case (EXCEPTION_DEBUG_EVENT): {
      onException(debugEvent);
      // waitEvent = FALSE;
      break;
    }
    default: {
      break;
    }
    }

    if (waitEvent == TRUE) {
      ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId,
                         continueStatus);
    }
  }

  return 0;
}
