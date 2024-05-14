// Copyright 2016 jeonghun

#include "code_patcher.h"

bool code_patcher::apply(void *addr_ptr, const void *code_ptr, uint32_t length) {
  bool apply = false;

  if (addr_ptr != nullptr && code_ptr != nullptr && length > 0) {
    DWORD protect_flag = 0;
    if (VirtualProtect(addr_ptr, length, PAGE_EXECUTE_READWRITE, &protect_flag) != FALSE) {
      protect_flag_ = protect_flag;
      addr_ptr_ = addr_ptr;
      backup_.resize(length);
      memcpy(backup_.data(), addr_ptr_, length);
      memcpy(addr_ptr, code_ptr, length);
      apply = true;
    }
  }

  return apply;
}

bool code_patcher::undo() {
  bool undo = false;

  if (protect_flag_ != 0) {
    memcpy(addr_ptr_, backup_.data(), backup_.size());
    DWORD protect_flag = 0;
    if (VirtualProtect(addr_ptr_, backup_.size(), protect_flag_, &protect_flag) != FALSE) {
      backup_.clear();
      addr_ptr_ = nullptr;
      protect_flag_ = 0;
      undo = true;
    }
  }

  return undo;
}
