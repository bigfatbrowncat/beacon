// Copyright 2019 Google LLC.
// Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.

#include "SkLoadICU.h"

#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

#include "unicode/udata.h"

/*
static void* win_mmap(const char* dataFile) {
    if (!dataFile) {
        return nullptr;
    }
    struct FCloseWrapper { void operator()(FILE* f) { fclose(f); } };
    std::unique_ptr<FILE, FCloseWrapper> stream(fopen(dataFile, "rb"));
    if (!stream) {
        fprintf(stderr, "SkIcuLoader: datafile missing.\n");
        return nullptr;
    }
    int fileno = _fileno(stream.get());
    if (fileno < 0) {
        fprintf(stderr, "SkIcuLoader: datafile fileno error.\n");
        return nullptr;
    }
    HANDLE file = (HANDLE)_get_osfhandle(fileno);
    if ((HANDLE)INVALID_HANDLE_VALUE == file) {
        fprintf(stderr, "SkIcuLoader: datafile handle error.\n");
        return nullptr;
    }
    struct CloseHandleWrapper { void operator()(HANDLE h) { CloseHandle(h); } };
    std::unique_ptr<void, CloseHandleWrapper> mmapHandle(
        CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr));
    if (!mmapHandle) {
        fprintf(stderr, "SkIcuLoader: datafile mmap error.\n");
        return nullptr;
    }
    void* addr = MapViewOfFile(mmapHandle.get(), FILE_MAP_READ, 0, 0, 0);
    if (nullptr == addr) {
        fprintf(stderr, "SkIcuLoader: datafile view error.\n");
        return nullptr;
    }
    return addr;
}
*/

static bool init_icu(void* addr) {
    UErrorCode err = U_ZERO_ERROR;
    udata_setCommonData(addr, &err);
    if (err != U_ZERO_ERROR) {
        fprintf(stderr, "udata_setCommonData() returned %d.\n", (int)err);
        return false;
    }
    udata_setFileAccess(UDATA_ONLY_PACKAGES, &err);
    if (err != U_ZERO_ERROR) {
        fprintf(stderr, "udata_setFileAccess() returned %d.\n", (int)err);
        return false;
    }
    return true;
}

/*static std::string executable_directory() {
    HMODULE hModule = GetModuleHandleA(NULL);
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);
    const char* end = strrchr(path, '\\');
    return end ? std::string(path, end - path) : std::string();
}
*/

extern "C" uint8_t* icudtl_dat_begin;     /* binary data         */
extern "C" uint8_t* icudtl_dat_end; /* size of binary data */

bool SkLoadICU() {
    static bool good = false;
    static std::once_flag flag;
    std::call_once(flag, []() {
        /*std::string sPath = executable_directory();
        sPath += "\\icudtl.dat";
        if (void* addr = win_mmap(sPath.c_str())) {
            if (init_icu(addr)) {
                good = true;
            }
        }*/

#ifndef WIN32
    uint8_t* icudtl_dat = icudtl_dat_begin; /* binary data         */
    size_t icudtl_dat_size = icudtl_dat_end - icudtl_dat_begin; /* size of binary data */
#endif

        if (init_icu(icudtl_dat)) {
            good = true;
        }
    });
    return good;
}
