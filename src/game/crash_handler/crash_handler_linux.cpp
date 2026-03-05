//=============================================================================
// TF2 Vintage Crash Handler — Linux
// Registers signal handlers for SIGSEGV/SIGABRT/SIGBUS/SIGFPE/SIGILL using
// the self-pipe trick so crash reporting runs on a worker thread rather than
// inside the signal handler itself (avoids async-signal-unsafe calls).
//
// Stack walking uses libunwind if available at runtime, falls back to
// glibc backtrace() otherwise.
//
// Produces in <modroot>/logs/crashes/:
//   crash_YYYYMMDD_HHMMSS.txt  — human-readable report
//   crash_YYYYMMDD_HHMMSS.map  — address list for addr2line (one per line)
//
// No .dmp on Linux — use the .map file with addr2line or gdb for symbols.
//=============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cerrno>

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/syscall.h>
#include <sys/utsname.h>

// execinfo.h provides backtrace() — part of glibc
#include <execinfo.h>

// dlfcn.h provides dladdr() — resolves address → shared lib + symbol
#include <dlfcn.h>

#include <string>
#include <vector>

// libunwind — preferred, more reliable through signal frames
// We load it dynamically so the binary runs even if libunwind isn't installed
#define UNW_LOCAL_ONLY
// Forward-declare just the symbols we need from libunwind so we can
// dlopen it without requiring the header at build time
typedef struct unw_cursor  { uint64_t opaque[140]; } unw_cursor_t;
typedef struct unw_context { uint64_t opaque[128]; } unw_context_t;
typedef uint64_t unw_word_t;

typedef int  (*unw_getcontext_fn)(unw_context_t*);
typedef int  (*unw_init_local_fn)(unw_cursor_t*, unw_context_t*);
typedef int  (*unw_step_fn)(unw_cursor_t*);
typedef int  (*unw_get_reg_fn)(unw_cursor_t*, int, unw_word_t*);
typedef int  (*unw_get_proc_name_fn)(unw_cursor_t*, char*, size_t, unw_word_t*);

static const int UNW_REG_IP = 0; // instruction pointer register id

struct LibUnwind {
    void*               handle;
    unw_getcontext_fn   getcontext;
    unw_init_local_fn   init_local;
    unw_step_fn         step;
    unw_get_reg_fn      get_reg;
    unw_get_proc_name_fn get_proc_name;
    bool                available;
};

static LibUnwind g_unwind = {};

static void TryLoadLibunwind()
{
    // Try versioned names first (more specific), then generic
    const char* candidates[] = {
        "libunwind.so.8",
        "libunwind.so.1",
        "libunwind.so",
        nullptr
    };
    for (int i = 0; candidates[i]; i++) {
        g_unwind.handle = dlopen(candidates[i], RTLD_LAZY | RTLD_LOCAL);
        if (g_unwind.handle) break;
    }
    if (!g_unwind.handle) return;

    g_unwind.getcontext    = (unw_getcontext_fn)   dlsym(g_unwind.handle, "unw_getcontext");
    g_unwind.init_local    = (unw_init_local_fn)   dlsym(g_unwind.handle, "unw_init_local");
    g_unwind.step          = (unw_step_fn)          dlsym(g_unwind.handle, "unw_step");
    g_unwind.get_reg       = (unw_get_reg_fn)       dlsym(g_unwind.handle, "unw_get_reg");
    g_unwind.get_proc_name = (unw_get_proc_name_fn) dlsym(g_unwind.handle, "unw_get_proc_name");

    g_unwind.available = g_unwind.getcontext &&
                         g_unwind.init_local &&
                         g_unwind.step       &&
                         g_unwind.get_reg    &&
                         g_unwind.get_proc_name;
}

// ── Self-pipe crash data ──────────────────────────────────────────────────────

struct CrashInfo {
    int         signo;
    siginfo_t   siginfo;
    ucontext_t  context;
    pid_t       tid;       // thread that crashed
};

static int  g_pipeFd[2]   = { -1, -1 };
static bool g_handlerDone = false;

// ── Utility (async-signal-safe versions) ─────────────────────────────────────

// Write exactly n bytes — retries on EINTR
static void SafeWrite(int fd, const void* buf, size_t n)
{
    const char* p = (const char*)buf;
    while (n > 0) {
        ssize_t written = write(fd, p, n);
        if (written < 0) {
            if (errno == EINTR) continue;
            return;
        }
        p += written;
        n -= written;
    }
}

// ── Signal handler (async-signal-safe) ───────────────────────────────────────

static void SignalHandler(int signo, siginfo_t* info, void* ctx)
{
    // Re-entrancy guard — use only async-signal-safe operations
    static volatile sig_atomic_t entered = 0;
    if (entered) {
        // Second crash inside the handler — just die
        signal(signo, SIG_DFL);
        raise(signo);
        return;
    }
    entered = 1;

    if (g_pipeFd[1] < 0) {
        signal(signo, SIG_DFL);
        raise(signo);
        return;
    }

    CrashInfo crash;
    crash.signo   = signo;
    crash.siginfo = *info;
    crash.tid     = (pid_t)syscall(SYS_gettid);
    if (ctx) {
        memcpy(&crash.context, ctx, sizeof(ucontext_t));
    } else {
        memset(&crash.context, 0, sizeof(ucontext_t));
    }

    // Send crash data to worker thread via pipe
    SafeWrite(g_pipeFd[1], &crash, sizeof(crash));
    close(g_pipeFd[1]);
    g_pipeFd[1] = -1;

    // Block until worker thread finishes writing the report
    // Use pause() in a loop — async-signal-safe
    while (!g_handlerDone) {
        pause();
    }

    // Re-raise with default handler to produce a core dump if ulimit allows
    signal(signo, SIG_DFL);
    raise(signo);
}

// ── Path utilities ────────────────────────────────────────────────────────────

static std::string GetSelfExePath()
{
    char buf[4096] = {};
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) buf[n] = '\0';
    return std::string(buf);
}

// Walk up from the crash DLL to find the tf2vintage mod root.
// Layout: <modroot>/bin/x64/tf2vintage_crash.so
//          → bin/x64  → bin  → <modroot>
static std::string GetModRoot()
{
    // Use /proc/self/maps to find our own .so path
    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps) return "";

    std::string soPath;
    char line[512];
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "tf2vintage_crash.so")) {
            // Line format: addr-addr perms offset dev inode path
            char* path = strrchr(line, ' ');
            if (!path) path = strrchr(line, '\t');
            if (path) {
                path++;
                size_t len = strlen(path);
                while (len > 0 && (path[len-1] == '\n' || path[len-1] == '\r'))
                    path[--len] = '\0';
                soPath = path;
            }
            break;
        }
    }
    fclose(maps);

    if (soPath.empty()) return "";

    // Strip two path components: x64 → bin → <modroot>
    std::string dir = soPath;
    for (int i = 0; i < 3; i++) { // strip filename + 2 dirs
        size_t slash = dir.rfind('/');
        if (slash == std::string::npos) return "";
        dir = dir.substr(0, slash);
    }
    return dir;
}

static std::string MakeTimestamp()
{
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm_info);
    return std::string(buf);
}

static bool MakeDirs(const std::string& path)
{
    // Create directory and parents (equivalent to mkdir -p)
    std::string p = path;
    for (size_t i = 1; i < p.size(); i++) {
        if (p[i] == '/') {
            p[i] = '\0';
            mkdir(p.c_str(), 0755);
            p[i] = '/';
        }
    }
    return mkdir(p.c_str(), 0755) == 0 || errno == EEXIST;
}

// ── Signal name ───────────────────────────────────────────────────────────────

static const char* SignalName(int signo)
{
    switch (signo) {
    case SIGSEGV: return "SIGSEGV (Segmentation Fault)";
    case SIGABRT: return "SIGABRT (Abort)";
    case SIGBUS:  return "SIGBUS (Bus Error)";
    case SIGFPE:  return "SIGFPE (Floating Point Exception)";
    case SIGILL:  return "SIGILL (Illegal Instruction)";
    case SIGPIPE: return "SIGPIPE (Broken Pipe)";
    default: {
        static char buf[32];
        snprintf(buf, sizeof(buf), "SIG%d", signo);
        return buf;
    }
    }
}

static const char* SIGSEGVCode(int code)
{
    switch (code) {
    case SEGV_MAPERR: return "address not mapped (SEGV_MAPERR)";
    case SEGV_ACCERR: return "invalid permissions (SEGV_ACCERR)";
    default:          return "unknown";
    }
}

static const char* SIGBUSCode(int code)
{
    switch (code) {
    case BUS_ADRALN: return "invalid address alignment (BUS_ADRALN)";
    case BUS_ADRERR: return "nonexistent physical address (BUS_ADRERR)";
    case BUS_OBJERR: return "object-specific hardware error (BUS_OBJERR)";
    default:         return "unknown";
    }
}

// ── Crash address from ucontext ───────────────────────────────────────────────

static uintptr_t GetCrashIP(const ucontext_t* ctx)
{
#if defined(__x86_64__)
    return (uintptr_t)ctx->uc_mcontext.gregs[REG_RIP];
#elif defined(__i386__)
    return (uintptr_t)ctx->uc_mcontext.gregs[REG_EIP];
#elif defined(__aarch64__)
    return (uintptr_t)ctx->uc_mcontext.pc;
#else
    return 0;
#endif
}

// ── Stack frame ───────────────────────────────────────────────────────────────

struct StackFrame {
    uintptr_t   address;
    std::string libPath;    // full path to shared library
    std::string libName;    // basename
    uintptr_t   libBase;    // load address of the library
    uintptr_t   offset;     // address - libBase
    std::string symbol;     // demangled symbol name if available
};

// Resolve address → library using /proc/self/maps
struct MapEntry {
    uintptr_t   start;
    uintptr_t   end;
    char        path[512];
};

static std::vector<MapEntry> ReadProcMaps()
{
    std::vector<MapEntry> entries;
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return entries;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        MapEntry e = {};
        unsigned long start, end;
        char perms[8], path[512] = {};
        unsigned long offset, inode;
        unsigned dev_maj, dev_min;
        // Format: start-end perms offset dev inode [path]
        int n = sscanf(line, "%lx-%lx %7s %lx %x:%x %lu %511s",
                       &start, &end, perms, &offset,
                       &dev_maj, &dev_min, &inode, path);
        if (n >= 2) {
            e.start = (uintptr_t)start;
            e.end   = (uintptr_t)end;
            if (n >= 8) strncpy(e.path, path, sizeof(e.path) - 1);
            entries.push_back(e);
        }
    }
    fclose(f);
    return entries;
}

static std::vector<StackFrame> WalkStackUnwind(const ucontext_t* /*ctx*/)
{
    std::vector<StackFrame> frames;
    auto maps = ReadProcMaps();

    unw_context_t uc;
    unw_cursor_t  cursor;

    if (g_unwind.getcontext(&uc) != 0) return frames;
    if (g_unwind.init_local(&cursor, &uc) != 0) return frames;

    for (int depth = 0; depth < 64; depth++) {
        unw_word_t ip = 0;
        g_unwind.get_reg(&cursor, UNW_REG_IP, &ip);
        if (ip == 0) break;

        StackFrame fr;
        fr.address = (uintptr_t)ip;

        // Resolve via dladdr
        Dl_info info = {};
        if (dladdr((void*)ip, &info) && info.dli_fbase) {
            fr.libBase = (uintptr_t)info.dli_fbase;
            fr.offset  = fr.address - fr.libBase;
            fr.libPath = info.dli_fname ? info.dli_fname : "";
            if (!fr.libPath.empty()) {
                size_t slash = fr.libPath.rfind('/');
                fr.libName = (slash != std::string::npos)
                    ? fr.libPath.substr(slash + 1) : fr.libPath;
            }
            if (info.dli_sname) fr.symbol = info.dli_sname;
        } else {
            // Fall back to /proc/self/maps
            for (auto& m : maps) {
                if (ip >= m.start && ip < m.end && m.path[0]) {
                    fr.libPath = m.path;
                    fr.libBase = m.start;
                    fr.offset  = ip - m.start;
                    size_t slash = fr.libPath.rfind('/');
                    fr.libName = (slash != std::string::npos)
                        ? fr.libPath.substr(slash + 1) : fr.libPath;
                    break;
                }
            }
        }

        frames.push_back(fr);

        if (g_unwind.step(&cursor) <= 0) break;
    }
    return frames;
}

static std::vector<StackFrame> WalkStackBacktrace()
{
    std::vector<StackFrame> frames;
    auto maps = ReadProcMaps();

    void* addrs[64];
    int count = backtrace(addrs, 64);

    for (int i = 0; i < count; i++) {
        StackFrame fr;
        fr.address = (uintptr_t)addrs[i];

        Dl_info info = {};
        if (dladdr(addrs[i], &info) && info.dli_fbase) {
            fr.libBase = (uintptr_t)info.dli_fbase;
            fr.offset  = fr.address - fr.libBase;
            fr.libPath = info.dli_fname ? info.dli_fname : "";
            if (!fr.libPath.empty()) {
                size_t slash = fr.libPath.rfind('/');
                fr.libName = (slash != std::string::npos)
                    ? fr.libPath.substr(slash + 1) : fr.libPath;
            }
            if (info.dli_sname) fr.symbol = info.dli_sname;
        } else {
            for (auto& m : maps) {
                if (fr.address >= m.start && fr.address < m.end && m.path[0]) {
                    fr.libPath = m.path;
                    fr.libBase = m.start;
                    fr.offset  = fr.address - m.start;
                    size_t slash = fr.libPath.rfind('/');
                    fr.libName = (slash != std::string::npos)
                        ? fr.libPath.substr(slash + 1) : fr.libPath;
                    break;
                }
            }
        }

        frames.push_back(fr);
    }
    return frames;
}

static std::vector<StackFrame> WalkStack(const ucontext_t* ctx)
{
    if (g_unwind.available)
        return WalkStackUnwind(ctx);
    return WalkStackBacktrace();
}

// ── OS / TF2V version ─────────────────────────────────────────────────────────

static std::string GetOSVersion()
{
    struct utsname uts;
    if (uname(&uts) != 0) return "Unknown";
    char buf[256];
    snprintf(buf, sizeof(buf), "%s %s %s", uts.sysname, uts.release, uts.machine);
    return std::string(buf);
}

static std::string GetTF2VVersion(const std::string& modRoot)
{
    std::string path = modRoot + "/bin/x64/version-bin.txt";
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return "  (version file not found)\n";

    std::string out;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        for (const char* field : {"commit=", "date=", "platform="}) {
            if (strncmp(line, field, strlen(field)) == 0) {
                out += "  ";
                out += line;
                if (out.back() != '\n') out += '\n';
            }
        }
    }
    fclose(f);
    return out.empty() ? "  (no version info)\n" : out;
}

// ── Loaded modules ────────────────────────────────────────────────────────────

struct ModuleEntry {
    uintptr_t   base;
    std::string path;
    std::string name;
};

static std::vector<ModuleEntry> GetLoadedModules()
{
    std::vector<ModuleEntry> modules;
    auto maps = ReadProcMaps();

    std::string lastPath;
    for (auto& m : maps) {
        if (!m.path[0] || m.path[0] == '[') continue;
        if (m.path == lastPath) continue; // only first mapping per library
        lastPath = m.path;

        ModuleEntry e;
        e.base = m.start;
        e.path = m.path;
        size_t slash = e.path.rfind('/');
        e.name = (slash != std::string::npos) ? e.path.substr(slash + 1) : e.path;
        modules.push_back(e);
    }
    return modules;
}

// ── Report writer ─────────────────────────────────────────────────────────────

static void WriteReport(const std::string& txtPath,
                        const std::string& mapPath,
                        const CrashInfo& crash,
                        const std::string& modRoot)
{
    FILE* f = fopen(txtPath.c_str(), "w");
    if (!f) return;

    // ── Header ────────────────────────────────────────────────────────────────
    fprintf(f, "========================================\n");
    fprintf(f, " TF2 Vintage Crash Report (Linux)\n");
    fprintf(f, "========================================\n\n");

    char timeBuf[64];
    struct tm tm_info;
    time_t now = time(nullptr);
    localtime_r(&now, &tm_info);
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm_info);

    fprintf(f, "Time:       %s\n", timeBuf);
    fprintf(f, "OS:         %s\n", GetOSVersion().c_str());
    fprintf(f, "PID:        %d\n", (int)getpid());
    fprintf(f, "TID:        %d\n", (int)crash.tid);
    fprintf(f, "TF2Vintage:\n%s\n", GetTF2VVersion(modRoot).c_str());
    fprintf(f, "Stack walk: %s\n\n",
            g_unwind.available ? "libunwind" : "backtrace() [install libunwind for better results]");

    // ── Signal info ───────────────────────────────────────────────────────────
    fprintf(f, "----------------------------------------\n");
    fprintf(f, " Signal\n");
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "Signal:   %s\n", SignalName(crash.signo));

    uintptr_t ip = GetCrashIP(&crash.context);
    if (ip) fprintf(f, "IP:       0x%016lx\n", (unsigned long)ip);

    if (crash.signo == SIGSEGV) {
        fprintf(f, "Address:  0x%016lx\n", (unsigned long)crash.siginfo.si_addr);
        fprintf(f, "Detail:   %s\n", SIGSEGVCode(crash.siginfo.si_code));
    } else if (crash.signo == SIGBUS) {
        fprintf(f, "Address:  0x%016lx\n", (unsigned long)crash.siginfo.si_addr);
        fprintf(f, "Detail:   %s\n", SIGBUSCode(crash.siginfo.si_code));
    } else if (crash.signo == SIGFPE) {
        fprintf(f, "Address:  0x%016lx\n", (unsigned long)crash.siginfo.si_addr);
    }

#if defined(__x86_64__)
    fprintf(f, "\nRegisters:\n");
    const auto& r = crash.context.uc_mcontext.gregs;
    fprintf(f, "  RAX=%016llx  RBX=%016llx  RCX=%016llx  RDX=%016llx\n",
            (unsigned long long)r[REG_RAX], (unsigned long long)r[REG_RBX],
            (unsigned long long)r[REG_RCX], (unsigned long long)r[REG_RDX]);
    fprintf(f, "  RSI=%016llx  RDI=%016llx  RSP=%016llx  RBP=%016llx\n",
            (unsigned long long)r[REG_RSI], (unsigned long long)r[REG_RDI],
            (unsigned long long)r[REG_RSP], (unsigned long long)r[REG_RBP]);
    fprintf(f, "  R8 =%016llx  R9 =%016llx  R10=%016llx  R11=%016llx\n",
            (unsigned long long)r[REG_R8],  (unsigned long long)r[REG_R9],
            (unsigned long long)r[REG_R10], (unsigned long long)r[REG_R11]);
    fprintf(f, "  RIP=%016llx\n\n", (unsigned long long)r[REG_RIP]);
#endif

    // ── Stack trace ───────────────────────────────────────────────────────────
    fprintf(f, "----------------------------------------\n");
    fprintf(f, " Stack Trace\n");
    fprintf(f, "----------------------------------------\n\n");

    auto frames = WalkStack(&crash.context);

    // Write addr2line map file alongside the report
    FILE* mapFile = fopen(mapPath.c_str(), "w");

    for (size_t i = 0; i < frames.size(); i++) {
        auto& fr = frames[i];
        if (!fr.libName.empty()) {
            fprintf(f, "  #%-2zu  %s + 0x%lx",
                    i, fr.libName.c_str(), (unsigned long)fr.offset);
        } else {
            fprintf(f, "  #%-2zu  0x%016lx", i, (unsigned long)fr.address);
        }
        if (!fr.symbol.empty()) {
            fprintf(f, "  [%s]", fr.symbol.c_str());
        }
        fprintf(f, "\n");

        // Map file: "libpath 0xoffset" — ready for addr2line
        if (mapFile && !fr.libPath.empty()) {
            fprintf(mapFile, "%s 0x%lx\n",
                    fr.libPath.c_str(), (unsigned long)fr.offset);
        }
    }

    if (mapFile) fclose(mapFile);

    // ── Loaded modules ────────────────────────────────────────────────────────
    fprintf(f, "\n----------------------------------------\n");
    fprintf(f, " Loaded Modules\n");
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "  %-40s  %-18s  %s\n", "Name", "Base Address", "Path");
    fprintf(f, "  %-40s  %-18s  %s\n",
            "----------------------------------------", "------------------", "----");

    for (auto& m : GetLoadedModules()) {
        fprintf(f, "  %-40s  0x%016lx  %s\n",
                m.name.c_str(), (unsigned long)m.base, m.path.c_str());
    }

    // ── How to read ───────────────────────────────────────────────────────────
    fprintf(f, "\n========================================\n");
    fprintf(f, " How to read this crash\n");
    fprintf(f, "========================================\n\n");
    fprintf(f, "Option 1 — addr2line (quickest, no download needed):\n");
    fprintf(f, "  The .map file alongside this report contains one 'libpath offset' per line.\n");
    fprintf(f, "  Run this to get source-level symbols for each frame:\n\n");
    fprintf(f, "    while IFS=' ' read -r lib offset; do\n");
    fprintf(f, "      echo \"$lib $offset:\"\n");
    fprintf(f, "      addr2line -e \"$lib\" -f -C \"$offset\"\n");
    fprintf(f, "    done < crash_*.map\n\n");
    fprintf(f, "  This uses the .so files already on your system — no extra download.\n");
    fprintf(f, "  Source lines only appear if the binary was built with debug info.\n\n");
    fprintf(f, "Option 2 — with published debug symbols:\n");
    fprintf(f, "  1. Find the matching release at:\n");
    fprintf(f, "     https://github.com/TF2V/TF2Vintage/releases\n");
    fprintf(f, "     (match by date/build shown in TF2Vintage section above)\n\n");
    fprintf(f, "  2. Download tf2vintage-symbols.zip and extract it\n\n");
    fprintf(f, "  3. Run addr2line pointing at the .debug files instead of the .so files:\n");
    fprintf(f, "     addr2line -e symbols/linux/server.so.debug -f -C 0x<offset>\n\n");
    fprintf(f, "Option 3 — report to us:\n");
    fprintf(f, "  Paste this .txt file into a GitHub issue at:\n");
    fprintf(f, "  https://github.com/TF2V/TF2Vintage/issues/new\n");
    fprintf(f, "  The module+offset stack trace is usually enough to identify the crash.\n");

    fclose(f);
}

// ── Worker thread ─────────────────────────────────────────────────────────────

static void* CrashWorkerThread(void* /*arg*/)
{
    if (g_pipeFd[0] < 0) return nullptr;

    CrashInfo crash;
    ssize_t   bytesRead = 0;
    ssize_t   total     = 0;

    // Read crash info from pipe (may arrive in chunks)
    while (total < (ssize_t)sizeof(crash)) {
        bytesRead = read(g_pipeFd[0], (char*)&crash + total, sizeof(crash) - total);
        if (bytesRead <= 0) {
            if (errno == EINTR) continue;
            break;
        }
        total += bytesRead;
    }
    close(g_pipeFd[0]);
    g_pipeFd[0] = -1;

    if (total < (ssize_t)sizeof(crash)) {
        g_handlerDone = true;
        return nullptr;
    }

    std::string modRoot = GetModRoot();
    std::string logDir  = modRoot + "/logs/crashes";

    // Try to create the log directory; if mod root is empty, use /tmp
    if (modRoot.empty() || !MakeDirs(logDir)) {
        logDir = "/tmp/tf2vintage-crashes";
        MakeDirs(logDir);
    }

    std::string ts      = MakeTimestamp();
    std::string txtPath = logDir + "/crash_" + ts + ".txt";
    std::string mapPath = logDir + "/crash_" + ts + ".map";

    WriteReport(txtPath, mapPath, crash, modRoot);

    // Signal the crash signal handler that we're done
    g_handlerDone = true;
    return nullptr;
}

// ── Constructor — runs before main() ─────────────────────────────────────────

__attribute__((constructor))
static void InstallCrashHandler()
{
    TryLoadLibunwind();

    if (pipe(g_pipeFd) != 0) return;

    // Worker thread — does all the file I/O outside the signal handler
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, CrashWorkerThread, nullptr);
    pthread_attr_destroy(&attr);

    // Register signal handlers
    struct sigaction sa = {};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags     = SA_SIGINFO | SA_RESETHAND;
    sa.sa_sigaction = SignalHandler;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGBUS,  &sa, nullptr);
    sigaction(SIGFPE,  &sa, nullptr);
    sigaction(SIGILL,  &sa, nullptr);
}
