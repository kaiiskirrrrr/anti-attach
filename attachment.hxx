#ifndef ATTACH_HXX
#define ATTACH_HXX

#include <memory>
#include <windows.h>
#include <string>
#include <winternl.h>
#include <iostream>

class already_debugged
{
public:

    auto start(int argc, char* argv[]) -> void
    {
        this->load();

        if (argc == 1)
        {
            this->initialize_main_process();
        }
        else
        {
            const auto parent_process_id = atoi(argv[1]);
            this->start_child_process(parent_process_id);
        }
    }

private:

    auto load() -> void
    {
        static DWORD(WINAPI * NtQuerySystemInformation)(DWORD, PVOID, ULONG, PULONG) = nullptr;
        static DWORD(WINAPI * NtOpenThread)(HANDLE*, DWORD, OBJECT_ATTRIBUTES*, CLIENT_ID*) = nullptr;
        static DWORD(WINAPI * NtQueryObject)(HANDLE, DWORD, PVOID, DWORD, DWORD*) = nullptr;

        const HMODULE ntdll_handle = GetModuleHandleA("ntdll.dll");
        if (!ntdll_handle)
            return;

        NtQuerySystemInformation = (DWORD(WINAPI*)(DWORD, PVOID, ULONG, PULONG))GetProcAddress(ntdll_handle, "NtQuerySystemInformation");
        NtOpenThread = (DWORD(WINAPI*)(HANDLE*, DWORD, OBJECT_ATTRIBUTES*, CLIENT_ID*))GetProcAddress(ntdll_handle, "NtOpenThread");
        NtQueryObject = (DWORD(WINAPI*)(HANDLE, DWORD, PVOID, DWORD, DWORD*))GetProcAddress(ntdll_handle, "NtQueryObject");
    }

    auto start_child_process(const DWORD& parent_process_id) -> void
    {
        HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, parent_process_id);
        if (!process_handle or DebugActiveProcess(parent_process_id) == 0)
        {
            CloseHandle(process_handle);
            return;
        }

        HANDLE debug_handle = *(HANDLE*)((BYTE*)__readgsqword(0x30) + 0x16A8);
        if (!debug_handle)
        {
            CloseHandle(process_handle);
            return;
        }

        HANDLE cloned_debug_handle;
        if (!DuplicateHandle(GetCurrentProcess(), debug_handle, process_handle, &cloned_debug_handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
        {
            CloseHandle(process_handle);
            return;
        }

        DEBUG_EVENT debug_event;
        DWORD timeout_count = 0;

        while (timeout_count < 10)
        {
            if (WaitForDebugEvent(&debug_event, 100) == 0)
            {
                timeout_count++;
                continue;
            }
            timeout_count = 0;
            ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, DBG_CONTINUE);
        }

        CloseHandle(cloned_debug_handle);
        CloseHandle(debug_handle);
        CloseHandle(process_handle);
        ExitThread(0);
    }

    auto initialize_main_process() -> void
    {
        STARTUPINFOA startup_info = { sizeof(startup_info) };
        PROCESS_INFORMATION process_info = {};
        char current_path[MAX_PATH];
        GetModuleFileNameA(nullptr, current_path, sizeof(current_path));

        std::string command_line = "\"" + std::string(current_path) + "\" " + std::to_string(GetCurrentProcessId());
        if (CreateProcessA(nullptr, const_cast<LPSTR>(command_line.c_str()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup_info, &process_info))
        {
            CloseHandle(process_info.hProcess);
            CloseHandle(process_info.hThread);
            WaitForSingleObject(process_info.hProcess, INFINITE);
        }
    }
};
inline const auto c_already_debugged = std::make_unique<already_debugged>();

#endif // !ATTACH_HXX
