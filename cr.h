/*
# cr.h
 
A single file header-only live reload solution for C, written in C++:

- simple public API, 3 functions only to use (and only one to export);
- works and tested on Linux and Windows;
- automatic crash protection;
- automatic static state transfer;
- based on dynamic reloadable binary (.so/.dll);
- MIT licensed;

### Build Status:

|Platform|Build Status|
|--------|------|
|Linux|[![Build Status](https://travis-ci.org/fungos/cr.svg?branch=master)](https://travis-ci.org/fungos/cr)|
|Windows|[![Build status](https://ci.appveyor.com/api/projects/status/jf0dq97w9b7b5ihi?svg=true)](https://ci.appveyor.com/project/fungos/cr)|

Note that the only file that matters is `cr.h`.

This file contains the documentation in markdown, the license, the implementation and the public api.
All other files in this repository are supporting files and can be safely ignored.

### Example

A (thin) host application executable will make use of `cr` to manage
live-reloading of the real application in the form of dynamic loadable binary, a host would be something like:

```c
#define CR_HOST // required in the host only and before including cr.h
#include "../cr.h"

int main(int argc, char *argv[]) {
    // the host application should initalize a plugin with a context, a plugin
    cr_plugin ctx;

    // the full path to the live-reloadable application
    cr_plugin_load(ctx, "c:/path/to/build/game.dll");

    // call the update function at any frequency matters to you, this will give the real application a chance to run
    while (!cr_plugin_update(ctx)) {
        // do anything you need to do on host side (ie. windowing and input stuff?)
    }

    // at the end do not forget to cleanup the plugin context
    cr_plugin_close(ctx);
    return 0;
}
```

While the guest (real application), would be like:

```c
CR_EXPORT int cr_main(struct cr_plugin *ctx, enum cr_op operation) {
    assert(ctx);
    switch (operation) {
        case CR_LOAD:   return on_load(...);
        case CR_UNLOAD: return on_unload(...);
    }
    // CR_STEP
    return on_update(...);
}
```

### Samples

Two simple samples can be found in the `samples` directory.

The first is one is a simple console application that demonstrate some basic static
states working between instances and basic crash handling tests. Print to output
is used to show what is happening.

The second one demonstrates how to live-reload an opengl application using
 [Dear ImGui](https://github.com/ocornut/imgui). Some state lives in the host
 side while most of the code is in the guest side.

 ![imgui sample](https://i.imgur.com/Nq6s0GP.gif)

#### Running Samples and Tests

The samples and tests uses the [fips build system](https://github.com/floooh/fips). It requires Python and CMake.

```
$ ./fips build            # will generate and build all artifacts
$ ./fips run crTest       # To run tests
$ ./fips run imgui_host   # To run imgui sample
# open a new console, then modify imgui_guest.cpp
$ ./fips make imgui_guest # to build and force imgui sample live reload
```

### Documentation

#### `int (*cr_main)(struct cr_plugin *ctx, enum cr_op operation)`

This is the function pointer to the dynamic loadable binary entry point function.

Arguments

- `ctx` pointer to a context that will be passed from `host` to the `guest` containing valuable information about the current loaded version, failure reason and user data. For more info see `cr_plugin`.
- `operation` which operation is being executed, see `cr_op`.

Return

- A negative value indicating an error, forcing a rollback to happen and failure
 being set to `CR_USER`. 0 or a positive value that will be passed to the
  `host` process.

#### `bool cr_plugin_load(cr_plugin &ctx, const char *fullpath)`

Loads and initialize the plugin.

Arguments

- `ctx` a context that will manage the plugin internal data and user data.
- `fullpath` full path with filename to the loadable binary for the plugin or
 `NULL`.

Return

- `true` in case of success, `false` otherwise.

#### `int cr_plugin_update(cr_plugin &ctx)`

This function will call the plugin `cr_main` function. It should be called as
 frequently as the core logic/application needs.

Arguments

- `ctx` the current plugin context data.

Return

- -1 if a failure happened during an update;
- -2 if a failure happened during a load or unload;
- anything else is returned directly from the plugin `cr_main`.

#### `void cr_plugin_close(cr_plugin &ctx)`

Cleanup internal states once the plugin is not required anymore.

Arguments

- `ctx` the current plugin context data.

#### `cr_op`

Enum indicating the kind of step that is being executed by the `host`:

- `CR_LOAD` A load is being executed, can be used to restore any saved internal
 state;
- `CR_STEP` An application update, this is the normal and most frequent operation;
- `CR_UNLOAD` An unload will be executed, giving the application one chance to
 store any required data.
- `CR_CLOSE` Like `CR_UNLOAD` but no `CR_LOAD` should be expected;

#### `cr_plugin`

The plugin instance context struct.

- `p` opaque pointer for internal cr data;
- `userdata` may be used by the user to pass information between reloads;
- `version` incremetal number for each succeded reload, starting at 1 for the
 first load;
- `failure` used by the crash protection system, will hold the last failure error
 code that caused a rollback. See `cr_failure` for more info on possible values;

#### `cr_failure`

If a crash in the loadable binary happens, the crash handler will indicate the
 reason of the crash with one of these:

- `CR_NONE` No error;
- `CR_SEGFAULT` Segmentation fault. `SIGSEGV` on Linux or
 `EXCEPTION_ACCESS_VIOLATION` on Windows;
- `CR_ILLEGAL` In case of illegal instruction. `SIGILL` on Linux or
 `EXCEPTION_ILLEGAL_INSTRUCTION` on Windows;
- `CR_ABORT` Abort, `SIGBRT` on Linux, not used on Windows;
- `CR_MISALIGN` Bus error, `SIGBUS` on Linux or `EXCEPTION_DATATYPE_MISALIGNMENT`
 on Windows;
- `CR_BOUNDS` Is `EXCEPTION_ARRAY_BOUNDS_EXCEEDED`, Windows only;
- `CR_STACKOVERFLOW` Is `EXCEPTION_STACK_OVERFLOW`, Windows only;
- `CR_STATE_INVALIDATED` Static `CR_STATE` management safety failure;
- `CR_OTHER` Other signal, Linux only;
- `CR_USER` User error (for negative values returned from `cr_main`);

#### `CR_HOST` define

This define should be used before including the `cr.h` in the `host`, if `CR_HOST`
 is not defined, `cr.h` will work as a public API header file to be used in the
  `guest` implementation.

Optionally `CR_HOST` may also be defined to one of the following values as a way
 to configure the `safety` operation mode for automatic static state management
  (`CR_STATE`):

- `CR_SAFEST` Will validate address and size of the state data sections during
 reloads, if anything changes the load will rollback;
- `CR_SAFE` Will validate only the size of the state section, this mean that the
 address of the statics may change (and it is best to avoid holding any pointer
  to static stuff);
- `CR_UNSAFE` Will validate nothing but that the size of section fits, may not
 be necessarelly exact (growing is acceptable but shrinking isn't), this is the
 default behavior;
- `CR_DISABLE` Completely disable automatic static state management;

#### `CR_STATE` macro

Used to tag a global or local static variable to be saved and restored during a reload.

Usage

`static bool CR_STATE bInitialized = false;`


### FAQ / Troubleshooting

#### Q: Why?

A: Read about why I made this [here](https://fungos.github.io/blog/2017/11/20/cr.h-a-simple-c-hot-reload-header-only-library/).

#### Q: My application asserts/crash when freeing heap data allocated inside the dll, what is happening?

A: Make sure both your application host and your dll are using the dynamic
 run-time (/MD or /MDd) as any data allocated in the heap must be freed with
  the same allocator instance, by sharing the run-time between guest and
   host you will guarantee the same allocator is being used.

#### Q: Can we load multiples plugins at the same time?

A: Yes. This should work without issues on Windows. On Linux, there may be 
issues with signal handling with the crash protection as it does not have the
plugin context to know which one crashed. This should be fixed in a near future.

### License

The MIT License (MIT)

Copyright (c) 2017 Danny Angelo Carminati Grein

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### Source

<details>
<summary>View Source Code</summary>

```c
*/
#ifndef __CR_H__
#define __CR_H__

// cr_mode defines how much we validate global state transfer between
// instances. The default is CR_UNSAFE, you can choose another mode by
// defining CR_HOST, ie.: #define CR_HOST CR_SAFEST
enum cr_mode {
    CR_SAFEST = 0, // validate address and size of the state section, if
                   // anything changes the load will rollback
    CR_SAFE = 1,   // validate only the size of the state section, this means
                   // that address is assumed to be safe if avoided keeping 
                   // references to global/static states
    CR_UNSAFE = 2, // don't validate anything but that the size of the section
                   // fits, may not be identical though
    CR_DISABLE = 3 // completely disable the auto state transfer
};

// cr_op is passed into the guest process to indicate the current operation
// happening so the process can manage its internal data if it needs.
enum cr_op {
    CR_LOAD = 0,
    CR_STEP = 1,
    CR_UNLOAD = 2,
    CR_CLOSE = 3,
};

enum cr_failure {
    CR_NONE,     // No error
    CR_SEGFAULT, // SIGSEGV / EXCEPTION_ACCESS_VIOLATION
    CR_ILLEGAL,  // illegal instruction (SIGILL) / EXCEPTION_ILLEGAL_INSTRUCTION
    CR_ABORT,    // abort (SIGBRT)
    CR_MISALIGN, // bus error (SIGBUS) / EXCEPTION_DATATYPE_MISALIGNMENT
    CR_BOUNDS,   // EXCEPTION_ARRAY_BOUNDS_EXCEEDED
    CR_STACKOVERFLOW, // EXCEPTION_STACK_OVERFLOW
    CR_STATE_INVALIDATED, // one or more global data sectio changed and does
                          // not safely match basically a failure of
                          // cr_plugin_validate_sections

    CR_OTHER,    // Unknown or other signal,
    CR_USER = 0x100,
};

struct cr_plugin;

typedef int (*cr_plugin_main_func)(struct cr_plugin *ctx, enum cr_op operation);

// public interface for the plugin context, this has some user facing
// variables that may be used to manage reload feedback.
// - userdata may be used by the user to pass information between reloads
// - version is the reload counter (after loading the first instance it will
//   be 1, not 0)
// - failure is the (platform specific) last error code for any crash that may
//   happen to cause a rollback reload used by the crash protection system
struct cr_plugin {
    void *p;
    void *userdata;
    unsigned int version;
    enum cr_failure failure;
};

#if defined(_MSC_VER)
#if defined(__cplusplus)
#define CR_EXPORT extern "C" __declspec(dllexport)
#define CR_IMPORT extern "C" __declspec(dllimport)
#else
#define CR_EXPORT __declspec(dllexport)
#define CR_IMPORT __declspec(dllimport)
#endif
#endif // defined(_MSC_VER)

#if defined(__GNUC__) // clang & gcc
#if defined(__cplusplus)
#define CR_EXPORT extern "C" __attribute__((visibility("default")))
#else
#define CR_EXPORT __attribute__((visibility("default")))
#endif
#define CR_IMPORT
#endif // defined(__GNUC__)

#ifndef CR_HOST

// Some helpers required in the guest side.
#pragma section(".state", read, write)

#if defined(_MSC_VER)
// GCC: __attribute__((section(".state")))
#define CR_STATE __declspec(allocate(".state"))
#endif // defined(_MSC_VER)

#if defined(__GNUC__) // clang & gcc
#define CR_STATE __attribute__((section(".state")))
#endif // defined(__GNUC__)

#else // #ifndef CR_HOST

#pragma warning(disable:4003) // macro args
#define CR_DO_EXPAND(x) x##1337
#define CR_EXPAND(x) CR_DO_EXPAND(x)

#if CR_EXPAND(CR_HOST) == 1337
#define CR_OP_MODE CR_UNSAFE
#else
#define CR_OP_MODE CR_HOST
#endif

#include <algorithm>
#include <cassert> // assert
#include <chrono>  // duration for sleep
#include <string>
#include <thread> // this_thread::sleep_for

#if _WIN32
#define CR_PATH_SEPARATOR '\\'
#define CR_PATH_SEPARATOR_INVALID '/'
#else
#define CR_PATH_SEPARATOR '/'
#define CR_PATH_SEPARATOR_INVALID '\\'
#endif

static void cr_split_path(std::string path, std::string &parent_dir,
                          std::string &base_name, std::string &ext) {
    std::replace(path.begin(), path.end(), CR_PATH_SEPARATOR_INVALID,
                 CR_PATH_SEPARATOR);
    auto sep_pos = path.rfind(CR_PATH_SEPARATOR);
    auto dot_pos = path.rfind('.');

    if (sep_pos == std::string::npos) {
        parent_dir = "";
        if (dot_pos == std::string::npos) {
            ext = "";
            base_name = path;
        } else {
            ext = path.substr(dot_pos);
            base_name = path.substr(0, dot_pos);
        }
    } else {
        parent_dir = path.substr(0, sep_pos + 1);
        if (dot_pos == std::string::npos || sep_pos > dot_pos) {
            ext = "";
            base_name = path.substr(sep_pos + 1);
        } else {
            ext = path.substr(dot_pos);
            base_name = path.substr(sep_pos + 1, dot_pos - sep_pos - 1);
        }
    }
}

static std::string cr_version_path(const std::string &basepath,
                                   unsigned version) {
    std::string folder, filename, ext;
    cr_split_path(basepath, folder, filename, ext);
    return folder + filename + std::to_string(version) + ext;
}

namespace cr_plugin_section_type {
enum e { state, bss, count };
}

namespace cr_plugin_section_version {
enum e { backup, current, count };
}

struct cr_plugin_section {
    cr_plugin_section_type::e type = {};
    intptr_t base = 0;
    char *ptr = 0;
    int64_t size = 0;
    void *data = nullptr;
};

struct cr_plugin_segment {
    char *ptr = 0;
    int64_t size = 0;
};

// keep track of some internal state about the plugin, should not be messed
// with by user
struct cr_internal {
    std::string fullname = {};
    time_t timestamp = {};
    void *handle = nullptr;
    cr_plugin_main_func main = nullptr;
    cr_plugin_segment seg = {};
    cr_plugin_section data[cr_plugin_section_type::count]
                          [cr_plugin_section_version::count] = {};
    cr_mode mode = CR_SAFEST;
};

static bool cr_plugin_section_validate(cr_plugin &ctx,
                                       cr_plugin_section_type::e type,
                                       intptr_t vaddr, intptr_t ptr,
                                       int64_t size);
static void cr_plugin_sections_reload(cr_plugin &ctx,
                                      cr_plugin_section_version::e version);
static void cr_plugin_sections_store(cr_plugin &ctx);
static void cr_plugin_sections_backup(cr_plugin &ctx);
static void cr_plugin_reload(cr_plugin &ctx);
static void cr_plugin_unload(cr_plugin &ctx, bool rollback, bool close);
static bool cr_plugin_changed(cr_plugin &ctx);
static bool cr_plugin_rollback(cr_plugin &ctx);
static int cr_plugin_main(cr_plugin &ctx, cr_op operation);

#if defined(_WIN32)

// clang-format off
#include <windows.h>
#include <dbghelp.h>
// clang-format on

#pragma comment(lib, "dbghelp.lib")

#define CR_STR "%ls"
#define CR_INT "%ld"

using so_handle = HMODULE;

static std::wstring cr_utf8_to_wstring(const std::string &str) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
    wchar_t wpath_small[MAX_PATH];
    std::unique_ptr<wchar_t[]> wpath_big;
    wchar_t *wpath = wpath_small;
    if (wlen > _countof(wpath_small)) {
        wpath_big = std::unique_ptr<wchar_t[]>(new wchar_t[wlen]);
        wpath = wpath_big.get();
    }

    if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wpath, wlen) != wlen) {
        return L"";
    }

    return wpath;
}

static size_t file_size(const std::string &path) {
    std::wstring wpath = cr_utf8_to_wstring(path);
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fad)) {
        return -1;
    }

    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;

    return static_cast<size_t>(size.QuadPart);
}

static time_t cr_last_write_time(const std::string &path) {
    std::wstring wpath = cr_utf8_to_wstring(path);
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fad)) {
        return -1;
    }

    LARGE_INTEGER time;
    time.HighPart = fad.ftLastWriteTime.dwHighDateTime;
    time.LowPart = fad.ftLastWriteTime.dwLowDateTime;

    return static_cast<time_t>(time.QuadPart / 10000000 - 11644473600LL);
}

static bool cr_exists(const std::string &path) {
    std::wstring wpath = cr_utf8_to_wstring(path);
    return GetFileAttributesW(wpath.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static void cr_copy(const std::string &from, const std::string &to) {
    std::wstring wfrom = cr_utf8_to_wstring(from);
    std::wstring wto = cr_utf8_to_wstring(to);
    CopyFileW(wfrom.c_str(), wto.c_str(), false);
}

// If using Microsoft Visual C/C++ compiler we need to do some workaround the
// fact that the compiled binary has a fullpath to the PDB hardcoded inside
// it. This causes a lot of headaches when trying compile while debugging as
// the referenced PDB will be locked by the debugger.
// To solve this problem, we patch the binary to rename the PDB to something
// we know will be unique to our in-flight instance, so when debugging it will
// lock this unique PDB and the compiler will be able to overwrite the
// original one.
#if defined(_MSC_VER)
#include <crtdbg.h>
#include <limits.h>
#include <stdio.h>
#include <tchar.h>

static std::string cr_replace_extension(const std::string &filepath,
                                        const std::string &ext) {
    std::string folder, filename, old_ext;
    cr_split_path(filepath, folder, filename, old_ext);
    return folder + filename + ext;
}

template <class T>
static T struct_cast(void *ptr, LONG offset = 0) {
    return reinterpret_cast<T>(reinterpret_cast<intptr_t>(ptr) + offset);
}

// RSDS Debug Information for PDB files
using DebugInfoSignature = DWORD;
#define CR_RSDS_SIGNATURE 'SDSR'
struct cr_rsds_hdr {
    DebugInfoSignature signature;
    GUID guid;
    long version;
    char filename[1];
};

static bool cr_pe_debugdir_rva(PIMAGE_OPTIONAL_HEADER optionalHeader,
                               DWORD &debugDirRva, DWORD &debugDirSize) {
    if (optionalHeader->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        auto optionalHeader64 =
            struct_cast<PIMAGE_OPTIONAL_HEADER64>(optionalHeader);
        debugDirRva =
            optionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG]
                .VirtualAddress;
        debugDirSize =
            optionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    } else {
        auto optionalHeader32 =
            struct_cast<PIMAGE_OPTIONAL_HEADER32>(optionalHeader);
        debugDirRva =
            optionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG]
                .VirtualAddress;
        debugDirSize =
            optionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    }

    if (debugDirRva == 0 && debugDirSize == 0) {
        return true;
    } else if (debugDirRva == 0 || debugDirSize == 0) {
        return false;
    }

    return true;
}

static bool cr_pe_fileoffset_rva(PIMAGE_NT_HEADERS ntHeaders, DWORD rva,
                                 DWORD &fileOffset) {
    bool found = false;
    auto *sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections;
         i++, sectionHeader++) {
        auto sectionSize = sectionHeader->Misc.VirtualSize;
        if ((rva >= sectionHeader->VirtualAddress) &&
            (rva < sectionHeader->VirtualAddress + sectionSize)) {
            found = true;
            break;
        }
    }

    if (!found) {
        return false;
    }

    const int diff = static_cast<int>(sectionHeader->VirtualAddress -
                                sectionHeader->PointerToRawData);
    fileOffset = rva - diff;
    return true;
}

static char *cr_pdb_find(LPBYTE imageBase, PIMAGE_DEBUG_DIRECTORY debugDir) {
    assert(debugDir && imageBase);
    LPBYTE debugInfo = imageBase + debugDir->PointerToRawData;
    const auto debugInfoSize = debugDir->SizeOfData;
    if (debugInfo == 0 || debugInfoSize == 0) {
        return nullptr;
    }

    if (IsBadReadPtr(debugInfo, debugInfoSize)) {
        return nullptr;
    }

    if (debugInfoSize < sizeof(DebugInfoSignature)) {
        return nullptr;
    }

    if (debugDir->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
        auto signature = *(DWORD *)debugInfo;
        if (signature == CR_RSDS_SIGNATURE) {
            auto *info = (cr_rsds_hdr *)(debugInfo);
            if (IsBadReadPtr(debugInfo, sizeof(cr_rsds_hdr))) {
                return nullptr;
            }

            if (IsBadStringPtrA((const char *)info->filename, UINT_MAX)) {
                return nullptr;
            }

            return info->filename;
        }
    }

    return nullptr;
}

static bool cr_pdb_replace(const std::string &filename,
                           const std::string &pdbname, char *pdbnamebuf,
                           int pdbnamelen) {
    assert(pdbnamebuf);
    HANDLE fp = nullptr;
    HANDLE filemap = nullptr;
    LPVOID mem = 0;
    bool result = false;
    do {
        fp = CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, nullptr);
        if ((fp == INVALID_HANDLE_VALUE) || (fp == nullptr)) {
            break;
        }

        filemap = CreateFileMapping(fp, nullptr, PAGE_READWRITE, 0, 0, nullptr);
        if (filemap == nullptr) {
            break;
        }

        mem = MapViewOfFile(filemap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (mem == nullptr) {
            break;
        }

        auto dosHeader = struct_cast<PIMAGE_DOS_HEADER>(mem);
        if (dosHeader == 0) {
            break;
        }

        if (IsBadReadPtr(dosHeader, sizeof(IMAGE_DOS_HEADER))) {
            break;
        }

        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            break;
        }

        auto ntHeaders =
            struct_cast<PIMAGE_NT_HEADERS>(dosHeader, dosHeader->e_lfanew);
        if (ntHeaders == 0) {
            break;
        }

        if (IsBadReadPtr(ntHeaders, sizeof(ntHeaders->Signature))) {
            break;
        }

        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            break;
        }

        if (IsBadReadPtr(&ntHeaders->FileHeader, sizeof(IMAGE_FILE_HEADER))) {
            break;
        }

        if (IsBadReadPtr(&ntHeaders->OptionalHeader,
                         ntHeaders->FileHeader.SizeOfOptionalHeader)) {
            break;
        }

        if (ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
            ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            break;
        }

        auto sectionHeaders = IMAGE_FIRST_SECTION(ntHeaders);
        if (IsBadReadPtr(sectionHeaders,
                         ntHeaders->FileHeader.NumberOfSections *
                             sizeof(IMAGE_SECTION_HEADER))) {
            break;
        }

        DWORD debugDirRva = 0;
        DWORD debugDirSize = 0;
        if (!cr_pe_debugdir_rva(&ntHeaders->OptionalHeader, debugDirRva,
                                debugDirSize)) {
            break;
        }

        if (debugDirRva == 0 || debugDirSize == 0) {
            break;
        }

        DWORD debugDirOffset = 0;
        if (!cr_pe_fileoffset_rva(ntHeaders, debugDirRva, debugDirOffset)) {
            break;
        }

        auto debugDir =
            struct_cast<PIMAGE_DEBUG_DIRECTORY>(mem, debugDirOffset);
        if (debugDir == 0) {
            break;
        }

        if (IsBadReadPtr(debugDir, debugDirSize)) {
            break;
        }

        if (debugDirSize < sizeof(IMAGE_DEBUG_DIRECTORY)) {
            break;
        }

        int numEntries = debugDirSize / sizeof(IMAGE_DEBUG_DIRECTORY);
        if (numEntries == 0) {
            break;
        }

        for (int i = 1; i <= numEntries; i++, debugDir++) {
            char *pdb = cr_pdb_find((LPBYTE)mem, debugDir);
            if (pdb && strlen(pdb) >= strlen(pdbname.c_str())) {
                auto len = strlen(pdb);
                memcpy_s(pdbnamebuf, pdbnamelen, pdb, len);
                std::memset(pdb, '\0', len);
                memcpy_s(pdb, len, pdbname.c_str(), pdbname.length());
                result = true;
            }
        }
    } while (0);

    if (mem != nullptr) {
        UnmapViewOfFile(mem);
    }

    if (filemap != nullptr) {
        CloseHandle(filemap);
    }

    if ((fp != nullptr) && (fp != INVALID_HANDLE_VALUE)) {
        CloseHandle(fp);
    }

    return result;
}

bool static cr_pdb_process(const std::string &filename,
                           const std::string &pdbname) {
    char orig_pdb[MAX_PATH];
    memset(orig_pdb, 0, sizeof(orig_pdb));
    bool result = cr_pdb_replace(filename, pdbname, orig_pdb, sizeof(orig_pdb));
    result &= static_cast<bool>(CopyFile(orig_pdb, pdbname.c_str(), 0));
    return result;
}
#endif // _MSC_VER

static void cr_pe_section_save(cr_plugin &ctx, cr_plugin_section_type::e type,
                               int64_t vaddr, int64_t base,
                               IMAGE_SECTION_HEADER &shdr) {
    const auto version = cr_plugin_section_version::current;
    auto p = (cr_internal *)ctx.p;
    auto data = &p->data[type][version];
    const size_t old_size = data->size;
    data->base = base;
    data->ptr = (char *)vaddr;
    data->size = shdr.SizeOfRawData;
    data->data = realloc(data->data, shdr.SizeOfRawData);
    if (old_size < shdr.SizeOfRawData) {
        memset((char *)data->data + old_size, '\0',
               shdr.SizeOfRawData - old_size);
    }
}

static bool cr_plugin_validate_sections(cr_plugin &ctx, so_handle handle,
                                        const std::string &imagefile,
                                        bool rollback) {
    (void)imagefile;
    assert(handle);
    auto p = (cr_internal *)ctx.p;
    if (p->mode == CR_DISABLE) {
        return true;
    }
    auto ntHeaders = ImageNtHeader(handle);
    auto base = ntHeaders->OptionalHeader.ImageBase;
    auto sectionHeaders = (IMAGE_SECTION_HEADER *)(ntHeaders + 1);
    bool result = true;
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i) {
        auto sectionHeader = sectionHeaders[i];
        const int64_t size = sectionHeader.SizeOfRawData;
        if (!strcmp((const char *)sectionHeader.Name, ".state")) {
            if (ctx.version || rollback) {
                result &= cr_plugin_section_validate(
                    ctx, cr_plugin_section_type::state,
                    base + sectionHeader.VirtualAddress, base, size);
            }
            if (result) {
                auto sec = cr_plugin_section_type::state;
                cr_pe_section_save(ctx, sec,
                                   base + sectionHeader.VirtualAddress, base,
                                   sectionHeader);
            }
        } else if (!strcmp((const char *)sectionHeader.Name, ".bss")) {
            if (ctx.version || rollback) {
                result &= cr_plugin_section_validate(
                    ctx, cr_plugin_section_type::bss,
                    base + sectionHeader.VirtualAddress, base, size);
            }
            if (result) {
                auto sec = cr_plugin_section_type::bss;
                cr_pe_section_save(ctx, sec,
                                   base + sectionHeader.VirtualAddress, base,
                                   sectionHeader);
            }
        }
    }
    return result;
}

static void cr_so_unload(cr_plugin &ctx) {
    auto p = (cr_internal *)ctx.p;
    assert(p->handle);
    FreeLibrary((HMODULE)p->handle);
}

static so_handle cr_so_load(cr_plugin &ctx, const std::string &filename) {
    auto new_dll = LoadLibrary(filename.c_str());
    if (!new_dll) {
        fprintf(stderr, "Couldn't load plugin: %d\n", GetLastError());
    }
    return new_dll;
}

static cr_plugin_main_func cr_so_symbol(so_handle handle) {
    assert(handle);
    auto new_main = (cr_plugin_main_func)GetProcAddress(handle, "cr_main");
    if (!new_main) {
        fprintf(stderr, "Couldn't find plugin entry point: %d\n",
                GetLastError());
    }
    return new_main;
}

static void cr_plat_init() {
}

static int cr_seh_filter(cr_plugin &ctx, unsigned long seh) {
    if (ctx.version == 1) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    switch (seh) {
    case EXCEPTION_ACCESS_VIOLATION:
        ctx.failure = CR_SEGFAULT;
        return EXCEPTION_EXECUTE_HANDLER;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        ctx.failure = CR_ILLEGAL;
        return EXCEPTION_EXECUTE_HANDLER;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        ctx.failure = CR_MISALIGN;
        return EXCEPTION_EXECUTE_HANDLER;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        ctx.failure = CR_BOUNDS;
        return EXCEPTION_EXECUTE_HANDLER;
    case EXCEPTION_STACK_OVERFLOW:
        ctx.failure = CR_STACKOVERFLOW;
        return EXCEPTION_EXECUTE_HANDLER;
    default:
        break;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static int cr_plugin_main(cr_plugin &ctx, cr_op operation) {
    auto p = (cr_internal *)ctx.p;
#ifndef __MINGW32__
    __try {
#endif
        if (p->main) {
            return p->main(&ctx, operation);
        }
#ifndef __MINGW32__
    } __except (cr_seh_filter(ctx, GetExceptionCode())) {
        return -1;
    }
#endif
    return -1;
}

#endif // _WIN32

#if defined(__unix__)

#include <csignal>
#include <cstring>
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ucontext.h>
#include <unistd.h>

#define CR_STR "%s"
#define CR_INT "%d"

using so_handle = void *;

static size_t cr_file_size(const std::string &path) {
    struct stat stats {};
    if (stat(path.c_str(), &stats) == -1) {
        return 0;
    }
    return static_cast<size_t>(stats.st_size);
}

static time_t cr_last_write_time(const std::string &path) {
    struct stat stats {};
    if (stat(path.c_str(), &stats) == -1) {
        return -1;
    }
    return stats.st_mtim.tv_sec;
}

static bool cr_exists(const std::string &path) {
    struct stat stats {};
    return stat(path.c_str(), &stats) != -1;
}

static void cr_copy(const std::string &from, const std::string &to) {
    char buffer[BUFSIZ];
    size_t size;

    FILE *source = fopen(from.c_str(), "rb");
    FILE *destination = fopen(to.c_str(), "wb");

    while ((size = fread(buffer, 1, BUFSIZ, source)) != 0) {
        fwrite(buffer, 1, size, destination);
    }

    fclose(source);
    fclose(destination);
}

// unix,internal
// a helper function to validate that an area of memory is empty
// this is used to validate that the data in the .bss haven't changed
// and that we are safe to discard it and uses the new one.
bool cr_is_empty(const void *const buf, int64_t len) {
    if (!buf || !len) {
        return true;
    }

    bool r = false;
    auto c = (const char *const)buf;
    for (int i = 0; i < len; ++i) {
        r |= c[i];
    }
    return !r;
}

// unix,internal
// save section information to be used during load/unload when copying
// around global state (from .bss and .state binary sections).
// vaddr = is the in memory loaded address of the segment-section
// base = is the in file section address
// shdr = the in file section header
template <class H>
void cr_elf_section_save(cr_plugin &ctx, cr_plugin_section_type::e type,
                         int64_t vaddr, int64_t base, H shdr) {
    const auto version = cr_plugin_section_version::current;
    auto p = (cr_internal *)ctx.p;
    auto data = &p->data[type][version];
    const size_t old_size = data->size;
    data->base = base;
    data->ptr = (char *)vaddr;
    data->size = shdr.sh_size;
    data->data = realloc(data->data, shdr.sh_size);
    if (old_size < shdr.sh_size) {
        memset((char *)data->data + old_size, '\0', shdr.sh_size - old_size);
    }
}

// unix,internal
// validates that the sections being loaded are compatible with the previous
// one accordingly with desired `cr_mode` mode. If this is a first load, a
// validation is not necessary. At the same time it will initialize the
// section tracking information and alloc the required temporary space to use
// during unload.
template <class H>
bool cr_elf_validate_sections(cr_plugin &ctx, bool rollback, H shdr, int shnum,
                              const char *sh_strtab_p) {
    assert(sh_strtab_p);
    auto p = (cr_internal *)ctx.p;
    bool result = true;
    for (int i = 0; i < shnum; ++i) {
        const char *name = sh_strtab_p + shdr[i].sh_name;
        auto sectionHeader = shdr[i];
        const int64_t addr = sectionHeader.sh_addr;
        const int64_t size = sectionHeader.sh_size;
        const int64_t base = (intptr_t)p->seg.ptr + p->seg.size;
        if (!strcmp(name, ".state")) {
            const int64_t vaddr = base - size;
            auto sec = cr_plugin_section_type::state;
            if (ctx.version || rollback) {
                result &=
                    cr_plugin_section_validate(ctx, sec, vaddr, addr, size);
            }
            if (result) {
                cr_elf_section_save(ctx, sec, vaddr, addr, sectionHeader);
            }
        } else if (!strcmp(name, ".bss")) {
            // .bss goes past segment filesz, but it may be just padding
            const int64_t vaddr = base;
            auto sec = cr_plugin_section_type::bss;
            if (ctx.version || rollback) {
                // this is kinda hack to skip bss validation if our data is zero
                // this means we don't care scrapping it, and helps skipping
                // validating a .bss that serves only as padding in the segment.
                if (!cr_is_empty(p->data[sec][0].data, p->data[sec][0].size)) {
                    result &=
                        cr_plugin_section_validate(ctx, sec, vaddr, addr, size);
                }
            }
            if (result) {
                cr_elf_section_save(ctx, sec, vaddr, addr, sectionHeader);
            }
        }
    }
    return result;
}

struct cr_ld_data {
    cr_plugin *ctx = nullptr;
    int64_t data_segment_address = 0;
    int64_t data_segment_size = 0;
    const char *fullname = nullptr;
};

// Iterate over all loaded shared objects and then for each one, iterates
// over each segment.
// So we find our plugin by filename and try to find the segment that
// contains our data sections (.state and .bss) to find their virtual
// addresses.
// We search segments with type PT_LOAD (1), meaning it is a loadable
// segment (anything that really matters ie. .text, .data, .bss, etc...)
// The segment where the p_memsz is bigger than p_filesz is the segment
// that contains the section .bss (if there is one or there is padding).
// Also, the segment will have sensible p_flags value (PF_W for exemple).
//
// Some useful references:
// http://www.skyfree.org/linux/references/ELF_Format.pdf
// https://eli.thegreenplace.net/2011/08/25/load-time-relocation-of-shared-libraries/
static int cr_dl_header_handler(struct dl_phdr_info *info, size_t size,
                                void *data) {
    assert(info && data);
    auto p = (cr_ld_data *)data;
    auto ctx = p->ctx;
    if (strcasecmp(info->dlpi_name, p->fullname)) {
        return 0;
    }

    for (int i = 0; i < info->dlpi_phnum; i++) {
        auto phdr = info->dlpi_phdr[i];
        if (phdr.p_type != PT_LOAD) {
            continue;
        }

        // assume the first writable segment is the one that contains our
        // sections this may not be true I imagine, but if this becomes an
        // issue we fix it by comparing against section addresses, but this
        // will require some rework on the code flow.
        if (phdr.p_flags & PF_W) {
            auto pimpl = (cr_internal *)ctx->p;
            pimpl->seg.ptr = (char *)(info->dlpi_addr + phdr.p_vaddr);
            pimpl->seg.size = phdr.p_filesz;
            break;
        }
    }
    return 0;
}

static bool cr_plugin_validate_sections(cr_plugin &ctx, so_handle handle,
                                        const std::string &imagefile,
                                        bool rollback) {
    assert(handle);
    cr_ld_data data;
    data.ctx = &ctx;
    auto pimpl = (cr_internal *)ctx.p;
    if (pimpl->mode == CR_DISABLE) {
        return true;
    }
    data.fullname = imagefile.c_str();
    dl_iterate_phdr(cr_dl_header_handler, (void *)&data);

    const auto len = cr_file_size(imagefile);
    char *p = nullptr;
    bool result = false;
    do {
        int fd = open(imagefile.c_str(), O_RDONLY);
        p = (char *)mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);

        auto ehdr = (Elf32_Ehdr *)p;
        if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
            ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
            ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
            ehdr->e_ident[EI_MAG3] != ELFMAG3) {
            break;
        }

        if (ehdr->e_ident[EI_CLASS] == ELFCLASS32) {
            auto shdr = (Elf32_Shdr *)(p + ehdr->e_shoff);
            auto sh_strtab = &shdr[ehdr->e_shstrndx];
            const char *const sh_strtab_p = p + sh_strtab->sh_offset;
            result = cr_elf_validate_sections(ctx, rollback, shdr,
                                              ehdr->e_shnum, sh_strtab_p);
        } else {
            auto ehdr = (Elf64_Ehdr *)p; // shadow
            auto shdr = (Elf64_Shdr *)(p + ehdr->e_shoff);
            auto sh_strtab = &shdr[ehdr->e_shstrndx];
            const char *const sh_strtab_p = p + sh_strtab->sh_offset;
            result = cr_elf_validate_sections(ctx, rollback, shdr,
                                              ehdr->e_shnum, sh_strtab_p);
        }
    } while (0);

    if (p) {
        munmap(p, len);
    }

    if (!result) {
        ctx.failure = CR_STATE_INVALIDATED;
    }

    return result;
}

static void cr_so_unload(cr_plugin &ctx) {
    assert(ctx.p);
    auto p = (cr_internal *)ctx.p;
    assert(p->handle);

    const int r = dlclose(p->handle);
    if (r) {
        fprintf(stderr, "Error closing plugin: %d\n", r);
    }

    p->handle = nullptr;
    p->main = nullptr;
}

static so_handle cr_so_load(cr_plugin &ctx, const std::string &new_file) {
    dlerror();
    auto new_dll = dlopen(new_file.c_str(), RTLD_NOW);
    if (!new_dll) {
        fprintf(stderr, "Couldn't load plugin: %s\n", dlerror());
    }
    return new_dll;
}

static cr_plugin_main_func cr_so_symbol(so_handle handle) {
    assert(handle);
    dlerror();
    auto new_main = (cr_plugin_main_func)dlsym(handle, "cr_main");
    if (!new_main) {
        fprintf(stderr, "Couldn't find plugin entry point: %s\n", dlerror());
    }
    return new_main;
}

volatile std::sig_atomic_t cr_signal = 0;
sigjmp_buf env;

static void cr_signal_handler(int sig, siginfo_t *si, void *uap) {
    (void)uap;
    assert(si);
    //printf("Signal %d raised at address: %p\n", sig, si->si_addr);
    // we may want to pass more info about the failure here
    cr_signal = sig;
    siglongjmp(env, sig);
}

static void cr_plat_init() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_RESTART | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = cr_signal_handler;
    sa.sa_restorer = nullptr;

    if (sigaction(SIGILL, &sa, nullptr) == -1) {
        fprintf(stderr, "Failed to setup SIGILL handler\n");
    }
    if (sigaction(SIGBUS, &sa, nullptr) == -1) {
        fprintf(stderr, "Failed to setup SIGBUS handler\n");
    }
    if (sigaction(SIGSEGV, &sa, nullptr) == -1) {
        fprintf(stderr, "Failed to setup SIGSEGV handler\n");
    }
    if (sigaction(SIGABRT, &sa, nullptr) == -1) {
        fprintf(stderr, "Failed to setup SIGABRT handler\n");
    }
}

static cr_failure cr_signal_to_failure(int sig) {
    switch (sig) {
    case 0:
        return CR_NONE;
    case SIGILL:
        return CR_ILLEGAL;
    case SIGBUS:
        return CR_MISALIGN;
    case SIGSEGV:
        return CR_SEGFAULT;
    case SIGABRT:
        return CR_ABORT;
    }
    return static_cast<cr_failure>(CR_OTHER + sig);
}

static int cr_plugin_main(cr_plugin &ctx, cr_op operation) {
    if (sigsetjmp(env, 0)) {
        ctx.failure = cr_signal_to_failure(cr_signal);
        cr_signal = 0;
        return -1;
    } else {
        auto p = (cr_internal *)ctx.p;
        assert(p);
        if (p->main) {
            return p->main(&ctx, operation);
        }
    }

    return -1;
}
#endif // __unix__

static bool cr_plugin_load_internal(cr_plugin &ctx, bool rollback) {
    auto p = (cr_internal *)ctx.p;
    const auto file = p->fullname;
    if (cr_exists(file) || rollback) {
        const auto new_file = cr_version_path(file, ctx.version);

        const bool close = false;
        cr_plugin_unload(ctx, rollback, close);
        if (!rollback) {
            cr_copy(file, new_file);

#if defined(_MSC_VER)
            auto new_pdb = cr_replace_extension(new_file, ".pdb");

            if (!cr_pdb_process(new_file, new_pdb)) {
                fprintf(stderr, "Couldn't process PDB, debugging may be "
                                "affected and/or reload may fail\n");
            }
#endif // defined(_MSC_VER)
        }

        auto new_dll = cr_so_load(ctx, new_file);
        if (!new_dll) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // we may want set a failure reason and avoid sleeping ourselves.
            // this may happen mostly due to compiler still writing the binary
            // to the disk, so for now we just sleep a bit, but ideally we
            // may want to report this to the user to deal with it.
            return false;
        }

        if (!cr_plugin_validate_sections(ctx, new_dll, new_file, rollback)) {
            return false;
        }

        if (rollback) {
            cr_plugin_sections_reload(ctx, cr_plugin_section_version::backup);
        } else if (ctx.version) {
            cr_plugin_sections_reload(ctx, cr_plugin_section_version::current);
        }

        auto new_main = cr_so_symbol(new_dll);
        if (!new_main) {
            return false;
        }
        
        auto p = (cr_internal *)ctx.p;
        p->handle = new_dll;
        p->main = new_main;
        p->timestamp = cr_last_write_time(file);
        ctx.version++;
    } else {
        fprintf(stderr, "Error loading plugin.\n");
        return false;
    }
    return true;
}

static bool cr_plugin_section_validate(cr_plugin &ctx,
                                       cr_plugin_section_type::e type,
                                       intptr_t ptr, intptr_t base,
                                       int64_t size) {
    (void)ptr;
    auto p = (cr_internal *)ctx.p;
    switch (p->mode) {
    case CR_SAFE:
        return (p->data[type][0].size == size);
    case CR_UNSAFE:
        return (p->data[type][0].size <= size);
    case CR_DISABLE:
        return true;
    default:
        break;
    }
    // CR_SAFEST
    return (p->data[type][0].base == base && p->data[type][0].size == size);
}

// internal
static void cr_plugin_sections_backup(cr_plugin &ctx) {
    auto p = (cr_internal *)ctx.p;
    if (p->mode == CR_DISABLE) {
        return;
    }
    for (int i = 0; i < cr_plugin_section_type::count; ++i) {
        auto cur = &p->data[i][cr_plugin_section_version::current];
        if (cur->ptr) {
            auto bkp = &p->data[i][cr_plugin_section_version::backup];
            bkp->data = realloc(bkp->data, cur->size);
            bkp->ptr = cur->ptr;
            bkp->size = cur->size;
            bkp->base = cur->base;

            if (bkp->data) {
                std::memcpy(bkp->data, cur->data, bkp->size);
            }
        }
    }
}

// internal
// Before unloading iterate over possible global static state and keeps an
// internal copy to be used in next version load and a backup copy as a known
// valid state checkpoint. This is mostly due that a new load may want to
// modify the state and if anything bad happens we are sure to have a valid
// and compatible copy of the state for the previous version of the plugin.
static void cr_plugin_sections_store(cr_plugin &ctx) {
    auto p = (cr_internal *)ctx.p;
    if (p->mode == CR_DISABLE) {
        return;
    }
    auto version = cr_plugin_section_version::current;
    for (int i = 0; i < cr_plugin_section_type::count; ++i) {
        if (p->data[i][version].ptr && p->data[i][version].data) {
            const char *ptr = p->data[i][version].ptr;
            const int64_t len = p->data[i][version].size;
            std::memcpy(p->data[i][version].data, ptr, len);
        }
    }

    cr_plugin_sections_backup(ctx);
}

// internal
// After a load happens reload the global state from previous version from our
// internal copy created during the unload step.
static void cr_plugin_sections_reload(cr_plugin &ctx,
                                      cr_plugin_section_version::e version) {
    assert(version < cr_plugin_section_version::count);
    auto p = (cr_internal *)ctx.p;
    if (p->mode == CR_DISABLE) {
        return;
    }
    for (int i = 0; i < cr_plugin_section_type::count; ++i) {
        if (p->data[i][version].data) {
            const int64_t len = p->data[i][version].size;
            // restore backup into the current section address as it may
            // change due aslr and backup address may be invalid
            const auto current = cr_plugin_section_version::current;
            auto dest = (void *)p->data[i][current].ptr;
            if (dest) {
                std::memcpy(dest, p->data[i][version].data, len);
            }
        }
    }
}

// internal
// Cleanup and frees any temporary memory used to keep global static data
// between sessions, used during shutdown.
static void cr_so_sections_free(cr_plugin &ctx) {
    auto p = (cr_internal *)ctx.p;
    for (int i = 0; i < cr_plugin_section_type::count; ++i) {
        for (int v = 0; v < cr_plugin_section_version::count; ++v) {
            if (p->data[i][v].data) {
                free(p->data[i][v].data);
            }
            p->data[i][v].data = nullptr;
        }
    }
}

static bool cr_plugin_changed(cr_plugin &ctx) {
    auto p = (cr_internal *)ctx.p;
    const auto src = cr_last_write_time(p->fullname);
    const auto cur = p->timestamp;
    return src > cur;
}

// internal
// Unload current running plugin, if it is not a rollback it will trigger a
// last update with `cr_op::CR_UNLOAD` (that may crash and cause another
// rollback, etc.) storing global static states to use with next load. If the
// unload is due a rollback, no `cr_op::CR_UNLOAD` is called neither any state
// is saved, giving opportunity to the previous version to continue with valid
// previous state.
static void cr_plugin_unload(cr_plugin &ctx, bool rollback, bool close) {
    auto p = (cr_internal *)ctx.p;
    if (p->handle) {
        if (!rollback) {
            cr_plugin_main(ctx, close ? CR_CLOSE : CR_UNLOAD);
            cr_plugin_sections_store(ctx);
        }
        cr_so_unload(ctx);
        p->handle = nullptr;
        p->main = nullptr;
    }
}

// internal
// Force a version rollback, causing a partial-unload and a load with the
// previous version, also triggering an update with `cr_op::CR_LOAD` that
// in turn may also cause more rollbacks.
static bool cr_plugin_rollback(cr_plugin &ctx) {
    if (ctx.version > 1) {
        ctx.version -= 2;
    }
    auto loaded = cr_plugin_load_internal(ctx, true);
    if (loaded) {
        loaded = cr_plugin_main(ctx, CR_LOAD) >= 0;
        if (loaded) {
            ctx.failure = CR_NONE;
        }
    }
    return loaded;
}

// internal
// Checks if a rollback or a reload is needed, do the unload/loading and call
// update one time with `cr_op::CR_LOAD`. Note that this may fail due to crash
// handling during this first update, effectivelly rollbacking if possible and
// causing a consecutive `CR_LOAD` with the previous version.
static void cr_plugin_reload(cr_plugin &ctx) {
    if (cr_plugin_changed(ctx)) {
        cr_plugin_load_internal(ctx, false);
        int r = cr_plugin_main(ctx, CR_LOAD);
        if (r < 0 && !ctx.failure) {
            ctx.failure = CR_USER;
        }
    }
}

// This is basically the plugin `main` function, should be called as
// frequently as your core logic/application needs. -1 and -2 are the only
// possible return values from cr meaning a fatal error (causes rollback),
// other return values are returned directly from `cr_main`.
extern "C" int cr_plugin_update(cr_plugin &ctx) {
    if (ctx.failure) {
        cr_plugin_rollback(ctx);
    } else {
        cr_plugin_reload(ctx);
    }

    // -2 to differentiate from crash handling code path, meaning the crash
    // happened probably during load or unload and not update
    if (ctx.failure) {
        return -2;
    }

    int r = cr_plugin_main(ctx, CR_STEP);
    if (r < 0 && !ctx.failure) {
        ctx.failure = CR_USER;
    }
    return r;
}

// Loads a plugin from the specified full path (or current directory if NULL).
extern "C" bool cr_plugin_load(cr_plugin &ctx, const char *fullpath) {
    assert(fullpath);
    auto p = new cr_internal;
    p->mode = CR_OP_MODE;
    p->fullname = fullpath;
    ctx.p = p;
    ctx.version = 0;
    ctx.failure = CR_NONE;
    cr_plat_init();
    return true;
}

// Call to cleanup internal state once the plugin is not required anymore.
extern "C" void cr_plugin_close(cr_plugin &ctx) {
    const bool rollback = false;
    const bool close = true;
    cr_plugin_unload(ctx, rollback, close);
    cr_so_sections_free(ctx);
    auto p = (cr_internal *)ctx.p;
    delete p;
    ctx.p = nullptr;
    ctx.version = 0;
}

#endif // #ifndef CR_HOST

#endif // __CR_H__
// clang-format off
/*
```

</details>
*/
