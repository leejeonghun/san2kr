// Copyright 2016 jeonghun

#ifndef SAN2KR_DDRAW_FUNC_HOOKER_H_
#define SAN2KR_DDRAW_FUNC_HOOKER_H_

#include <windows.h>
#include <cstdint>

#pragma intrinsic(memcpy)

class func_hooker final {
 public:
#ifdef _WIN64
  typedef uint64_t addr_t;
#else
  typedef uint32_t addr_t;
#endif

  enum {
    CODE_LENGTH_FOR_32BIT = 5,
    CODE_LENGTH_FOR_64BIT = 14,
  };

  class rehook_on_exit final {
   public:
    rehook_on_exit(const func_hooker& hooker) : hooker_(hooker) {
      hooker_.unhook();
    }
    ~rehook_on_exit() { hooker_.hook(); }

   private:
    const func_hooker& hooker_;
  };

  func_hooker() = default;
  ~func_hooker() = default;

  bool install(FARPROC func_ptr, FARPROC hook_ptr);
  inline void hook() const {
    memcpy(func_ptr_, trampoline_code_, hookcode_length_);
  }
  inline void unhook() const {
    memcpy(func_ptr_, original_code_, hookcode_length_);
  }

 private:
  inline addr_t calc_offset(FARPROC src_ptr, FARPROC dst_ptr) const;
  inline addr_t conv_to_addr(FARPROC func_ptr) const {
    return reinterpret_cast<addr_t>(func_ptr);
  }
  inline bool need_64bit_jump(addr_t offset) const {
    return offset > 0x7FFF0000;
  }

  FARPROC func_ptr_ = nullptr;
  FARPROC hook_ptr_ = nullptr;
  uint32_t hookcode_length_ = 0;
  uint8_t original_code_[CODE_LENGTH_FOR_64BIT] = { 0 };
  uint8_t trampoline_code_[CODE_LENGTH_FOR_64BIT] = { 0 };
};

#endif  // SAN2KR_DDRAW_FUNC_HOOKER_H_
