#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <windows.h>

uint64_t num = 0x1122334455667788U;

void foo() { return; }

void writeProcessIdToPipe(DWORD processId) {
  HANDLE hPipe =
      CreateNamedPipeA(TEXT("\\\\.\\pipe\\Pipe"), PIPE_ACCESS_DUPLEX,
                       PIPE_TYPE_BYTE | PIPE_READMODE_BYTE |
                           PIPE_WAIT, // FILE_FLAG_FIRST_PIPE_INSTANCE is not
                                      // needed but forces CreateNamedPipe(..)
                                      // to fail if the pipe already exists...
                       1, 1024 * 16, 1024 * 16, NMPWAIT_USE_DEFAULT_WAIT, NULL);

  if (hPipe == INVALID_HANDLE_VALUE) {
    perror("CreateNamedPipeA failed");
    exit(1);
  }

  if (ConnectNamedPipe(hPipe, NULL) !=
      FALSE) // wait for someone to connect to the pipe
  {
    printf("pipe client connected\n");

    DWORD writtenSize;

    void *fooAddress = reinterpret_cast<void *>(&foo);

    std::array<uint8_t, sizeof(DWORD) + sizeof(void *)> buf{};

    memcpy(buf.data(), &processId, sizeof(DWORD));
    memcpy(buf.data() + sizeof(DWORD), &fooAddress, sizeof(void *));

    BOOL success = WriteFile(hPipe, buf.data(), buf.size(), &writtenSize, NULL);

    if (success == 0) {
      printf("Write Pipe failed %lx\n", GetLastError());
      exit(1);
    }

    success = FlushFileBuffers(hPipe);
    if (success == 0) {
      printf("FlushFileBuffers failed %lx\n", GetLastError());
      exit(1);
    }
  }

  DisconnectNamedPipe(hPipe);

  CloseHandle(hPipe);
}

int main(int argc, const char *argv[]) {
  DWORD processId = GetCurrentProcessId();
  printf("process id is %ld, num address is %p, foo address is %p\n", processId,
         &num, &foo);

  writeProcessIdToPipe(processId);

  while (true) {
    foo();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return 0;
}