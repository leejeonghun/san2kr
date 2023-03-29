[![License](https://img.shields.io/badge/License-BSD%202--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause)
[![CI](https://github.com/leejeonghun/san2kr/actions/workflows/main.yml/badge.svg)](https://github.com/leejeonghun/san2kr/actions/workflows/main.yml)
# san2kr


코에이테크모의 스팀판 [삼국지2](https://store.steampowered.com/app/521690/Romance_of_the_Three_Kingdoms_II/)의 한글 패치입니다.

![screenshot](https://github.com/leejeonghun/san2kr/assets/11531985/f6341895-d19c-4843-93da-5101d80598cc)


## 기능

* 본 패치 적용 시, 후리가나를 제외한 게임 내 전체 텍스트가 한글화 되며, 일본어가 포함된 경로 접근이 가능하도록 수정하여 세이브/로드가 정상적으로 동작합니다. 
* 번역된 텍스트는 [여기](https://github.com/leejeonghun/san2kr/tree/main/translate)의 JSON 파일 포맷으로 관리됩니다.
* 텍스트 번역은 비스코에서 출시한 DOS용 한글판을 참고하였습니다. 참고하기 위해 추출한 텍스트 내역은 [여기](https://github.com/leejeonghun/san2kr/tree/main/reference)서 확인 가능합니다.


## 사용법

ddraw.dll 파일을 게임 설치 폴더에 복사한 후 게임을 실행하면 게임 내 텍스트가 한글 번역되어 출력됩니다.
해당 파일을 삭제하면 원복됩니다.

개인적으로 만든 프로그램이라, 디지털 서명이 되어 있지 않습니다. 다운로드 및 압축 해제 시 웹브라우저 보안 경고 및 윈도우에서 스마트 스크린 경고가 뜰 수 있으므로 참고 부탁드립니다.


## 라이선스

BSD 라이선스를 따릅니다. MIT 라이선스인 [JSON for Modern C++](https://github.com/nlohmann/json)을 사용합니다.
