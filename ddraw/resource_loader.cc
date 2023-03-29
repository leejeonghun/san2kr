// Copyright 2014 jeonghun
#include "resource_loader.h"
#include <cassert>

resource_loader::resource_loader(uint32_t rsc_id, HINSTANCE inst_handle)
    : mem_ptr_(nullptr), size_(0) {
  HMODULE module_handle = inst_handle ? inst_handle : GetModuleHandle(nullptr);
  HRSRC res_handle =
      FindResource(module_handle, MAKEINTRESOURCE(rsc_id), RT_RCDATA);
  if (res_handle != NULL) {
    HGLOBAL global_handle = LoadResource(module_handle, res_handle);
    assert(global_handle);
    size_ = SizeofResource(module_handle, res_handle);
    mem_ptr_ = reinterpret_cast<LPBYTE>(LockResource(global_handle));
  }
  assert(mem_ptr_ != nullptr);
  assert(size_ > 0);
}
