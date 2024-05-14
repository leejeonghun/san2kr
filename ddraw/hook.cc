// Copyright 2016 jeonghun

#include "hook.h"
#include <string>
#include <unordered_map>
#include "codepage.h"
#include "iat_hooker.h"
#include "memory_patch.h"

namespace hook {

HFONT kr_font = NULL;
HMENU kr_menu = NULL;
HMODULE dll_handle = NULL;

iat_hooker createfonta;
HFONT WINAPI hook_createfonta(int cHeight, int cWidth, int cEscapement,
    int cOrientation, int cWeight, DWORD bItalic, DWORD bUnderline,
    DWORD bStrikeOut, DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision,
    DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName) {
  if (kr_font == NULL) {
    kr_font = createfonta.call_origin<decltype(&CreateFontA)>(cHeight, cWidth,
        cEscapement, cOrientation, cWeight, bItalic, bUnderline, bStrikeOut,
        HANGUL_CHARSET, iOutPrecision, iClipPrecision, iQuality,
        iPitchAndFamily, "바탕체");
  }
  return createfonta.call_origin<decltype(&CreateFontA)>(cHeight, cWidth,
      cEscapement, cOrientation, cWeight, bItalic, bUnderline, bStrikeOut,
      iCharSet, iOutPrecision, iClipPrecision, iQuality, iPitchAndFamily,
      pszFaceName);
}

iat_hooker textouta;
BOOL WINAPI hook_textouta(HDC hdc, int x, int y, LPCSTR lpString, int c) {
  constexpr int kr_bgn_row = 16;
  constexpr int kr_end_row = 41;
  constexpr int last_row = 96;

  static int font_raw_cnt = 0;

  BOOL result = FALSE;
  if (kr_bgn_row <= font_raw_cnt && font_raw_cnt <= kr_end_row) {
    auto org_font = SelectObject(hdc, kr_font);

    int row = font_raw_cnt - kr_bgn_row;
    unsigned char kor_str[(0xFE - 0xA1 + 1) * 2 + 2 + 1] = { 0xA1, 0xA1, 0 };
    for (int i = 2, j = 0; i < sizeof(kor_str) - 1; i += 2, j++) {
      kor_str[i] = 0xB0 + (uint8_t)row;
      kor_str[i + 1] = 0xA1 + (uint8_t)j;
    }

    auto fonts_row = reinterpret_cast<char*>(kor_str);
    result = textouta.call_origin<decltype(&TextOutA)>(
        hdc, x, y, fonts_row, strlen(fonts_row));
    SelectObject(hdc, org_font);
  } else
    result = textouta.call_origin<decltype(&TextOutA)>(hdc, x, y, lpString, c);

  font_raw_cnt++;

  if (font_raw_cnt >= last_row) {
    DeleteObject(kr_font);
    createfonta.unhook();
    textouta.unhook();
  }

  return result;
}

iat_hooker createwindowexa;
HWND WINAPI hook_createwindowexa(DWORD dwExStyle, LPCSTR lpClassName,
    LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
  HWND hwnd = createwindowexa.call_origin<decltype(&CreateWindowExA)>(dwExStyle,
      lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent,
      kr_menu, hInstance, lpParam);
  SetWindowTextW(hwnd, codepage<932>(lpWindowName));
  memory_patch::patch_to_code(dll_handle);

  return hwnd;
}

iat_hooker createfilea;
HANDLE WINAPI hook_createfilea(LPCSTR lpFileName, DWORD dwDesiredAccess,
    DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile) {
  return CreateFileW(codepage<932>(lpFileName), dwDesiredAccess, dwShareMode,
      lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
      hTemplateFile);
}

const std::unordered_map<std::wstring, const wchar_t*> menu_translate_map = {
  { L"Aディスク", L"A디스크" },
  { L"ドライブ&1", L"드라이브&1" },
  { L"Bディスク", L"B디스크" },
  { L"Dディスク", L"D디스크" },
  { L"ドライブ&2", L"드라이브&2" }
};

iat_hooker insertmenua;
BOOL WINAPI hook_insertmenua(HMENU hMenu, UINT uPosition, UINT uFlags,
    UINT_PTR uIDNewItem, LPCSTR lpNewItem) {
  codepage<932> org(lpNewItem);
  auto iter = menu_translate_map.find(static_cast<const wchar_t*>(org));
  const bool need_translation = iter != menu_translate_map.end();
  return InsertMenuW(hMenu, uPosition, uFlags, uIDNewItem,
      need_translation ? iter->second : org);
}

iat_hooker appendmenua;
BOOL WINAPI hook_appendmenua(
    HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR lpNewItem) {
  codepage<932> org(lpNewItem);
  auto iter = menu_translate_map.find(static_cast<const wchar_t*>(org));
  const bool need_translation = iter != menu_translate_map.end();
  return AppendMenuW(
      hMenu, uFlags, uIDNewItem, need_translation ? iter->second : org);
}

iat_hooker createdialogparama;
HWND WINAPI hook_createdialogparama(HINSTANCE hInstance, LPCSTR lpTemplateName,
    HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
  int rsc_id = reinterpret_cast<INT>(lpTemplateName);
  return createdialogparama.call_origin<decltype(&CreateDialogParamA)>(
      rsc_id == 103 ? dll_handle : hInstance, lpTemplateName, hWndParent,
      lpDialogFunc, dwInitParam);
}

const std::unordered_map<uintptr_t, const wchar_t*> msg_translate_map = {
  { 0x0043DBBC, L"게임을 종료하시겠습니까?" },
  { 0x0043DBE4, L"확인" }
};

inline const wchar_t* translate_msg(const char* msg) {
  auto iter = msg_translate_map.find(reinterpret_cast<uintptr_t>(msg));
  return iter != msg_translate_map.end() ? iter->second : nullptr;
}

iat_hooker messageboxa;
int WINAPI hook_messageboxa(
  HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
  auto text = translate_msg(lpText);
  auto caption = translate_msg(lpCaption);
  return MessageBoxW(hWnd, text ? text : codepage<932>(lpText),
    caption ? caption : codepage<932>(lpCaption), uType);
}

iat_hooker showwindow;
BOOL WINAPI hook_ShowWindow(HWND hWnd, int nCmdShow) {
  memory_patch::patch_to_fdi();
  return showwindow.call_origin<decltype(&ShowWindow)>(hWnd, nCmdShow);
}


bool install(HMODULE hdll) {
  dll_handle = hdll;
  kr_menu = LoadMenu(hdll, MAKEINTRESOURCE(102));
  HINSTANCE hinst = GetModuleHandle(nullptr);
  return
    createwindowexa.hook(hinst, "user32.dll", "CreateWindowExA", hook_createwindowexa) &&
    createfilea.hook(hinst, "kernel32.dll", "CreateFileA", hook_createfilea) &&
    textouta.hook(hinst, "gdi32.dll", "TextOutA", hook_textouta) &&
    createfonta.hook(hinst, "gdi32.dll", "CreateFontA", hook_createfonta) &&
    insertmenua.hook(hinst, "user32.dll", "InsertMenuA", hook_insertmenua) &&
    appendmenua.hook(hinst, "user32.dll", "AppendMenuA", hook_appendmenua) &&
    createdialogparama.hook(hinst, "user32.dll", "CreateDialogParamA", hook_createdialogparama) &&
    messageboxa.hook(hinst, "user32.dll", "MessageBoxA", hook_messageboxa) &&
    showwindow.hook(hinst, "user32.dll", "ShowWindow", hook_ShowWindow);
}

bool uninstall() {
  return createwindowexa.unhook() &&
    createfilea.unhook() &&
    textouta.unhook() &&
    createfonta.unhook() &&
    insertmenua.unhook() &&
    appendmenua.unhook() &&
    createdialogparama.unhook() &&
    messageboxa.unhook() &&
    showwindow.unhook();
}

}  // namespace hook
