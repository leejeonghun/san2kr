// Copyright 2016 jeonghun

#ifndef SAN2KR_DDRAW_IAT_HOOKER_H_
#define SAN2KR_DDRAW_IAT_HOOKER_H_

#include <windows.h>
#include <cstdint>
#include <type_traits>

class iat_hooker final {
 public:
  iat_hooker() = default;
  ~iat_hooker() = default;

  bool hook(HINSTANCE base_addr, const char *module_name,
    const char *func_name, const void *hook_ptr);
  bool unhook();

  template <typename T, typename ...ARGS>
  inline std::result_of_t<T(ARGS...)> call_origin(ARGS... args) {
    return reinterpret_cast<T>(org_func_ptr_)(args...);
  }

 private:
  inline const IMAGE_NT_HEADERS* get_nt_headers() const;
  inline const IMAGE_IMPORT_DESCRIPTOR* get_import_descriptor(
    const IMAGE_NT_HEADERS *nt_hdr_ptr, const char *module_name) const;

  inline const IMAGE_THUNK_DATA* find_thunk(
    const IMAGE_IMPORT_DESCRIPTOR *import_desc_ptr,
    const char *func_name) const;

  inline const void* set_func_ptr_to_thunk(
    const IMAGE_THUNK_DATA* thunk_ptr, const void *hook_ptr) const;

  inline bool check_null(const IMAGE_IMPORT_DESCRIPTOR *imp_desc_ptr) const {
    return imp_desc_ptr != nullptr && imp_desc_ptr->Characteristics == 0;
  }

  inline bool check_null(const IMAGE_THUNK_DATA *thunk_ptr) const {
    return thunk_ptr != nullptr && thunk_ptr->u1.AddressOfData == 0;
  }

  uintptr_t base_addr_ = 0;
  const void *org_func_ptr_ = nullptr;
  const IMAGE_THUNK_DATA *org_thunk_ptr_ = nullptr;
};

#endif  // SAN2KR_DDRAW_IAT_HOOKER_H_
