#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <thread>
#include <chrono>

HRESULT PseudoConsoleTest()
{
    HRESULT hr = S_OK;

    COORD size = { 150, 150 };

    // Create communication channels

    // - Close these after CreateProcess of child application with pseudoconsole object.
    HANDLE inputReadSide, outputWriteSide;

    // - Hold onto these and use them for communication with the child through the pseudoconsole
    HANDLE outputReadSide, inputWriteSide;

    if (!CreatePipe(&inputReadSide, &inputWriteSide, NULL, 0)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!CreatePipe(&outputReadSide, &outputWriteSide, NULL, 0)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    HPCON hpc;
    hr = CreatePseudoConsole(size, inputReadSide, outputWriteSide, 0, &hpc);
    if (FAILED(hr)) {
        return hr;
    }
    
    // -----------------------------------------------------------------------------

    // Prepare startup information structure
    STARTUPINFOEXW si;
    ZeroMemory(&si, sizeof(si));
    si.StartupInfo.cb = sizeof(STARTUPINFOEX);

    si.StartupInfo.hStdOutput = outputWriteSide;
    si.StartupInfo.hStdError = outputWriteSide;
    si.StartupInfo.hStdInput = inputReadSide;
    si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;

    // Discover the size required for the list
    size_t bytesRequired;
    InitializeProcThreadAttributeList(NULL, 1, 0, &bytesRequired);

    // Allocate memory to represent the list
    si.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, bytesRequired);
    if (!si.lpAttributeList) {
        return E_OUTOFMEMORY;
    }

    // Initialize the list memory location
    if (!InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &bytesRequired)) {
        HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Set the psueodconsole information into the list
    if (!UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hpc, sizeof(hpc), NULL, NULL)) {
        HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // -----------------------------------------------------------------------------
    
    PCWSTR childApplication = L"C:\\Windows\\System32\\cmd.exe /c  out\\test.exe";

    // Create mutable text string for CreateProcess command line string.
    const size_t charsRequired = wcslen(childApplication) + 1; // +1 null terminator
    PWSTR cmdLineMutable = (PWSTR)HeapAlloc(GetProcessHeap(), 0, sizeof(wchar_t) * charsRequired);

    if (!cmdLineMutable) {
        return E_OUTOFMEMORY;
    }

    wcscpy_s(cmdLineMutable, charsRequired, childApplication);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    using namespace std::chrono;
    auto start = steady_clock::now();

    DWORD create_flags = 0;
    create_flags |= EXTENDED_STARTUPINFO_PRESENT;

    if (!CreateProcessW(NULL, cmdLineMutable, NULL, NULL, FALSE, create_flags, NULL, NULL, &si.StartupInfo, &pi)) {
        HeapFree(GetProcessHeap(), 0, cmdLineMutable);
        std::cout << "Failed to start process!\n";
    }

    // -----------------------------------------------------------------------------

    DWORD total_bytes_read = 0;

    {
        std::jthread reader_thread{ 
            [&] {
                char buffer[65535];
                DWORD bytes_read;
                BOOL read;

                do {
                    read = ReadFile(outputReadSide, buffer, sizeof(buffer), &bytes_read, NULL);
                    // fwrite(buffer, 1, bytes_read, stdout);
                    total_bytes_read += bytes_read;
                    // std::cout << "Read " << bool(read) << " bytes = " << bytes_read << '\n';
                } while (read && bytes_read > 0);
            } 
        };

        WaitForSingleObject(pi.hProcess, INFINITE);
        
        auto end = steady_clock::now();
        std::cout << "Finished in: " << duration_cast<milliseconds>(end - start) << '\n';

        std::cout << "Process complete, waiting for read thread to finish\n";

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        CloseHandle(outputWriteSide);
        CloseHandle(inputReadSide);

        CloseHandle(outputReadSide);
        CloseHandle(inputWriteSide);
    }

    std::cout << "Read " << total_bytes_read << " total\n";

    return S_OK;
}

int main()
{
    HRESULT res = PseudoConsoleTest();
    std::cout << "Result: " << DWORD(res) << '\n';
}