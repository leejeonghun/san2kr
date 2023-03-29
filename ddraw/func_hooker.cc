#include "func_hooker.h"

#pragma pack(push, 1)
struct template_64bit_opcode {
  uint8_t opcode[2];
  uint8_t reserve[4];
  uint64_t target_addr;
};

struct template_32bit_opcode {
  uint8_t opcode;
  uint32_t target_offset;
};
#pragma pack(pop)

bool func_hooker::install(FARPROC func_ptr, FARPROC hook_ptr) {
  bool ready_to_hook = false;

  if (func_ptr != nullptr && hook_ptr != nullptr && func_ptr != hook_ptr) {
    func_ptr_ = func_ptr;
    hook_ptr_ = hook_ptr;

    const addr_t offset = calc_offset(func_ptr_, hook_ptr_);
    const bool use_64bit_jump =
        sizeof(addr_t) > sizeof(int32_t) && need_64bit_jump(offset);
    hookcode_length_ =
        use_64bit_jump ? CODE_LENGTH_FOR_64BIT : CODE_LENGTH_FOR_32BIT;

    DWORD old_protect = 0;
    BOOL set_protect = VirtualProtect(
        func_ptr_, hookcode_length_, PAGE_EXECUTE_READWRITE, &old_protect);

    if (set_protect != false) {
      memcpy(original_code_, func_ptr_, hookcode_length_);

      if (use_64bit_jump != false) {
        template_64bit_opcode op_jmp = { { 0xFF, 0x25 }, { 0 },
          conv_to_addr(hook_ptr_) };
        *reinterpret_cast<template_64bit_opcode*>(trampoline_code_) = op_jmp;
      } else {
        template_32bit_opcode op_jmp = { 0xE9,
          static_cast<uint32_t>(offset - CODE_LENGTH_FOR_32BIT) };
        *reinterpret_cast<template_32bit_opcode*>(trampoline_code_) = op_jmp;
      }

      hook();
      ready_to_hook = true;
    }
  }

  return ready_to_hook;
}

inline func_hooker::addr_t func_hooker::calc_offset(
    FARPROC src_ptr, FARPROC dst_ptr) const {
  const addr_t src_addr = conv_to_addr(src_ptr);
  const addr_t dst_addr = conv_to_addr(dst_ptr);
  return dst_addr - src_addr;
}
