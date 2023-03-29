// Copyright 2016 jeonghun

#include <windows.h>
#include <ddraw.h>
#include "dll_loader.h"
#include "hook.h"

static dll_loader ddraw(L"ddraw", true);
HRESULT WINAPI DirectDrawCreate(GUID *lpGUID, LPDIRECTDRAW *lplpDD, IUnknown *pUnkOuter) {
  return ddraw.call<decltype(&DirectDrawCreate)>("DirectDrawCreate", lpGUID, lplpDD, pUnkOuter);
}

int APIENTRY DllMain(HMODULE hDLL, DWORD Reason, LPVOID Reserved) {
  switch (Reason) {
  case DLL_PROCESS_ATTACH:
    DisableThreadLibraryCalls(hDLL);
    hook::install(hDLL);
    break;

  case DLL_PROCESS_DETACH:
    hook::uninstall();
    break;
  }

  return TRUE;
}
