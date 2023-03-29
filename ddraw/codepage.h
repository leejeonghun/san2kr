// Copyright 2023 jeonghun

#ifndef SAN2KR_DDRAW_CODEPAGE_H_
#define SAN2KR_DDRAW_CODEPAGE_H_

#include <windows.h>
#include <cstdint>

template <int T = CP_UTF8>
class codepage final {
 public:
  explicit codepage(const char* str) {
    int req_ch_cnt = MultiByteToWideChar(T, 0, str, -1, nullptr, 0);
    if (req_ch_cnt > _countof(src_buf_)) {
      unicode_ = new wchar_t[req_ch_cnt];
    }
    MultiByteToWideChar(T, 0, str, -1, unicode_, req_ch_cnt);
  }

  ~codepage() {
    if (unicode_ != src_buf_) delete[] unicode_;
    if (mbcs_ != dst_buf_) delete[] mbcs_;
  }

  inline operator const wchar_t*() const { return unicode_; }
  inline const char* to_cp949() { return conv_to(949); }
  inline const char* to_cp932() { return conv_to(932); }

 private:
  inline const char* conv_to(int codepage) {
    int req_size = WideCharToMultiByte(
        codepage, 0, unicode_, -1, nullptr, 0, nullptr, nullptr);
    if (req_size > sizeof(dst_buf_)) mbcs_ = new char[req_size];

    WideCharToMultiByte(
        codepage, 0, unicode_, -1, mbcs_, req_size, nullptr, nullptr);
    return mbcs_;
  }

  wchar_t src_buf_[128];
  char dst_buf_[sizeof(src_buf_)];
  wchar_t* unicode_ = src_buf_;
  char* mbcs_ = dst_buf_;
};

inline bool is_cp949(uint16_t mbch) {
  // °¡~°þ    B0A1~B0FE
  // ±¡~±þ    B1A1~B1FE
  // ²¡~²þ    B2A1~B2FE
  // ³¡~³þ    B3A1~B3FE
  // ´¡~´þ    B4A1~B4FE
  // µ¡~µþ    B5A1~B5FE
  // ¶¡~¶þ    B6A1~B6FE
  // ·¡~·þ    B7A1~B7FE
  // ¸¡~¸þ    B8A1~B8FE
  // ¹¡~¹þ    B9A1~B9FE
  // º¡~ºþ    BAA1~BAFE
  // »¡~»þ    BBA1~BBFE
  // ¼¡~¼þ    BCA1~BCFE
  // ½¡~½þ    BDA1~BDFE
  // ¾¡~¾þ    BEA1~BEFE
  // ¿¡~¿þ    BFA1~BFFE
  // À¡~Àþ    C0A1~C0FE
  // Á¡~Áþ    C1A1~C1FE
  // Â¡~Âþ    C2A1~C2FE
  // Ã¡~Ãþ    C4A1~C4FE
  // Ä¡~Åþ    C5A1~C5FE
  // Æ¡~Æþ    C6A1~C6FE
  // Ç¡~Çþ    C7A1~C7FE
  // È¡~Èþ    C8A1~C8FE
  const uint8_t lo = HIBYTE(mbch);
  const uint8_t hi = LOBYTE(mbch);
  return (0xB0 <= hi && hi <= 0xC8) && (0xA1 <= lo && lo <= 0xFE);
}

inline bool exist_cp949(const char* str) {
  while (*str != '\0') {
    if (static_cast<unsigned char>(*str) > 0x7F) {
      auto mbch = *reinterpret_cast<const uint16_t*>(str);
      str += 2;
      if (is_cp949(mbch)) return true;
    } else {
      str++;
    }
  }
  return false;
}

#endif  // SAN2KR_DDRAW_CODEPAGE_H_
