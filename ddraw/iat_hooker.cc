// Copyright 2016 jeonghun

#include "iat_hooker.h"

bool iat_hooker::hook(HINSTANCE base_addr, const char *module_name,
    const char *func_name, const void *hook_ptr) {
  if (org_func_ptr_ == nullptr && module_name != nullptr &&
      func_name != nullptr && hook_ptr != nullptr) {
    base_addr_ = reinterpret_cast<uintptr_t>(base_addr);
    const IMAGE_IMPORT_DESCRIPTOR *imp_desc_ptr = get_import_descriptor(
      get_nt_headers(), module_name);

    if (imp_desc_ptr != nullptr) {
      org_thunk_ptr_ = find_thunk(imp_desc_ptr, func_name);
      if (org_thunk_ptr_ != nullptr) {
        org_func_ptr_ = set_func_ptr_to_thunk(org_thunk_ptr_, hook_ptr);
      }
    }
  }

  return org_func_ptr_ != nullptr;
}

bool iat_hooker::unhook() {
  if (org_thunk_ptr_ != nullptr && org_func_ptr_ != nullptr) {
    if (set_func_ptr_to_thunk(org_thunk_ptr_, org_func_ptr_) != nullptr) {
      org_thunk_ptr_ = nullptr;
      org_func_ptr_ = nullptr;
      base_addr_ = 0;
    }
  }

  return org_func_ptr_ == nullptr;
}

const IMAGE_NT_HEADERS* iat_hooker::get_nt_headers() const {
  const IMAGE_NT_HEADERS *nt_hdr_ptr = nullptr;

  const IMAGE_DOS_HEADER *dos_hdr_ptr =
    reinterpret_cast<IMAGE_DOS_HEADER*>(base_addr_);
  if (dos_hdr_ptr != nullptr && dos_hdr_ptr->e_magic == IMAGE_DOS_SIGNATURE) {
    const IMAGE_NT_HEADERS* target_ptr =
      reinterpret_cast<IMAGE_NT_HEADERS*>(base_addr_ + dos_hdr_ptr->e_lfanew);
    if (target_ptr != nullptr && target_ptr->Signature == IMAGE_NT_SIGNATURE) {
      nt_hdr_ptr = target_ptr;
    }
  }

  return nt_hdr_ptr;
}

const IMAGE_IMPORT_DESCRIPTOR* iat_hooker::get_import_descriptor(
    const IMAGE_NT_HEADERS *nt_hdr_ptr, const char *module_name) const {
  const IMAGE_IMPORT_DESCRIPTOR* find_imp_desc_ptr = nullptr;

  if (nt_hdr_ptr != nullptr && module_name != nullptr) {
    const uintptr_t offset = nt_hdr_ptr->OptionalHeader.DataDirectory
      [IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    IMAGE_IMPORT_DESCRIPTOR* imp_desc_ptr =
      reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base_addr_ + offset);

    for (uint32_t i = 0; check_null(imp_desc_ptr + i) == false; i++) {
      LPCSTR imp_mod_name = reinterpret_cast<LPCSTR>(
        base_addr_ + (imp_desc_ptr + i)->Name);
      if (imp_mod_name != nullptr && _stricmp(imp_mod_name, module_name) == 0) {
        find_imp_desc_ptr = imp_desc_ptr + i;
        break;
      }
    }
  }

  return find_imp_desc_ptr;
}

const IMAGE_THUNK_DATA* iat_hooker::find_thunk(
    const IMAGE_IMPORT_DESCRIPTOR *import_desc_ptr,
    const char *func_name) const {
  const IMAGE_THUNK_DATA *thunk_ptr = nullptr;

  if (import_desc_ptr != nullptr && func_name != nullptr) {
    const IMAGE_THUNK_DATA *org_thunk_ptr =
      reinterpret_cast<IMAGE_THUNK_DATA*>(
        base_addr_ + import_desc_ptr->OriginalFirstThunk);
    for (uint32_t i = 0; check_null(org_thunk_ptr + i) == false; i++) {
      if (IMAGE_SNAP_BY_ORDINAL(org_thunk_ptr[i].u1.Ordinal) == FALSE) {
        IMAGE_IMPORT_BY_NAME *import_ptr =
          reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
            base_addr_ + org_thunk_ptr[i].u1.AddressOfData);
        if (import_ptr != nullptr && strcmp(
            reinterpret_cast<const char*>(import_ptr->Name), func_name) == 0) {
          thunk_ptr = reinterpret_cast<IMAGE_THUNK_DATA*>(
            base_addr_ + import_desc_ptr->FirstThunk) + i;
          break;
        }
      }
    }
  }

  return thunk_ptr;
}

const void* iat_hooker::set_func_ptr_to_thunk(
    const IMAGE_THUNK_DATA *thunk_ptr, const void *func_ptr) const {
  void* org_func_ptr = nullptr;

  if (thunk_ptr != nullptr) {
    MEMORY_BASIC_INFORMATION mbi = { 0 };
    if (VirtualQuery(thunk_ptr, &mbi, sizeof(mbi)) > 0) {
      if (VirtualProtect(mbi.BaseAddress, mbi.RegionSize,
        PAGE_READWRITE, &mbi.Protect) != FALSE) {
        org_func_ptr = reinterpret_cast<void*>(thunk_ptr->u1.Function);
        InterlockedExchangePointer(reinterpret_cast<PVOID*>(
          &const_cast<IMAGE_THUNK_DATA*>(thunk_ptr)->u1.Function),
          const_cast<void*>(func_ptr));
        VirtualProtect(mbi.BaseAddress, mbi.RegionSize,
          mbi.Protect, &mbi.Protect);
      }
    }
  }

  return org_func_ptr;
}
