// Copyright 2023 jeonghun

#include "memory_patch.h"
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include "codepage.h"
#include "codetable.h"
#include "func_hooker.h"
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
constexpr uint32_t scenario_offset = 197828;
constexpr uint32_t taiki_offset = 210987;
constexpr uint32_t name_offset = 43;

const auto hook_func_org_ptr = reinterpret_cast<void (*)()>(0x00401250);
func_hooker hook_func;

HMODULE dll_handle = NULL;
std::unordered_map<uint64_t, std::string> taiki_map;

enum TRANSLATE_STATUS {
  OPEN_EXE,
  INTRO,
  MAIN_EXE,
  ETC
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

void create_intro_bitmap() {
  const wchar_t* intro_kor =
      L"2세기 말, 중국의 후한 말기, 궁정 내에서는, 중신등이 사리 사욕으로 정치를 사물화 "
      L"하고, 민중은 중세에 허덕여 사회 불안은 증대해 가기만 하는 형편이었다.\n"
      L"그러한 때 나타난 것이 태평도의 장각이다. 그는 \"푸른 세상이 망했으니 마땅히 누런 "
      L"세상이 서리\" 라는 깃발을 내걸고, 대규모의 반란을 일으켰다. 후에 \"황건적의 난\" "
      L"이라고 일컬어지는 난이다.\n\n"
      L"반란 자체는 곧 진정되었으나, 한왕실의 내부 부패, 호족의 독립화 등, 한조 사백년"
      L"의 종언을 예감시키는 사건이었다. 몇 년후 영제가 승하하고 소제가 황위를 계승하자, "
      L"외척 하진과 십상시가 궁정을 무대로 더러운 권력 투쟁을 전개했다. 하진은 사태의 "
      L"수습을 위해 제후를 불러들였으나 결국 십상시에게 암살당해, 십상시도 역시 제후에게 "
      L"암살당했다.\n\n"
      L"궁정을 무대로 일어난 이 사건은, 한왕실의 권위를 실추시키기에 충분했다. 이 혼란을 "
      L"틈탄 것이, 서량의 동탁이다. 동탁은 강대한 군사력을 배경으로, 국도 낙양에 들어가 "
      L"전권을 잡고, 소제를 폐위시키고 헌제를 즉위시키는 등, 궁정 내에서 마음대로 권세를 "
      L"부렸다.\n\n"
      L"이 동탁의 횡포에 대하여 조조는 의병을 일으켜 동탁소멸의 격문을 띄운다.\n"
      L"각지의 호웅들은 이에 응하여 봉기했다.\n"
      L"각자의 야망을 가슴 속에 감추고..\n";

  constexpr int width = 304;
  constexpr int height = 540;
  constexpr int stride = ((width + 31) & ~31);

  HDC hdc = CreateCompatibleDC(GetDC(GetDesktopWindow()));
  auto hbitmap = CreateBitmap(width, height, 1, 1, NULL);
  DeleteObject(SelectObject(hdc, hbitmap));

  HFONT hfont = CreateFont(16, 8, 0, 0, FW_NORMAL, 0, 0, 0, HANGEUL_CHARSET, 0,
      0, NONANTIALIASED_QUALITY, FIXED_PITCH, L"바탕체");
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

void translate_scenario(int scenario_no) {
  uint8_t* name_pos = main_exe_ptr + scenario_offset;

  resource_loader rsc(IDR_SCENARIO_JSON, dll_handle);
  auto data =
      nlohmann::json::parse(rsc.get_ptr(), rsc.get_ptr() + rsc.get_length());

  for (auto& element : data[scenario_no - 1]) {
    auto utf8 = element["kor"].get<std::string>();
    if (utf8.length() > 0) {
      auto offset = element["offset"].get<uint32_t>();
      auto size = element["size"].get<uint32_t>();

      auto name = name_pos + offset;
      codepage<> unicode(utf8.c_str());
      auto cp949 = unicode.to_cp949();
      memcpy(name, cp949, size);
      conv_cp949_to_custom(name, size);
    }
  }
}

const int name_total_count[] = { 150, 203, 210, 214, 196, 183 };
const int name_check_indices[] = { 7, 2 };

void clear_scenario_checkpoints() {
  uint8_t* name_pos = main_exe_ptr + scenario_offset;
  constexpr int marker_size = 4;

  for (auto count : name_total_count)
    memset(name_pos + (count - 1) * name_offset, 0, marker_size);
  for (auto index : name_check_indices)
    memset(name_pos + index * name_offset, 0, marker_size);
}

int check_scenario_no() {
  const uint8_t ref_names[][4] {
    { 0x8F, 0x99, 0x8F, 0x8E },  // 서서
    { 0xE6, 0xC9, 0xEB, 0xA8 },  // 가후
    { 0xE6, 0xE2, 0x94, 0xCD },  // 조범
    { 0x91, 0xB7, 0x8C, 0xA0 },  // 손권
    { 0x91, 0xB7, 0x8D, 0xF4 },  // 손책
    { 0x91, 0xB7, 0x8C, 0x98 },  // 손견
  };
  const int last_scenario_no = 6;

  // index 2: 1손견, 2손책, 3손권
  // index 7: 4조범, 5가후, 6서서

  uint8_t* name_pos = main_exe_ptr + scenario_offset;

  for (int i = 0; i < _countof(ref_names); i++) {
    auto name_for_check = name_pos + name_offset * name_check_indices[i > 2];
    if (*name_for_check == 0)
      return 0;
    if (memcmp(name_for_check, ref_names[i], sizeof(ref_names[i])) == 0)
      return last_scenario_no - i;
  }

  return 0;
}

bool check_scenario_load_complete(int no) {
  uint8_t* name_pos = main_exe_ptr + scenario_offset;
  uint32_t* last_name_pos = reinterpret_cast<uint32_t*>(
    name_pos + name_offset * (name_total_count[no - 1] - 1));
  return *last_name_pos != 0;
}

void load_taiki_map() {
  resource_loader rsc(IDR_TAIKI_JSON, dll_handle);
  auto data =
      nlohmann::json::parse(rsc.get_ptr(), rsc.get_ptr() + rsc.get_length());

  for (const auto& element : data.items()) {
    char buffer[8] = { 0 };
    auto key = strtoull(element.key().c_str(), nullptr, 10);
    strcpy_s(buffer,
        codepage<>(element.value().get<std::string>().c_str()).to_cp949());
    conv_cp949_to_custom(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer) - 1);
    taiki_map.emplace(key, buffer);
  }
}

void translate_taiki() {
  uint64_t hash = *reinterpret_cast<uint64_t*>(main_exe_ptr + taiki_offset);
  auto iter = taiki_map.find(hash);
  if (iter != taiki_map.end()) {
    strcpy_s(reinterpret_cast<char*>(main_exe_ptr + taiki_offset),
        iter->second.length() + 1,
        iter->second.c_str());
  }
}

void translate_etc() {
  static bool scenario_detected = false;

  if (scenario_detected == false) {
    const int scenario_no = check_scenario_no();
    if (scenario_no > 0 && check_scenario_load_complete(scenario_no)) {
      translate_scenario(scenario_no);
      scenario_detected = true;
    }
  }

  translate_taiki();
}

void translate() { 
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
        clear_scenario_checkpoints();
        load_taiki_map();
        status = TRANSLATE_STATUS::ETC;
      }
      break;
    case TRANSLATE_STATUS::ETC:
      translate_etc();
      break;
  }
}

void hook_func_impl() {
  func_hooker::rehook_on_exit hook_guard(hook_func);
  translate();
  hook_func_org_ptr();
}

bool apply(HMODULE hdll) {
  dll_handle = hdll;
  return hook_func.install(reinterpret_cast<FARPROC>(hook_func_org_ptr),
    reinterpret_cast<FARPROC>(hook_func_impl));
}

}  // namespace memory_patch
