/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*++

Copyright (c) Microsoft Corporation

Module Name:

    dt_symsrv.cpp

Abstract:

    This file implements the symbol server to support fbt provider name
    resolution.

    N.B. Though this symbol server starts a separate thread to handle FBT name
         resolution, calls are only expected to happen when main thread is
         blocked on IO control to the driver creating probes, so single-threded
         design of the dbghelp.dll is not an issue for this implementation.

--*/

#include <ntcompat.h>
#include <ntdtrace.h>
#include <cvconst.h>
#include <malloc.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <forward_list>
#include <initializer_list>
#include "dt_symsrv.h"
#include "dt_disasm.h"

// These references to the common libdtrace framework are declared here
// independently so that this file can be easily included into test code that
// compiles separately from dtrace.
extern "C" int strisglob(const char *s);
extern "C" void dt_dprintf(const char *, ...);

using namespace std::literals;

// This local definition exists so that dtrace can build with SDKs not containing
// the new IMAGEHLP_SYMBOL_TYPE_INFO definition. Retrieval of 'this' pointer should
// fail gracefully in old versions.
#define TI_GET_OBJECTPOINTERTYPE 34

struct dt_symsrv {
    HANDLE Thread;
    HANDLE Device;
    volatile BOOL Exiting;
};

namespace {

struct delete_hlocal_string {
    void operator()(PWCHAR str) { LocalFree(str); };
};

using unique_hlocal_string = std::unique_ptr<WCHAR[], delete_hlocal_string>;

std::string str_concat(std::string_view a, std::string_view b)
{
    std::string ret;

    ret.reserve(a.length() + b .length());
    ret = a;
    ret.append(b);
    return std::move(ret);
}

std::string str_concat(const std::initializer_list<std::string_view>& list)
{

    size_t len = 0;
    for (auto& view : list) {
        len += view.size();
    }

    std::string ret;
    ret.reserve(len);
    for (auto& view : list) {
        ret.append(view);
    }

    return std::move(ret);
}

struct dt_symsrv_function_t {
    dt_symsrv_function_t(std::string &&name, ULONG size, bool vaArgs, std::vector<std::string> &&paramTypes) noexcept
        : Name(std::move(name))
        , Size(size)
        , VaArgs(vaArgs)
        , ParamTypes(std::move(paramTypes))
    {
    }

    ULONG Size;
    const bool VaArgs;
    std::string Name;

    // First parameter is the return type.
    std::vector<std::string> ParamTypes;

    // Due to identical code folding, the same RVA may refer to multiple
    // functions.
    std::forward_list<std::string> AltNames;

    template <typename TCallback>
    bool ForAllNames(TCallback &&func) noexcept(noexcept(func)) {
        if (func(Name)) {
            return true;
        }

        for (const auto &name : AltNames) {
            if (func(name)) {
                return true;
            }
        }

        return false;
    }
};

using rva_to_func_map_t = std::map<ULONG, dt_symsrv_function_t>;

struct dt_symsrv_function_enum_ctx_t {
    rva_to_func_map_t RvaToFunction;
    const std::string_view ModuleName;
    HANDLE Process;
};

struct dt_symsrv_type_ctx_t {
    const HANDLE Process;
    const ULONGLONG DebugBase;
    const std::string_view ModuleName;

    std::string typestr(ULONG TypeId);

private:
    std::string udt(ULONG TypeId);
    std::string pointer(ULONG TypeId);
    std::string array(ULONG TypeId);
    std::string basetype(ULONG TypeId);
};

} // end anonymous namespace

std::string
dt_symsrv_type_ctx_t::udt(ULONG TypeId)
{
    unique_hlocal_string typeNameWLocal;
    SymGetTypeInfo(this->Process, this->DebugBase, TypeId,
                   TI_GET_SYMNAME, &typeNameWLocal);

    if (!typeNameWLocal || typeNameWLocal[0] == 0) {
        return "__$unknownUDT";
    }

    std::wstring_view typeNameW(typeNameWLocal.get());

    const auto transLen = typeNameW.length() + 1;
    PCHAR typeNameTrans = (PCHAR)_alloca(transLen);
    typeNameTrans[0] = '`';
    size_t pos = 1;
    for (auto ch : typeNameW) {
        typeNameTrans[pos++] = (char)ch;
    }

    std::string_view typeName(typeNameTrans, transLen);

    if (typeName.find_first_of(":< ") != typeName.npos) {
        // If the type name is something that looks like a template
        // or has another reserved character, wrap it in an __identifier
        // keyword.
        return str_concat({ "__identifier(\""sv, this->ModuleName, typeName, "\")"sv });
    }

    return str_concat(this->ModuleName, typeName);;
}

std::string
dt_symsrv_type_ctx_t::pointer(ULONG TypeId)
{
    std::string BaseTypeName;
    ULONG baseType;
    if (SymGetTypeInfo(this->Process, this->DebugBase, TypeId,
                       TI_GET_TYPEID, &baseType)) {

        BaseTypeName = this->typestr(baseType);
    }

    if (BaseTypeName.empty()) {
        return "void*";
    }

    return BaseTypeName + "*";
}

std::string
dt_symsrv_type_ctx_t::array(ULONG TypeId)
{
    std::string ElementTypeName;
    ULONG typeIdElement;

    ULONGLONG Length = 0;
    CHAR ArrayStr[100];
    if (SymGetTypeInfo(this->Process, this->DebugBase, TypeId,
                       TI_GET_LENGTH, &Length)) {

        sprintf(ArrayStr, "[%I64d]", Length);

    } else {
        strcpy(ArrayStr, "[]");
    }

    if (SymGetTypeInfo(this->Process, this->DebugBase, TypeId,
                       TI_GET_TYPEID, &typeIdElement)) {
        ElementTypeName = this->typestr(typeIdElement);
    }

    if (ElementTypeName.empty()) {
        return "void*";
    }
    return ElementTypeName + ArrayStr;
}

std::string
dt_symsrv_type_ctx_t::basetype(ULONG TypeId)
{
    ULONG baseType;
    ULONG64 length;
    if (!SymGetTypeInfo(this->Process, this->DebugBase, TypeId,
                        TI_GET_BASETYPE, &baseType) ||
        !SymGetTypeInfo(this->Process, this->DebugBase, TypeId,
            TI_GET_LENGTH, &length)) {

        return "__$unknownBaseType";
    }

    std::string_view prefix;
    std::string_view type;
    switch (baseType) {
    case btWChar:
    case btChar:
        switch (length) {
        case 1: type = "char"sv; break;
        case 2: type = "wchar_t"sv; break;
        }

        break;

    case btUInt:
    case btULong:
        prefix = "unsigned "sv;
        __fallthrough;
    case btVoid:
    case btInt:
    case btLong:
        switch (length) {
        case 0: type = "void"sv; break;
        case 1: type = "char"sv; break;
        case 2: type = "short"sv; break;
        case 4: type = "long"sv; break;
        case 8: type = "long long"sv; break;
        }

        break;

    case btFloat :
        switch (length) {
        case 4: type = "float"sv; break;
        case 8: type = "double"sv; break;
        }

        break;

    case btBool:
        switch (length) {
        case 1: type = "bool"sv; break;
        case 4: type = "`BOOL"sv; break;
        }

        break;

    case btHresult:
        type = "`HRESULT"sv; break;
        break;
    }

    if (type.empty()) {
        return "__$unknownBaseType";
    }

    if (!prefix.empty()) {
        return str_concat(prefix, type);
    } else {
        if (type[0] == '`') {
            return str_concat(this->ModuleName, type);
        }

        return std::string(type);
    }
}

std::string
dt_symsrv_type_ctx_t::typestr(ULONG TypeId)
{
    ULONG SymTag;
    if (!SymGetTypeInfo(this->Process, this->DebugBase, TypeId, TI_GET_SYMTAG, &SymTag)) {
        return {};
    }

    switch (SymTag) {
    case SymTagBaseType:
        return this->basetype(TypeId);
    case SymTagUDT:
    case SymTagEnum:
        return this->udt(TypeId);
    case SymTagPointerType:
        return this->pointer(TypeId);
    case SymTagFunctionType:
        return "void*";
    case SymTagArrayType:
        return this->array(TypeId);
    }

    return "__$unknownType";
}

static std::string
dt_symsrv_typestr(HANDLE Process, std::string_view ModuleName, ULONGLONG Base, ULONG TypeId)
{
    return dt_symsrv_type_ctx_t{ Process, Base, ModuleName }.typestr(TypeId);
}

dt_symsrv_param_types_t
dt_symsrv_load_paramtypes(HANDLE Process, std::string_view ModuleName,
                          ULONGLONG Base, ULONG TypeIndex)
{
    if (0 == TypeIndex) {
        return {};
    }

    ULONG SymTag;
    if (!SymGetTypeInfo(Process, Base, TypeIndex,
                        TI_GET_SYMTAG, &SymTag)) {
        return {};
    }

    if (SymTagFunctionType != SymTag) {
        return {};
    }

    ULONG retTypeId;
    if (!SymGetTypeInfo(Process, Base, TypeIndex,
        TI_GET_TYPEID, &retTypeId)) {
        return {};
    }

    auto retType = dt_symsrv_typestr(Process, ModuleName, Base, retTypeId);
    if (retType.empty()) {
        return {};
    }

    bool VaArgs = false;

    ULONG reserveCount = 1;
    ULONG thisTypeId{};
    std::string thisTypeName;
    if (SymGetTypeInfo(Process, Base, TypeIndex,
                       (IMAGEHLP_SYMBOL_TYPE_INFO)TI_GET_OBJECTPOINTERTYPE, &thisTypeId) && thisTypeId) {
        reserveCount += 1;
        thisTypeName = dt_symsrv_typestr(Process, ModuleName, Base, thisTypeId);
        if (thisTypeName.empty()) {
            return {};
        }
    }

    ULONG paramCount = 0;
    if (!SymGetTypeInfo(Process, Base, TypeIndex,
        TI_GET_CHILDRENCOUNT, &paramCount)) {
        return {};
    }

    reserveCount += paramCount;

    std::vector<std::string> paramTypes;
    paramTypes.reserve(reserveCount);

    paramTypes.emplace_back(std::move(retType));
    if (!thisTypeName.empty()) {
        paramTypes.emplace_back(std::move(thisTypeName));
    }

    if (paramCount != 0) {
        auto* Params = (TI_FINDCHILDREN_PARAMS*)
            _alloca(sizeof(TI_FINDCHILDREN_PARAMS) + paramCount * sizeof(ULONG));

        Params->Count = paramCount;
        Params->Start = 0;
        if (!SymGetTypeInfo(Process, Base, TypeIndex,
                            TI_FINDCHILDREN, Params)) {
            return {};
        }

        for (ULONG i = 0; i < paramCount; i++) {
            ULONG typeId;
            if (!SymGetTypeInfo(Process, Base, Params->ChildId[i],
                                TI_GET_TYPEID, &typeId)) {
                return {};
            }

            ULONG baseType;
            if (((i + 1) == paramCount) &&
                SymGetTypeInfo(Process, Base, typeId,
                               TI_GET_BASETYPE, &baseType) &&
                (btNoType == baseType)) {

                VaArgs = true;
                break;
            }

            auto paramType = dt_symsrv_typestr(Process, ModuleName, Base, typeId);
            if (paramType.empty()) {
                return {};
            }
            paramTypes.emplace_back(std::move(paramType));
        }
    }

    return {std::move(paramTypes), VaArgs};
}

static BOOL dt_symsrv_is_function(PSYMBOL_INFO SymInfo)
{
    if (SymTagFunction == SymInfo->Tag) {
        return TRUE;
    }

    if ((SymTagPublicSymbol == SymInfo->Tag) &&
        (0 != (SymInfo->Flags & (SYMFLAG_EXPORT |
                                 SYMFLAG_FUNCTION |
                                 SYMFLAG_PUBLIC_CODE)))) {

        return TRUE;
    }

    return FALSE;
}

static void dt_symsrv_add_function(dt_symsrv_function_enum_ctx_t &ctx,
    ULONG Rva, ULONG Size, std::string_view Name, ULONG TypeIndex, ULONGLONG ModBase,
    bool AllowMultiple) noexcept
{
    try {
        auto f = ctx.RvaToFunction.lower_bound(Rva);

        if (f == ctx.RvaToFunction.end() || (f->first != Rva)) {
            auto paramTypes = dt_symsrv_load_paramtypes(ctx.Process, ctx.ModuleName, ModBase, TypeIndex);

            dt_symsrv_function_t func(std::string(Name), Size, paramTypes.VaArgs,
                                      std::move(paramTypes.ParamTypes));

            ctx.RvaToFunction.emplace_hint(f, Rva, std::move(func));

        } else if (AllowMultiple) {
            f->second.AltNames.emplace_front(Name);
        }
    } catch (std::bad_alloc &) {
    }

    return;
}

static BOOL CALLBACK dt_symsrv_EnumSymProc(PSYMBOL_INFO SymInfo,
    ULONG SymbolSize, PVOID UserContext)
{
    if (dt_symsrv_is_function(SymInfo)) {
        dt_symsrv_add_function(*(dt_symsrv_function_enum_ctx_t*)UserContext,
                               (ULONG)(SymInfo->Address - SymInfo->ModBase),
                               SymInfo->Size,
                               SymInfo->Name,
                               SymInfo->TypeIndex,
                               SymInfo->ModBase,
                               TRUE);
    }

    return TRUE;
}

static std::string dt_symsrv_module_name(ULONGLONG ModuleBase)
{
    char DriverPath[MAXPATHLEN];
    if (!GetDeviceDriverFileNameA((PVOID)(ULONG_PTR)ModuleBase, DriverPath, sizeof(DriverPath))) {
        return NULL;
    }

    if (DriverPath[0] == '\\' && DriverPath[1] == '?' &&
        DriverPath[2] == '?' && DriverPath[3] == '\\') {

        DriverPath[1] = '\\';

    } else {
        const char SystemRootPrefix[] = "\\SystemRoot\\";

        if (!_strnicmp(DriverPath, SystemRootPrefix, sizeof(SystemRootPrefix) - 1)) {
            CHAR Buf[MAXPATHLEN];
            int Len = GetEnvironmentVariableA("SYSTEMROOT", Buf, sizeof(Buf));

            if ((Len > 0) &&
                ((sizeof(DriverPath) - Len) >
                 ((strlen(DriverPath) + 1) - (sizeof(SystemRootPrefix) - 3)))) {

                strcat(Buf, DriverPath + (sizeof(SystemRootPrefix) - 2));
                strcpy(DriverPath, Buf);
            }
        }
    }

    return DriverPath;
}

#if !defined(_X86_)

static PVOID dt_symsrv_map(PCSTR FileName)
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    HANDLE SectionHandle = NULL;
    PVOID View = NULL;
    BOOL RedirectionDisabled;
    PVOID OldRedirectionDisabled;

    RedirectionDisabled = Wow64DisableWow64FsRedirection(&OldRedirectionDisabled);
    FileHandle = CreateFileA(FileName,
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

    if (RedirectionDisabled) {
        Wow64RevertWow64FsRedirection(OldRedirectionDisabled);
    }

    if (INVALID_HANDLE_VALUE == FileHandle) {
        goto exit;
    }

    SectionHandle = CreateFileMappingW(FileHandle,
                                       NULL,
                                       PAGE_READONLY,
                                       0, 0, // Max size is zero = map entire file.
                                       NULL);

    if (NULL == SectionHandle) {
        goto exit;
    }

    View = MapViewOfFile(SectionHandle,
                         FILE_MAP_READ,
                         0, 0, // At zero offset.
                         0);   // Map entire section.

exit:
    if (NULL != SectionHandle) {
        CloseHandle(SectionHandle);
    }

    if (INVALID_HANDLE_VALUE != FileHandle) {
        CloseHandle(FileHandle);
    }

    return View;
}

#endif

static void dt_symsrv_load_code_blocks(dt_symsrv_function_enum_ctx_t& ctx,
                                       IMAGEHLP_MODULE64* ModuleInfo)
{
#if !defined(_X86_)

    //
    // This is a fallback path to add code blocks from image exception table
    // as synthetic symbols on architectures that have them.
    //

    PVOID Base = dt_symsrv_map(ModuleInfo->LoadedImageName);
    if (NULL == Base) {
        return;
    }

    ULONG FunctionTableSize;
    PRUNTIME_FUNCTION FunctionTable = (PRUNTIME_FUNCTION)
        ImageDirectoryEntryToData(Base, FALSE, IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                                  &FunctionTableSize);

    if (NULL != FunctionTable) {
        FunctionTableSize /= sizeof(RUNTIME_FUNCTION);
        while (FunctionTableSize-- > 0) {
            char Name[12]; // "+0x12345678" + 1
            int Chars = sprintf(Name, "+0x%08lx", FunctionTable->BeginAddress);
            if (Chars > 0) {
                dt_symsrv_add_function(ctx, FunctionTable->BeginAddress, 0,
                                       Name, 0, ModuleInfo->BaseOfImage, FALSE);
            }

            FunctionTable += 1;
        }
    }

    UnmapViewOfFile(Base);
    return;

#endif !defined(_X86_)
}

static rva_to_func_map_t dt_symsrv_load_functions(ULONG_PTR ModuleBase,
                                                  const MODLOAD_PDBGUID_PDBAGE *Age,
                                                  PCSTR SymNameFilter,
                                                  bool isNtosKrnl)
{
    MODLOAD_DATA Modload = {0};
    PMODLOAD_DATA ModuleData = NULL;
    IMAGEHLP_MODULE64 ModuleInfo = {0};
    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);
    const auto Process = GetCurrentProcess();
    BOOL AlreadyLoaded = SymGetModuleInfo64(Process, ModuleBase, &ModuleInfo);
    ULONGLONG Base;

    if (Age) {
        Modload.ssize = sizeof(MODLOAD_DATA);
        Modload.ssig = DBHHEADER_PDBGUID;
        Modload.data = const_cast<MODLOAD_PDBGUID_PDBAGE*>(Age);
        Modload.size = sizeof(MODLOAD_PDBGUID_PDBAGE);
        ModuleData = &Modload;
    }

    if (AlreadyLoaded) {
        Base = ModuleBase;

    } else {
        auto ImageFileName = dt_symsrv_module_name(ModuleBase);

        PVOID OldRedirectionDisabled;
        BOOL RedirectionDisabled =
            Wow64DisableWow64FsRedirection(&OldRedirectionDisabled);

        Base = SymLoadModuleEx(Process, NULL, ImageFileName.c_str(), NULL,
                               ModuleBase, 0, ModuleData, 0);

        if (0 == Base) {
            dt_dprintf("Failed symbol server failed to load image at %p, %08lx\n",
                       ModuleBase, GetLastError());
        }

        if (RedirectionDisabled) {
            Wow64RevertWow64FsRedirection(OldRedirectionDisabled);
        }

        if (0 == Base) {
            return {};
        }

        SymGetModuleInfo64(Process, Base, &ModuleInfo);
    }

    if ((NULL != ModuleData) && (DBHHEADER_PDBGUID == ModuleData->ssig)) {
        if (0 != memcmp(&ModuleInfo.PdbSig70,
                        ModuleData->data,
                        sizeof(MODLOAD_PDBGUID_PDBAGE))) {

            dt_dprintf("Failed symbol server failed to load image at %p - PDB info unmatched.\n",
                       ModuleBase);

            if (!AlreadyLoaded) {
                SymUnloadModule64(Process, Base);
            }

            return {};
        }
    }

    PCSTR ModuleName = ModuleInfo.ModuleName;
    if (isNtosKrnl ||
        (0 == _stricmp(ModuleInfo.ModuleName, "ntoskrnl")) ||
        (0 == _stricmp(ModuleInfo.ModuleName, "ntkrnlmp"))) {

        ModuleName = "nt";
    }

    if (!SymNameFilter || !*SymNameFilter) {
        SymNameFilter = "*";
    }

    dt_symsrv_function_enum_ctx_t ctx{{}, ModuleName, Process};
    if (!SymEnumSymbols(Process, Base, SymNameFilter, dt_symsrv_EnumSymProc,
                        &ctx)) {
        dt_dprintf("Failed symbol server failed to enum symbols for load image at %p, %08lx\n",
                   ModuleBase, GetLastError());
    }

    dt_symsrv_load_code_blocks(ctx, &ModuleInfo);

    if (!AlreadyLoaded) {
        SymUnloadModule64(Process, Base);
    }

    return std::move(ctx.RvaToFunction);
}

static DWORD CALLBACK dt_symsrv_thread(PVOID param)
{
    SetThreadDescription(GetCurrentThread(), L"dtrace_symsrv");

    dt_symsrv* s = (dt_symsrv*)param;
    UCHAR buf[4096] = {0};
    ULONG bytesdone;
    ULONGLONG LoadedImageBase = 0;
    rva_to_func_map_t FuncMap;
    rva_to_func_map_t::iterator CurFunc;
    std::string UsedNameFilter;

    //
    // Start the API loop.
    // It will be broken when stopping the symbol server by
    // canceling an outstanding IO.
    //

    while (!s->Exiting &&
           DeviceIoControl(s->Device,
                           TRACE_SYM_QUEUE_PACKET,
                           buf, sizeof(buf),
                           buf, sizeof(buf),
                           &bytesdone,
                           NULL)) {

        PTRACE_SYM_REQUEST Request = (PTRACE_SYM_REQUEST)&buf[0];
        PTRACE_SYM_REPLY Reply = (PTRACE_SYM_REPLY)&buf[0];
        ULONG Index = Request->Index;

        if ((ULONG)-1 == Index) {
            continue;
        }

        // Set the reply index to -1, indicating no result in case a failure occurs below.
        static_assert(offsetof(TRACE_SYM_REQUEST, Index) == offsetof(TRACE_SYM_REPLY, Index));

        Reply->Index = -1;

        // Capture elements of the request buffer up front since it is shared with the reply buffer.
        PCSTR nameFilter = (PCSTR)(Request + 1);
        MODLOAD_PDBGUID_PDBAGE age{};
        const decltype(age) *pAge = nullptr;
        if (Request->Flags.DbgInfoPresent) {
            age = *(MODLOAD_PDBGUID_PDBAGE*)nameFilter;
            pAge = &age;
            nameFilter += sizeof(MODLOAD_PDBGUID_PDBAGE);
        }

        bool isKernel = Request->Flags.IsNtosKrnl;
        auto moduleBase = Request->ModuleBase;
        Request = nullptr;
        try {

            // Reference the name filter.
            // This code will return all entries if glob or no filter.
            if ((0 == *nameFilter) || strisglob(nameFilter)) {
                nameFilter = nullptr;
            }

            bool filterChanged = false;
            if (!nameFilter) {
                filterChanged = !UsedNameFilter.empty();
                UsedNameFilter.clear();
            } else {
                if (UsedNameFilter != nameFilter) {
                    filterChanged = true;
                    UsedNameFilter = nameFilter;
                }
                nameFilter = nullptr;
            }

            // Remap the module if different from one already mapped,
            // and load requested functions from it.
            if (LoadedImageBase != moduleBase || filterChanged || FuncMap.empty()) {
                FuncMap.clear();
                FuncMap = dt_symsrv_load_functions(moduleBase, pAge, UsedNameFilter.c_str(), isKernel);
                LoadedImageBase = moduleBase;
                Index = 0;
            }

        } catch(const std::exception &e) {
            dt_dprintf("dt_symsrv error: %s", e.what());
            FuncMap.clear();
            Index = 0;
            continue;
        }

        // Check if resetting the walker.
        if (0 == Index) {
            CurFunc = FuncMap.begin();
        }

        for (; CurFunc != FuncMap.end(); ++CurFunc) {
            // Check if any of the names match the filter.
            bool Matched = UsedNameFilter.empty();
            if (!Matched) {
                Matched = CurFunc->second.ForAllNames(
                    [&UsedNameFilter](const auto &name) noexcept {
                    return UsedNameFilter == name;
                    });

                if (!Matched) {
                    continue;
                }
            }

            PSTR names = (PSTR)(Reply + 1);
            PSTR limit = ((PSTR)&buf[0] + (sizeof(buf) - 1));
            bool overflow = false;

            auto addToNames = [&names, limit, &overflow](const std::string& str) {
                auto len = str.size() + 1;
                if (names + len < limit) {
                    memcpy(names, str.c_str(), len);
                    names += len;
                } else {
                    overflow = true;
                }
            };

            auto terminateMultiString = [&names, limit, &overflow]() {
                if (names <= limit) {
                    *names = '\0';
                    ++names;
                } else {
                    overflow = true;
                }
            };

            CurFunc->second.ForAllNames(
                [&addToNames](const auto& name) noexcept {
                    addToNames(name);
                    return false;
                });

            terminateMultiString();

            // Fill in parameter types multistring.
            for (const auto &param : CurFunc->second.ParamTypes) {
                addToNames(param);
            }

            terminateMultiString();

            // If this entry overflows the reply packet, just skip it.
            // This should only really happen for code that represents many
            // folded functions (with tons of alternate names). Such code is
            // probably something simple that's not worth instrumenting.
            if (overflow) {
                continue;
            }

            // Write location information for those matching the requested
            // filter.
            Reply->Rva = CurFunc->first;
            Reply->Size = CurFunc->second.Size;
            Reply->Flags.VaArgs = CurFunc->second.VaArgs;

            // We return single entry here. Advance the iterator so that the
            // next time we come back we will start on the next function.
            Reply->Index = Index + 1;
            Reply->NextEntryOffset = 0;
            ++CurFunc;
            break;
        }
    }

    return 0;
}

dt_symsrv* dt_symsrv_start(void)
{
    DWORD Tid;
    dt_symsrv* s;

    s = (dt_symsrv*)malloc(sizeof(dt_symsrv));
    if (NULL == s) {
        goto error;
    }

    ZeroMemory(s, sizeof(dt_symsrv));

    s->Device = CreateFileW(L"\\\\.\\dtrace\\symsrv",
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (INVALID_HANDLE_VALUE == s->Device) {
        dt_dprintf("symbol server failed to open control device, %08lx\n",
                   GetLastError());
        s->Device = NULL;
        goto error;
    }

    s->Thread = CreateThread(NULL, 0, dt_symsrv_thread, s, 0, &Tid);
    if (NULL == s->Thread) {
        goto error;
    }

    return s;

error:
    if (NULL != s) {
        dt_symsrv_stop(s);
    }

    return NULL;
}

void dt_symsrv_stop(dt_symsrv* srv)
{
    if (NULL != srv->Thread) {
        srv->Exiting = TRUE;
        while (WAIT_TIMEOUT ==
               WaitForSingleObject(srv->Thread,
                                   (CancelIoEx(srv->Device, NULL)
                                    ? INFINITE
                                    : 100))) {
            ;
        }
        CloseHandle(srv->Thread);
    }

    if (NULL != srv->Device) {
        CloseHandle(srv->Device);
    }

    free(srv);
    return;
}

