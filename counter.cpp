#include <iostream>
#include <windows.h>
#include <string>
#include <iomanip>
#include <ctime>
#include <sstream>

CRITICAL_SECTION printSection;
volatile bool firstThreadDone = false;

//current timestamp in milliseconds
std::string getTimestamp() {
    SYSTEMTIME st;
    GetSystemTime(&st);
    std::ostringstream oss;
    oss << "[" << st.wMinute << ":" << st.wSecond << "." << st.wMilliseconds << "] ";
    return oss.str();
}

void printSafe(const std::string& message) {
    EnterCriticalSection(&printSection);
    std::cout << getTimestamp() << message << "\n" << std::flush;
    LeaveCriticalSection(&printSection);
}

//some work to demonstrate concurrent execution
void doSomeWork() {
    for(int i = 0; i < 1000000; i++) {
        // Some CPU work
        double x = 123.456 * 789.012;
    }
}

DWORD WINAPI countUp(LPVOID) {
    for(int i = 0; i <= 20; i++) {
        printSafe("Thread 1 counting up: " + std::to_string(i) + " (doing work...)");
        doSomeWork();  //some work instead of just sleeping
        if(i == 20) {
            firstThreadDone = true;
            printSafe("Thread 1 has reached 20!");
        }
    }
    return 0;
}

DWORD WINAPI countDown(LPVOID) {
    printSafe("Thread 2 is waiting but still active (checking every 100ms)");
    while (!firstThreadDone) {
        Sleep(100);
        doSomeWork();  //some work while waiting
        printSafe("Thread 2 still waiting... (doing background work)");
    }
    
    printSafe("Thread 2 detected Thread 1 completion, starting countdown");
    for(int i = 20; i >= 0; i--) {
        printSafe("Thread 2 counting down: " + std::to_string(i) + " (doing work...)");
        doSomeWork();
    }
    return 0;
}

int main() {
    try {
        InitializeCriticalSection(&printSection);
        
        printSafe("Starting threaded counter program with timestamp proof of execution...");
        
        HANDLE h1 = CreateThread(NULL, 0, countUp, NULL, 0, NULL);
        if (h1 == NULL) throw std::runtime_error("Failed to create first thread");

        HANDLE h2 = CreateThread(NULL, 0, countDown, NULL, 0, NULL);
        if (h2 == NULL) {
            CloseHandle(h1);
            throw std::runtime_error("Failed to create second thread");
        }
        
        //main thread also shows it's active
        for(int i = 0; i < 10; i++) {
            Sleep(500);
            printSafe("Main thread is also running (monitoring threads)");
        }
        
        WaitForSingleObject(h1, INFINITE);
        WaitForSingleObject(h2, INFINITE);
        
        CloseHandle(h1);
        CloseHandle(h2);
        DeleteCriticalSection(&printSection);
        
        printSafe("All threads finished!");
        system("pause");
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
