// Copyright 2023 jeonghun

#include "memory_patch.h"
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include "codepage.h"
#include "codetable.h"
#include "func_hooker.h"
#include "code_patcher.h"
#include "resource.h"
#include "resource_loader.h"
#include "../third_party/json.hpp"

namespace memory_patch {

uint8_t* const open_exe_ptr = reinterpret_cast<uint8_t*>(0x0054F106);
uint8_t* const intro_bitmap_ptr = reinterpret_cast<uint8_t*>(0x0055E0F2);
uint8_t* const main_exe_ptr = reinterpret_cast<uint8_t*>(0x005579A0);

constexpr uint32_t open_exe_length = 61254;
constexpr uint32_t main_exe_length = 251792;
constexpr uint32_t intro_bitmap_length = 20416;

constexpr uint32_t fdi_chunk_size = 1024;
constexpr uint32_t fdi_chunk_header_size = 16;

const auto hook_func_org_ptr = reinterpret_cast<void (*)()>(0x00401250);
func_hooker hook_func;

HMODULE dll_handle = NULL;
uint8_t* fdi_ptr = nullptr;

enum TRANSLATE_STATUS {
  OPEN_EXE,
  INTRO,
  MAIN_EXE,
  END
};

inline void conv_cp949_to_custom(uint8_t* str, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    if (str[i] > 0x7F) {
      uint16_t ch = MAKEWORD(str[i + 1], str[i]);
      auto iter = code_table.find(ch);
      if (iter != code_table.end()) {
        str[i] = HIBYTE(iter->second);
        str[i + 1] = LOBYTE(iter->second);
      } else {
        uint8_t src[] = { str[i], str[i + 1], 0 };
        auto cp932 = codepage<949>(reinterpret_cast<char*>(src)).to_cp932();
        str[i] = cp932[0];
        str[i + 1] = cp932[1];
      }
      i++;
    }
  }
}

inline void write_str_to_fdi(uint8_t* base_ptr, uint32_t offset, const char* utf8, uint32_t size) {
  codepage<> unicode(utf8);
  auto cp949 = unicode.to_cp949();
  if (size <= 0)
    size = strlen(cp949);

  const uint32_t partial_size = fdi_chunk_size - (offset % fdi_chunk_size);
  const bool chunk_boundary =
      partial_size < size && partial_size > 0 &&
      (offset / fdi_chunk_size) != (offset + size) / fdi_chunk_size;
  const uint32_t total_chunk_header =
      (offset / fdi_chunk_size) * fdi_chunk_header_size;

  auto name = base_ptr + offset + total_chunk_header;

  if (chunk_boundary) {
    uint8_t buffer[8] = { 0 };
    assert(sizeof(buffer) >= size);
    memcpy(buffer, cp949, size);
    conv_cp949_to_custom(buffer, size);

    memcpy(name, buffer, partial_size);
    memcpy(name + partial_size + fdi_chunk_header_size, buffer + partial_size,
        size - partial_size);
  } else {
    memcpy(name, cp949, size);
    conv_cp949_to_custom(name, size);
  }
}

void create_intro_bitmap() {
  const wchar_t* intro_kor =
      L"2���� ��, �߱��� ���� ����, ���� ��������, �߽ŵ��� �縮 ������� ��ġ�� �繰ȭ "
      L"�ϰ�, ������ �߼��� ����� ��ȸ �Ҿ��� ������ ���⸸ �ϴ� �����̾���.\n"
      L"�׷��� �� ��Ÿ�� ���� ������ �尢�̴�. �״� \"Ǫ�� ������ �������� ������ ���� "
      L"������ ����\" ��� ����� ���ɰ�, ��Ը��� �ݶ��� �����״�. �Ŀ� \"Ȳ������ ��\" "
      L"�̶�� ���þ����� ���̴�.\n\n"
      L"�ݶ� ��ü�� �� �����Ǿ�����, �ѿս��� ���� ����, ȣ���� ����ȭ ��, ���� ����"
      L"�� ������ ������Ű�� ����̾���. �� ���� ������ �����ϰ� ������ Ȳ���� �������, "
      L"��ô ������ �ʻ�ð� ������ ����� ������ �Ƿ� ������ �����ߴ�. ������ ������ "
      L"������ ���� ���ĸ� �ҷ��鿴���� �ᱹ �ʻ�ÿ��� �ϻ����, �ʻ�õ� ���� ���Ŀ��� "
      L"�ϻ���ߴ�.\n\n"
      L"������ ����� �Ͼ �� �����, �ѿս��� ������ ���߽�Ű�⿡ ����ߴ�. �� ȥ���� "
      L"ƴź ����, ������ ��Ź�̴�. ��Ź�� ������ ������� �������, ���� ���翡 �� "
      L"������ ���, ������ ������Ű�� ������ ������Ű�� ��, ���� ������ ������� �Ǽ��� "
      L"�ηȴ�.\n\n"
      L"�� ��Ź�� Ⱦ���� ���Ͽ� ������ �Ǻ��� ������ ��Ź�Ҹ��� �ݹ��� ����.\n"
      L"������ ȣ������ �̿� ���Ͽ� �����ߴ�.\n"
      L"������ �߸��� ���� �ӿ� ���߰�..\n";

  constexpr int width = 304;
  constexpr int height = 540;
  constexpr int stride = ((width + 31) & ~31);

  HDC hdc = CreateCompatibleDC(GetDC(GetDesktopWindow()));
  auto hbitmap = CreateBitmap(width, height, 1, 1, NULL);
  DeleteObject(SelectObject(hdc, hbitmap));

  HFONT hfont = CreateFont(16, 8, 0, 0, FW_NORMAL, 0, 0, 0, HANGEUL_CHARSET, 0,
      0, NONANTIALIASED_QUALITY, FIXED_PITCH, L"����ü");
  SelectObject(hdc, hfont);
  SetBkColor(hdc, RGB(0, 0, 0));
  SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));

  RECT rect = { 0, 0, width, height };
  DrawText(hdc, intro_kor, -1, &rect, DT_NOFULLWIDTHCHARBREAK | DT_WORDBREAK);
  
  static char bits[stride * height / 8] = { 0 };
  char bmi_buffer[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2] = { 0 };
  BITMAPINFO* bmi = reinterpret_cast<BITMAPINFO*>(bmi_buffer);
  bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi->bmiHeader.biWidth = width;
  bmi->bmiHeader.biHeight = -height;
  bmi->bmiHeader.biBitCount = 1;
  bmi->bmiHeader.biPlanes = 1;
  memset(&(bmi->bmiColors[0]), 0xFF, sizeof(RGBQUAD));
  memset(&(bmi->bmiColors[1]), 0, sizeof(RGBQUAD));

  GetDIBits(hdc, hbitmap, 0, height, bits, bmi, DIB_RGB_COLORS);

  DeleteDC(hdc);
  DeleteObject(hbitmap);
  DeleteObject(hfont);

  for (int i = 0; i < height; i++)
    memcpy(intro_bitmap_ptr + i * width / 8, bits + i * stride / 8, width / 8);
}

void translate_binary(int rsc_id, uint8_t* addr) {
  resource_loader rsc(rsc_id, dll_handle);
  auto data =
      nlohmann::json::parse(rsc.get_ptr(), rsc.get_ptr() + rsc.get_length());

  for (auto& element : data) {
    auto utf8 = element["kor"].get<std::string>();
    if (utf8.length() > 0) {
      auto offset = element["offset"].get<uint32_t>();
      auto size = element["size"].get<uint32_t>();

      auto text = addr + offset;
      codepage<> unicode(utf8.c_str());
      auto cp949 = unicode.to_cp949();

      if (*(text + size) == '\0')
        strncpy_s(reinterpret_cast<char*>(text), size + 1, cp949, size);
      else {
        memset(text, ' ', size);
        memcpy(text, cp949, min(strlen(cp949), size));
      }
      conv_cp949_to_custom(text, size);
    }
  }
}

inline void translate_open_exe() {
  translate_binary(IDR_OPEN_EXE_JSON, open_exe_ptr);
}

inline void translate_main_exe() {
  translate_binary(IDR_MAIN_EXE_JSON, main_exe_ptr);
}

void translate_scenario() {
  constexpr uint32_t sndata_offset[] = {
    0x166DB0,
    0x1EF340,
    0x1F2810,
    0x1F5CE0,
    0x1F91B0,
    0x1FC680
  };
  constexpr uint32_t firstchar_offset = 0x32;
  constexpr uint32_t sndata_01_partial_offset = 0x1EC280;
  constexpr uint32_t sndata_01_partial_firstchar_offset = 989 - 0xF;
  constexpr uint32_t sndata_01_lastchar_offset = 946;

  resource_loader rsc(IDR_SCENARIO_JSON, dll_handle);
  auto data =
      nlohmann::json::parse(rsc.get_ptr(), rsc.get_ptr() + rsc.get_length());

  uint32_t scenario_no = 0;
  for (auto& sndata : data) {
    uint8_t* file_pos = fdi_ptr + sndata_offset[scenario_no++];

    for (auto& element : sndata) {
      auto utf8 = element["kor"].get<std::string>();
      if (utf8.length() > 0) {
        auto offset = element["offset"].get<uint32_t>();
        auto size = element["size"].get<uint32_t>();

        if (scenario_no - 1 == 0 && offset > sndata_01_lastchar_offset) {
          file_pos = fdi_ptr + sndata_01_partial_offset;
          offset -= sndata_01_partial_firstchar_offset;
        } else {
          offset += firstchar_offset;
        }

        write_str_to_fdi(file_pos, offset, utf8.c_str(), size);
      }
    }
  }
}

void translate_taiki() {
  uint8_t* const taiki_base_ptr = fdi_ptr + 0x72AF0;

  resource_loader rsc(IDR_TAIKI_JSON, dll_handle);
  auto data =
      nlohmann::json::parse(rsc.get_ptr(), rsc.get_ptr() + rsc.get_length());

  for (auto& element : data) {
    auto utf8 = element["kor"].get<std::string>();
    if (utf8.length() > 0) {
      auto offset = element["offset"].get<uint32_t>();
      write_str_to_fdi(taiki_base_ptr, offset, utf8.c_str(), 0);
    }
  }
}

TRANSLATE_STATUS translate() {
  constexpr uint8_t exe_end_marker[] = { 0x0A, 0x00, 0xFF, 0xFF };
  constexpr uint8_t intro_marker[] = { 0x00, 0x38, 0x7F, 0xFF };

  static TRANSLATE_STATUS status = TRANSLATE_STATUS::OPEN_EXE;

  switch (status) {
    case TRANSLATE_STATUS::OPEN_EXE:
      if (*reinterpret_cast<uint32_t*>(open_exe_ptr + open_exe_length) ==
          *reinterpret_cast<const uint32_t*>(exe_end_marker)) {
        translate_open_exe();
        status = TRANSLATE_STATUS::INTRO;
      }
      break;
    case TRANSLATE_STATUS::INTRO:
      if (*reinterpret_cast<uint32_t*>(
              intro_bitmap_ptr + intro_bitmap_length) ==
          *reinterpret_cast<const uint32_t*>(intro_marker)) {
        create_intro_bitmap();
        status = TRANSLATE_STATUS::MAIN_EXE;
      }
    case TRANSLATE_STATUS::MAIN_EXE:
      if (*reinterpret_cast<uint32_t*>(main_exe_ptr + main_exe_length) ==
          *reinterpret_cast<const uint32_t*>(exe_end_marker)) {
        translate_main_exe();
        status = TRANSLATE_STATUS::END;
      }
      break;
  }
  return status;
}

void hook_func_impl() {
  func_hooker::rehook_on_exit hook_guard(hook_func);
  if (translate() == TRANSLATE_STATUS::END)
    hook_guard.cancel();
  hook_func_org_ptr();
}

void* const fdi_hook_return_addr = reinterpret_cast<void*>(0x00413103);
_declspec(naked) void hook_fdi_addr() {
  __asm {
    mov fdi_ptr, ebx
    jmp fdi_hook_return_addr
  }
}

bool patch_to_fdi() {
  if (fdi_ptr != nullptr) {
    translate_scenario();
    translate_taiki();
  }
  return fdi_ptr != nullptr;
}

bool patch_to_code(HMODULE hdll) {
  dll_handle = hdll;

  uint8_t jmp_to_nop_area[] = { 0xEB, 0x64 };
  code_patcher().apply(reinterpret_cast<void*>(0x413101),
    jmp_to_nop_area, sizeof(jmp_to_nop_area));

  uint8_t origin_code[] = { 0x51, 0x53 };
  code_patcher().apply(reinterpret_cast<void*>(0x413167),
    origin_code, sizeof(origin_code));

  func_hooker().install(reinterpret_cast<FARPROC>(0x413169),
      reinterpret_cast<FARPROC>(hook_fdi_addr));

  return hook_func.install(reinterpret_cast<FARPROC>(hook_func_org_ptr),
    reinterpret_cast<FARPROC>(hook_func_impl));
}

}  // namespace memory_patch
