//=============================================================================
// TF2 Vintage Crash Handler
// Registers an unhandled exception filter as early as possible so crashes
// during DLL load and global constructors are captured before the engine
// has a chance to install its own handler (or fail to).
//
// Produces two files in <sourcemods>/tf2vintage/logs/crashes/:
//   crash_YYYYMMDD_HHMMSS.dmp  — full minidump (open in WinDbg/VS + PDBs)
//   crash_YYYYMMDD_HHMMSS.txt  — human-readable report, no tools required
//=============================================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string>
#include <vector>

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")

// ── Utilities ─────────────────────────────────────────────────────────────────

static std::string GetModuleDir()
{
    char path[MAX_PATH] = {};
    HMODULE hSelf = NULL;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&GetModuleDir, &hSelf);
    GetModuleFileNameA(hSelf, path, MAX_PATH);
    std::string s(path);
    size_t slash = s.find_last_of("\\/");
    return (slash != std::string::npos) ? s.substr(0, slash) : s;
}

// Walk up from the DLL's location to find the tf2vintage root.
// Layout: game/tf2vintage/bin/x64/tf2vintage_crash.dll
//         → game/tf2vintage/bin/x64  (GetModuleDir)
//         → game/tf2vintage/bin       (up 1)
//         → game/tf2vintage            (up 2, this is root)
static std::string GetModRoot()
{
    std::string dir = GetModuleDir();
    for (int i = 0; i < 2; i++) {
        size_t slash = dir.find_last_of("\\/");
        if (slash == std::string::npos) break;
        dir = dir.substr(0, slash);
    }
    return dir;
}

static std::string MakeTimestamp()
{
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_s(&tm_info, &now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm_info);
    return std::string(buf);
}

static std::string GetExceptionName(DWORD code)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:         return "ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:               return "BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:    return "DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_OVERFLOW:             return "FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:          return "FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:            return "FLT_UNDERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:      return "ILLEGAL_INSTRUCTION";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:             return "INT_OVERFLOW";
    case EXCEPTION_PRIV_INSTRUCTION:         return "PRIV_INSTRUCTION";
    case EXCEPTION_STACK_OVERFLOW:           return "STACK_OVERFLOW";
    // 0xC0000005 == EXCEPTION_ACCESS_VIOLATION (already above)
    // 0xC000001D == EXCEPTION_ILLEGAL_INSTRUCTION (already above)
    case 0xC0000374:                         return "HEAP_CORRUPTION";
    case 0xC0000409:                         return "STACK_BUFFER_OVERRUN";
    default: {
        char buf[32];
        snprintf(buf, sizeof(buf), "0x%08X", code);
        return std::string(buf);
    }
    }
}

// ── Module list ───────────────────────────────────────────────────────────────

struct ModuleInfo {
    HMODULE base;
    uintptr_t baseAddr;
    uintptr_t size;
    char name[MAX_PATH];
    char path[MAX_PATH];
    DWORD timestamp;
};

static std::vector<ModuleInfo> GetLoadedModules()
{
    std::vector<ModuleInfo> modules;
    HANDLE hProcess = GetCurrentProcess();
    HMODULE hMods[1024];
    DWORD cbNeeded;

    if (!EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
        return modules;

    DWORD count = cbNeeded / sizeof(HMODULE);
    for (DWORD i = 0; i < count; i++) {
        ModuleInfo m = {};
        m.base = hMods[i];
        m.baseAddr = (uintptr_t)hMods[i];

        MODULEINFO mi = {};
        if (GetModuleInformation(hProcess, hMods[i], &mi, sizeof(mi)))
            m.size = mi.SizeOfImage;

        GetModuleBaseNameA(hProcess, hMods[i], m.name, MAX_PATH);
        GetModuleFileNameExA(hProcess, hMods[i], m.path, MAX_PATH);

        // Read timestamp from PE header
        IMAGE_DOS_HEADER dos = {};
        SIZE_T bytesRead;
        if (ReadProcessMemory(hProcess, hMods[i], &dos, sizeof(dos), &bytesRead)
            && dos.e_magic == IMAGE_DOS_SIGNATURE) {
            IMAGE_NT_HEADERS nt = {};
            LPVOID ntAddr = (LPVOID)((uintptr_t)hMods[i] + dos.e_lfanew);
            if (ReadProcessMemory(hProcess, ntAddr, &nt, sizeof(nt), &bytesRead))
                m.timestamp = nt.FileHeader.TimeDateStamp;
        }

        modules.push_back(m);
    }
    return modules;
}

// ── Stack walk ────────────────────────────────────────────────────────────────

struct StackFrame {
    uintptr_t address;
    char moduleName[MAX_PATH];
    uintptr_t moduleBase;
    uintptr_t offset;        // address - moduleBase
    char symbolName[256];    // filled if DbgHelp resolves it
    DWORD lineNumber;
    char sourceFile[MAX_PATH];
};

static std::vector<StackFrame> WalkStack(CONTEXT* ctx)
{
    std::vector<StackFrame> frames;
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread  = GetCurrentThread();

    SymInitialize(hProcess, NULL, TRUE);
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

    STACKFRAME64 sf = {};
    DWORD machineType;

#ifdef _M_X64
    machineType         = IMAGE_FILE_MACHINE_AMD64;
    sf.AddrPC.Offset    = ctx->Rip;
    sf.AddrPC.Mode      = AddrModeFlat;
    sf.AddrFrame.Offset = ctx->Rbp;
    sf.AddrFrame.Mode   = AddrModeFlat;
    sf.AddrStack.Offset = ctx->Rsp;
    sf.AddrStack.Mode   = AddrModeFlat;
#else
    machineType         = IMAGE_FILE_MACHINE_I386;
    sf.AddrPC.Offset    = ctx->Eip;
    sf.AddrPC.Mode      = AddrModeFlat;
    sf.AddrFrame.Offset = ctx->Ebp;
    sf.AddrFrame.Mode   = AddrModeFlat;
    sf.AddrStack.Offset = ctx->Esp;
    sf.AddrStack.Mode   = AddrModeFlat;
#endif

    auto modules = GetLoadedModules();

    for (int depth = 0; depth < 64; depth++) {
        if (!StackWalk64(machineType, hProcess, hThread, &sf,
                         ctx, NULL, SymFunctionTableAccess64,
                         SymGetModuleBase64, NULL))
            break;
        if (sf.AddrPC.Offset == 0)
            break;

        StackFrame frame = {};
        frame.address = (uintptr_t)sf.AddrPC.Offset;

        // Find which module this address belongs to
        for (auto& m : modules) {
            if (frame.address >= m.baseAddr &&
                frame.address < m.baseAddr + m.size) {
                strncpy_s(frame.moduleName, m.name, _TRUNCATE);
                frame.moduleBase = m.baseAddr;
                frame.offset     = frame.address - m.baseAddr;
                break;
            }
        }

        // Try to resolve symbol name (works if PDB is loaded)
        char symBuf[sizeof(SYMBOL_INFO) + 256] = {};
        SYMBOL_INFO* sym = (SYMBOL_INFO*)symBuf;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen   = 255;
        DWORD64 displacement = 0;
        if (SymFromAddr(hProcess, frame.address, &displacement, sym)) {
            strncpy_s(frame.symbolName, sym->Name, _TRUNCATE);
        }

        // Try to resolve source line
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(line);
        DWORD lineDisp = 0;
        if (SymGetLineFromAddr64(hProcess, frame.address, &lineDisp, &line)) {
            frame.lineNumber = line.LineNumber;
            strncpy_s(frame.sourceFile, line.FileName, _TRUNCATE);
        }

        frames.push_back(frame);
    }

    SymCleanup(hProcess);
    return frames;
}

// ── OS version ────────────────────────────────────────────────────────────────

static std::string GetOSVersion()
{
    // RtlGetVersion bypasses the compatibility shim that GetVersionEx uses
    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return "Unknown";

    auto fn = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
    if (!fn) return "Unknown";

    RTL_OSVERSIONINFOW info = {};
    info.dwOSVersionInfoSize = sizeof(info);
    fn(&info);

    char buf[64];
    snprintf(buf, sizeof(buf), "Windows %lu.%lu.%lu",
        info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber);
    return std::string(buf);
}

// ── TF2V version ──────────────────────────────────────────────────────────────

static std::string GetTF2VVersion()
{
    std::string root = GetModRoot();
    std::string versionPath = root + "\\bin\\x64\\version-bin.txt";
    FILE* f = nullptr;
    fopen_s(&f, versionPath.c_str(), "r");
    if (!f) return "Unknown";

    std::string result;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        result += line;
    }
    fclose(f);

    // Extract just the commit and date lines
    std::string out;
    for (auto& field : {"commit=", "date=", "platform="}) {
        size_t pos = result.find(field);
        if (pos != std::string::npos) {
            size_t end = result.find('\n', pos);
            out += std::string("  ") + result.substr(pos,
                (end != std::string::npos) ? end - pos : std::string::npos) + "\n";
        }
    }
    return out.empty() ? "Unknown" : out;
}

// ── Minidump writer ───────────────────────────────────────────────────────────

static void WriteMiniDump(const std::string& path, EXCEPTION_POINTERS* ep)
{
    HANDLE hFile = CreateFileA(path.c_str(),
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    MINIDUMP_EXCEPTION_INFORMATION mei = {};
    mei.ThreadId          = GetCurrentThreadId();
    mei.ExceptionPointers = ep;
    mei.ClientPointers    = FALSE;

    // MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo |
    // MiniDumpWithUnloadedModules | MiniDumpWithIndirectlyReferencedMemory
    MINIDUMP_TYPE type = (MINIDUMP_TYPE)(
        MiniDumpWithDataSegs              |
        MiniDumpWithProcessThreadData     |
        MiniDumpWithHandleData            |
        MiniDumpWithThreadInfo            |
        MiniDumpWithUnloadedModules       |
        MiniDumpWithFullMemoryInfo        |
        MiniDumpWithIndirectlyReferencedMemory
    );

    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                      hFile, type, &mei, NULL, NULL);
    CloseHandle(hFile);
}

// ── Text report writer ────────────────────────────────────────────────────────

static void WriteTextReport(const std::string& path,
                             EXCEPTION_POINTERS* ep,
                             const std::string& dmpPath)
{
    FILE* f = nullptr;
    fopen_s(&f, path.c_str(), "w");
    if (!f) return;

    EXCEPTION_RECORD* rec = ep->ExceptionRecord;
    CONTEXT*          ctx = ep->ContextRecord;

    // ── Header ────────────────────────────────────────────────────────────────
    fprintf(f, "========================================\n");
    fprintf(f, " TF2 Vintage Crash Report\n");
    fprintf(f, "========================================\n\n");

    time_t now = time(NULL);
    char timeBuf[64];
    struct tm tm_info;
    localtime_s(&tm_info, &now);
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm_info);
    fprintf(f, "Time:       %s\n", timeBuf);
    fprintf(f, "OS:         %s\n", GetOSVersion().c_str());
    fprintf(f, "TF2Vintage:\n%s\n", GetTF2VVersion().c_str());
    fprintf(f, "Dump file:  %s\n\n", dmpPath.c_str());

    // ── Exception info ────────────────────────────────────────────────────────
    fprintf(f, "----------------------------------------\n");
    fprintf(f, " Exception\n");
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "Code:     %s\n", GetExceptionName(rec->ExceptionCode).c_str());
    fprintf(f, "Address:  0x%016llX\n", (unsigned long long)rec->ExceptionAddress);

    if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2) {
        const char* opStr = (rec->ExceptionInformation[0] == 0) ? "read" :
                            (rec->ExceptionInformation[0] == 1) ? "write" : "execute";
        fprintf(f, "Detail:   Attempted to %s address 0x%016llX\n",
                opStr, (unsigned long long)rec->ExceptionInformation[1]);
    }

#ifdef _M_X64
    fprintf(f, "\nRegisters:\n");
    fprintf(f, "  RAX=%016llX  RBX=%016llX  RCX=%016llX  RDX=%016llX\n",
        ctx->Rax, ctx->Rbx, ctx->Rcx, ctx->Rdx);
    fprintf(f, "  RSI=%016llX  RDI=%016llX  RSP=%016llX  RBP=%016llX\n",
        ctx->Rsi, ctx->Rdi, ctx->Rsp, ctx->Rbp);
    fprintf(f, "  R8 =%016llX  R9 =%016llX  R10=%016llX  R11=%016llX\n",
        ctx->R8,  ctx->R9,  ctx->R10, ctx->R11);
    fprintf(f, "  RIP=%016llX\n\n", ctx->Rip);
#else
    fprintf(f, "\nRegisters:\n");
    fprintf(f, "  EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X\n",
        ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx);
    fprintf(f, "  ESI=%08X  EDI=%08X  ESP=%08X  EBP=%08X\n",
        ctx->Esi, ctx->Edi, ctx->Esp, ctx->Ebp);
    fprintf(f, "  EIP=%08X\n\n", ctx->Eip);
#endif

    // ── Stack trace ───────────────────────────────────────────────────────────
    fprintf(f, "----------------------------------------\n");
    fprintf(f, " Stack Trace\n");
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "To get full source-level symbols, load this .dmp in WinDbg or Visual Studio\n");
    fprintf(f, "with the matching tf2vintage-symbols.zip from the same release.\n\n");

    auto frames = WalkStack(ctx);
    for (size_t i = 0; i < frames.size(); i++) {
        auto& fr = frames[i];
        if (fr.moduleName[0]) {
            fprintf(f, "  #%-2zu  %s + 0x%llX",
                i, fr.moduleName, (unsigned long long)fr.offset);
        } else {
            fprintf(f, "  #%-2zu  0x%016llX", i, (unsigned long long)fr.address);
        }
        if (fr.symbolName[0]) {
            fprintf(f, "  [%s", fr.symbolName);
            if (fr.sourceFile[0]) {
                // Strip full path — just show filename:line for readability
                const char* fname = strrchr(fr.sourceFile, '\\');
                fname = fname ? fname + 1 : fr.sourceFile;
                fprintf(f, "  %s:%lu", fname, fr.lineNumber);
            }
            fprintf(f, "]");
        }
        fprintf(f, "\n");
    }

    // ── Loaded modules ────────────────────────────────────────────────────────
    fprintf(f, "\n----------------------------------------\n");
    fprintf(f, " Loaded Modules\n");
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "  %-40s  %-18s  %s\n", "Name", "Base Address", "Path");
    fprintf(f, "  %-40s  %-18s  %s\n",
        "----------------------------------------",
        "------------------",
        "----");

    auto modules = GetLoadedModules();
    for (auto& m : modules) {
        fprintf(f, "  %-40s  0x%016llX  %s\n",
            m.name, (unsigned long long)m.baseAddr, m.path);
    }

    fprintf(f, "\n========================================\n");
    fprintf(f, " How to read this crash\n");
    fprintf(f, "========================================\n");
    fprintf(f, "1. Find the matching release at:\n");
    fprintf(f, "   https://github.com/TF2V/TF2Vintage/releases\n");
    fprintf(f, "   (match by date/build shown in TF2Vintage section above)\n\n");
    fprintf(f, "2. Download tf2vintage-symbols.zip from that release\n\n");
    fprintf(f, "3. Open the .dmp file in WinDbg or Visual Studio\n");
    fprintf(f, "   Set the symbol path to the extracted symbols folder\n\n");
    fprintf(f, "4. Or paste this text file into a GitHub issue at:\n");
    fprintf(f, "   https://github.com/TF2V/TF2Vintage/issues/new\n");
    fprintf(f, "   The stack trace above is often enough to identify the crash\n");
    fprintf(f, "   even without loading symbols.\n");

    fclose(f);
}

// ── Exception filter ──────────────────────────────────────────────────────────

static LONG WINAPI TF2VExceptionFilter(EXCEPTION_POINTERS* ep)
{
    // Avoid re-entering if we crash inside the crash handler
    static volatile LONG inHandler = 0;
    if (InterlockedCompareExchange(&inHandler, 1, 0) != 0)
        return EXCEPTION_CONTINUE_SEARCH;

    // Build output directory: tf2vintage/logs/crashes/
    std::string root    = GetModRoot();
    std::string logDir  = root + "\\logs\\crashes";
    CreateDirectoryA((root + "\\logs").c_str(), NULL);
    CreateDirectoryA(logDir.c_str(), NULL);

    std::string ts      = MakeTimestamp();
    std::string dmpPath = logDir + "\\crash_" + ts + ".dmp";
    std::string txtPath = logDir + "\\crash_" + ts + ".txt";

    WriteMiniDump(dmpPath, ep);
    WriteTextReport(txtPath, ep, dmpPath);

    // Let the default handler run (produces the standard Windows crash dialog
    // or WER report), then terminate
    return EXCEPTION_CONTINUE_SEARCH;
}

// ── DLL entry point ───────────────────────────────────────────────────────────

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hInst);

        // Register our filter. We use EXCEPTION_CONTINUE_SEARCH so the engine's
        // own handler (if it loads later) still runs after us.
        // AddVectoredExceptionHandler runs BEFORE SetUnhandledExceptionFilter,
        // which is exactly what we want for catching pre-engine crashes.
        AddVectoredExceptionHandler(0 /* last */, TF2VExceptionFilter);
    }
    return TRUE;
}
