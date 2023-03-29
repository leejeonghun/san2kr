// Copyright 2014 jeonghun
#ifndef SAN2KR_DDRAW_RESOURCE_FILE_H_
#define SAN2KR_DDRAW_RESOURCE_FILE_H_

#include <windows.h>
#include <cstdint>

class resource_loader final {
 public:
  explicit resource_loader(uint32_t rsc_id, HINSTANCE inst_handle = NULL);
  ~resource_loader() = default;

  inline const LPBYTE get_ptr() const { return mem_ptr_; }
  inline const uint32_t get_length() const { return size_; }

 private:
  LPBYTE mem_ptr_;
  uint32_t size_;
};

#endif  // SAN2KR_DDRAW_RESOURCE_FILE_H_
