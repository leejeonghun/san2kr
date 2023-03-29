// Copyright 2016 jeonghun

#ifndef SAN2KR_DDRAW_HOOK_H_
#define SAN2KR_DDRAW_HOOK_H_

namespace hook {

bool install(HMODULE hdll);
bool uninstall();

}  // namespace hook

#endif  // SAN2KR_DDRAW_HOOK_H_
