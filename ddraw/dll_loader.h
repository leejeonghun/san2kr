// Copyright 2015 jeonghun
#ifndef SAN2KR_DDRAW_DLL_LOADER_H_
#define SAN2KR_DDRAW_DLL_LOADER_H_

#include <windows.h>
#include <shlwapi.h>
#include <cassert>
#include <type_traits>

#pragma comment(lib, "shlwapi")

class dll_loader final {
 public:
  explicit dll_loader(const wchar_t *dll_name, bool load_from_system = false) {
    module_ = load_from_system ? NULL : GetModuleHandle(dll_name);
    if (module_ == NULL) {
			if (load_from_system) {
				wchar_t sys_path[MAX_PATH] = { 0 };
				GetSystemDirectory(sys_path, _countof(sys_path));
				PathCombine(sys_path, sys_path, dll_name);
				module_ = LoadLibrary(sys_path);
			} else {
				module_ = LoadLibrary(dll_name);
			}
      use_loadlib = module_ != NULL;
    }
    assert(module_ != NULL);
  }

  ~dll_loader() {
    if (use_loadlib) {
      FreeLibrary(module_);
    }
  }

  dll_loader(const dll_loader&) = delete;
  void operator=(const dll_loader&) = delete;

  template <typename T = FARPROC>
  inline T get_func_ptr(const char* api_name) const {
    assert(api_name != nullptr);
    return reinterpret_cast<T>(GetProcAddress(module_, api_name));
  }

  template <typename T, typename... ARGS>
  inline std::result_of_t<T(ARGS...)> call(const char *api_name, ARGS ...args) const {
    auto func_ptr = get_func_ptr<T>(api_name);
    assert(func_ptr != nullptr);
    return func_ptr(args...);
  }

 private:
  HMODULE module_ = NULL;
  bool use_loadlib = false;
};

#endif  // SAN2KR_DDRAW_DLL_LOADER_H_
