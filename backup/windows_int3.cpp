#include <thread>
#include <cstdio>

#include <cstdint>

#ifdef _WIN32
#include <windows.h>

long signalHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    printf("enter signalHandler\n");
    DWORD exceptionCode = pExceptionInfo->ExceptionRecord->ExceptionCode;

    if (exceptionCode == EXCEPTION_BREAKPOINT) {
        printf("int3\n");
        pExceptionInfo->ContextRecord->Rip += 1;
        while (true) {
            std::this_thread::sleep_for(std::chrono::microseconds(1000000));
        }
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    else {


    }

    return EXCEPTION_CONTINUE_SEARCH;
}

void debug(uint64_t x) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    asm(
        "mov $5, %rdi\n"
        "int3"
    );
    printf("done\n");
}

int test_int3() {

    void* handle = AddVectoredExceptionHandler(1, reinterpret_cast<PVECTORED_EXCEPTION_HANDLER>(signalHandler));

    if (handle == nullptr) {
        printf("%s", "AddVectoredExceptionHandler failed\n");
        exit(-1);
    }

    std::thread th(debug, 5);
    while (true) {
        printf("runing\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

#endif