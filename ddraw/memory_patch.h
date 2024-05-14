// Copyright 2023 jeonghun

#ifndef SAN2KR_DDRAW_MEMORY_PATCH_H_
#define SAN2KR_DDRAW_MEMORY_PATCH_H_

namespace memory_patch {

bool patch_to_fdi();
bool patch_to_code(HMODULE hdll);

}  // namespace memory_patch

#endif  // SAN2KR_DDRAW_MEMORY_PATCH_H_
