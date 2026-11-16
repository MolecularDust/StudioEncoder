<!-- languages:start -->
<div align="center">

<details>
<summary><b>🌐 Language · Idioma · Langue · Sprache · Язык · 语言 · 言語 · 언어 · भाषा · لغة</b></summary>
<br>

<table>
  <tr>
    <td align="center"><a href="../../README.md">English</a></td>
    <td align="center"><a href="README.ar.md">العربية</a></td>
    <td align="center"><a href="README.bn.md">বাংলা</a></td>
    <td align="center"><a href="README.zh-Hans.md">简体中文</a></td>
    <td align="center"><a href="README.fil.md">Filipino</a></td>
  </tr>
  <tr>
    <td align="center"><a href="README.fr.md">Français</a></td>
    <td align="center"><a href="README.de.md">Deutsch</a></td>
    <td align="center"><a href="README.hi.md">हिन्दी</a></td>
    <td align="center"><a href="README.id.md">Bahasa Indonesia</a></td>
    <td align="center"><a href="README.it.md">Italiano</a></td>
  </tr>
  <tr>
    <td align="center"><a href="README.ja.md">日本語</a></td>
    <td align="center"><b>한국어</b></td>
    <td align="center"><a href="README.fa.md">فارسی</a></td>
    <td align="center"><a href="README.pt-BR.md">Português (Brasil)</a></td>
    <td align="center"><a href="README.ru.md">Русский</a></td>
  </tr>
  <tr>
    <td align="center"><a href="README.es.md">Español</a></td>
    <td align="center"><a href="README.te.md">తెలుగు</a></td>
    <td align="center"><a href="README.th.md">ไทย</a></td>
    <td align="center"><a href="README.tr.md">Türkçe</a></td>
    <td align="center"><a href="README.vi.md">Tiếng Việt</a></td>
  </tr>
</table>

</details>

</div>
<!-- languages:end -->

# StudioEncoder: OBS Studio용 편집 친화적 인코더

> 이 번역은 AI가 작성했으며 원어민의 검수를 받지 않았습니다. OBS Studio의 메뉴, 설정, 인코더 이름은 OBS가 표시하는 그대로 영어로 적었습니다. 기준이 되는 문서는 [영어 README](../../README.md)입니다.

OBS Studio 33 이상에 10비트 올인트라(all-intra) 인코더를 추가하는 Windows용 OBS Studio 플러그인입니다. 편집이 매끄러운 4K MOV 녹화에 쓸 수 있습니다.

- **AW+**는 10비트 4:2:2 색을 지원하는 ProRes 호환 CPU 인코더입니다. FFmpeg의 ProRes 인코더(`prores_aw`)를 개선한 버전으로, ProRes Proxy 모드에서 영상이 흐려지던 반올림 버그를 고쳤습니다. 속도 최적화도 되어 있어 FFmpeg 인코더의 절반 정도의 CPU만 사용합니다.
- **10비트 4:2:2를 지원하는 GPU 인코더:** NVIDIA RTX 50 시리즈용 H.264, 그리고 Intel QuickSync를 통한 HEVC(Intel Panther Lake CPU에서 테스트).
- **10비트 4:2:0을 지원하는 GPU 인코더:** AMD GPU와 RTX 50 시리즈보다 오래된 NVIDIA 카드용 올인트라 HEVC. OBS에는 이 GPU들을 위한 하드웨어 HEVC 인코더가 이미 있지만, StudioEncoder는 난해한 설정 대신 간단한 프리셋으로 더 쉽게 쓸 수 있게 합니다. AMD GPU는 4:2:2를 인코딩할 수 없으므로 이쪽은 4:2:0으로 남습니다.

OBS 33.0 기준으로 Custom Output (FFmpeg) 경로에는 가끔 "non monotonically increasing dts" 오류를 일으켜 새 녹화가 시작되지 않게 만들 수 있는 버그가 있습니다. StudioEncoder는 이 경로를 피합니다. 인코더가 **Standard** 녹화 모드에서 동작하므로 문제없이 돌아갑니다.

> **Apple ProRes가 아닙니다.** 이 플러그인은 비공식이며 Apple과 제휴 관계가 없고 Apple의 보증도 받지 않았습니다. Apple ProRes는 라이선스가 필요한 인증 코덱입니다. 여기 있는 인코더는 공개된 포맷(SMPTE RDD 36)의 독립 구현입니다. FFmpeg의 `prores_aw`(와 이를 바탕으로 만든 AW+ 인코더), FilmCraft Rust 인코더, OxideAV Rust 인코더입니다. 출력물은 DaVinci Resolve 같은 일반적인 편집기에서 재생되지만 Apple의 인증을 받은 것은 아닙니다.

## 하드웨어

실제로 10비트 4:2:2로 녹화하는지는 캡처 카드가 결정합니다. 카드가 PC에 10비트 4:2:2를 전달해야 하기 때문입니다.

**카드.** PC에서 4K 10비트 4:2:2를 캡처하는 가장 저렴한 방법은 Blackmagic Design Intensity 또는 DeckLink 카드, 아니면 Magewell Pro Capture 카드입니다. AJA와 Datapath 카드도 가능하지만 훨씬 비쌉니다.

**OBS의 P210.** OBS는 자체 Video Capture Device 소스에서 P210 픽셀 포맷(10비트 4:2:2)을 캡처할 수 없으며, 10비트 4:2:0까지만 지원합니다. Blackmagic 카드(v210)는 DeckLink 소스를 통해 이미 OBS에서 동작합니다. StudioEncoder는 Magewell 카드와 P210을 출력하는 다른 모든 카드에 대한 지원을 추가합니다. OBS에는 없던 기능입니다. **SE: 10-bit 4:2:2 capture (P210)** 소스를 사용하세요.

**PCIe 카드.** 최대 성능으로 동작하려면 **x4 레인**으로 연결된 슬롯이 필요합니다. M.2 소켓이나 SATA 포트와 레인을 공유하지 않게 하세요. 슬롯이 x2나 x1로 줄어들 수 있습니다. 메인보드 설명서를 확인하세요.

**USB 카드.** 4K30 10비트 4:2:2는 패킹된 v210로 약 5.3 Gbit/s, P210로는 8.0 Gbit/s가 필요합니다. 5 Gbit/s USB 3.0 링크는 실제로는 약 3.5~4 Gbit/s만 전달하므로 불가능합니다. 진짜 10 Gbit/s USB 3.2 Gen 2 포트, 적합한 케이블, 10비트 4:2:2를 출력하는 카드가 필요하며, 4K30 P210이 간신히 들어갑니다. 4K60은 어떤 USB 링크에도 들어가지 않습니다. 대부분의 USB 캡처 카드는 8비트 4:2:2 또는 10비트 4:2:0만 제공하므로, 구매 전에 카드의 출력 포맷을 확인하세요.

## 인코더

| OBS의 인코더 | 구현 | 비고 |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: 플러그인에 내장된 FFmpeg의 `prores_aw`, 반올림 수정 | **권장.** CPU 부담이 적음: AVX2 CPU에서는 FFmpeg 자체 prores_aw의 절반 정도이고, 오래되었거나 저사양인 CPU에서는 일반 C로 대체됩니다(그래도 FFmpeg보다 빠름). Proxy에서도 미세한 디테일이 유지되지만, FFmpeg의 prores_aw는 Proxy를 흐리게 만듭니다. |
| ProRes-compatible (Rust / FilmCraft) (experimental) | [FilmCraft](https://github.com/storytold/filmcraft) Rust 인코더 | 실험적. 4K에서 CPU 사용량이 많습니다. |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | 실험적. 4K에서 prores_aw의 약 2.5배 CPU(22스레드 노트북에서 30% 대 12%)를 쓰고, 실제 카메라 영상의 프로파일 422에서 파일이 약 3분의 1 더 크며, 비트당 화질 이점은 없습니다. 아주 젊은 코드(0.1). |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | GPU에서 더 작은 파일. **RTX 50 시리즈** 카드가 필요합니다. 아래를 참고하세요. |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | **RTX 50 시리즈보다 오래된** NVIDIA 카드용(그런 카드에는 10비트 H.264가 없음): 10비트 **4:2:0**, 모든 프레임이 키프레임, 비트레이트 설정. 아래를 참고하세요. |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | Radeon GPU에서 10비트 **4:2:0**, 모든 프레임이 키프레임, 4개의 화질 프리셋. 아래를 참고하세요. |

프로파일: Proxy(`apco`), LT(`apcs`), 422(`apcn`), HQ(`apch`).

### 표시할 인코더 선택하기 (Tools > StudioEncoder)

OBS의 인코더 목록은 기본적으로 짧게 유지됩니다. **Tools > StudioEncoder**를 열면 이 플러그인의 모든 인코더를 세 구역으로 나누어 보여 주는 창이 열립니다. **Recommended (10-bit 4:2:2)**(AW+와 RTX 50 H.264 인코더), **10-bit 4:2:0**(NVIDIA와 AMD HEVC 인코더), **Experimental**(FilmCraft와 OxideAV, 기본값은 꺼짐)입니다. 마지막 두 구역은 접을 수 있는 그룹이며 처음에는 닫혀 있습니다. 작은 화살표를 눌러 여세요. 원하는 항목에 체크하고 OK를 누르면, OBS를 다시 시작한 뒤 적용됩니다.

<p align="center"><img src="../images/settings-window.png" alt="OBS의 StudioEncoder 설정 창(Tools > StudioEncoder): 체크박스가 있는 인코더 목록과 접을 수 있는 10-bit 4:2:0 및 Experimental 그룹" width="420"></p>

꺼 둔 인코더는 드롭다운에서만 숨겨집니다. 이미 그 인코더를 쓰는 프로필은 계속 동작합니다. 이 플러그인의 모든 인코더는 OBS 인코더 목록에서 "SE:"로 시작하므로 쉽게 찾을 수 있습니다.


### 왜 AW+인가 (수정된 prores_aw)

FFmpeg의 prores_aw는 각 디테일 계수를 양자화 단계로 나누고 **0 방향으로 버림**하기 때문에, 한 단계에 못 미치는 값은 모두 사라집니다. 단계가 거친 Proxy에서는 이것이 미세한 질감을 지워서 얼굴과 옷감이 뭉개져 보입니다. Apple의 인코더는 같은 Proxy 행렬과 양자화기를 쓰면서도 그 디테일을 유지합니다. AW+는 prores_aw를 플러그인에 이식하고 버림 대신 반올림을 적용한 것입니다(프로파일별 오프셋: Proxy 4/8, LT 3/8, 422 3/8, HQ 2/8 단계, 비트당 최고 화질이 되도록 선택).

4K 카메라 클립으로 측정(PSNR, 높을수록 좋음):

| 프로파일 | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s, 54.0 dB | **94 Mbit/s, 56.2 dB** | 96 Mbit/s, 56.5 dB |
| LT | 170 Mbit/s, 59.7 dB | 198 Mbit/s, 61.6 dB | 235 Mbit/s, 62.2 dB |
| HQ | 492 Mbit/s, 71.1 dB | 560 Mbit/s, 73.5 dB | 520 Mbit/s, 72.2 dB |

반올림을 끄면 AW+는 FFmpeg의 prores_aw와 바이트 단위로 동일한 파일을 만듭니다(네 프로파일 모두 확인).
GPU 인코더는 OBS가 시작될 때 그래픽 카드를 확인합니다. 카드가 인코더를 실행할 수 없으면(H.264 인코더에는 RTX 50 시리즈보다 오래된 NVIDIA 카드, NVIDIA HEVC 인코더에는 10비트 HEVC를 지원하는 NVIDIA 카드 없음, AMD 인코더에는 Radeon 없음) 그 인코더는 목록에서 숨겨지고 이 창에서는 회색으로 표시됩니다.

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

GPU에서 인코딩하는, 공간을 절약하는 ProRes 대안입니다. 모든 프레임이 키프레임(all-intra)이고, High 4:2:2 프로파일, 10비트, 고정 비트레이트입니다. ProRes처럼 편집되지만 화질을 약간 양보하는 대신 더 작습니다.

- **NVIDIA RTX 50 시리즈 GPU**(Blackwell)와 최신 드라이버가 필요합니다. 더 오래된 NVIDIA 카드는 10비트 H.264나 4:2:2를 인코딩할 수 없으며, 이 경우 녹화가 시작되지 않고 OBS가 오류를 표시합니다. 그런 카드에서는 ProRes(prores_aw)를 사용하세요.
- **Bitrate**: 100 Mbit/s(작음, ProRes LT 정도의 화질), **200(기본값**, ProRes LT보다 좋고 ProRes 422의 절반 정도 크기), 300(높음). 400을 넘겨도 이득이 없습니다.
- **Speed**: Quality(p5, 기본값) 또는 Fast(p1, 화질이 조금 낮음).
- 프레임은 변환 없이 P216으로 GPU에 전달됩니다. 동시에 처리되는 프레임은 최대 2개입니다. 일반 소비자용 NVIDIA 카드는 인코더 세션 수를 제한하기 때문입니다.
- RTX 5080에서 한 FFmpeg 테스트(4K 29.97 카메라 영상)에서는 p5에서 약 77 fps, p1에서 약 101 fps였습니다.

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

**RTX 50 시리즈보다 오래된** NVIDIA 카드용입니다(GTX 10 시리즈 이상이면 동작할 것으로 보이며, RTX 4070 Laptop에서 테스트). 그런 카드에는 10비트 H.264 모드도 4:2:2도 없으므로, 이것은 **10비트 4:2:0** HEVC(Main 10)입니다. 색 디테일은 ProRes 422가 유지하는 것의 절반이지만, 모든 프레임이 키프레임이라 ProRes처럼 자르고 탐색할 수 있고 파일은 더 작습니다. RTX 50 카드에서는 색 디테일을 유지하는 위의 H.264 4:2:2 인코더도 쓸 수 있습니다.

- **Settings > Advanced > Color Format: P010**(P216이 아님), Rec. 709, Limited. 영상은 OBS가 만든 그대로 변환 없이 GPU로 전달됩니다.
- **Bitrate**(30~400 Mbit/s, 기본값 150)와 **Speed**(Quality p5 또는 Fast p1). 4K 카메라 클립 하나로 한 테스트(원본 대비 루마 화질): 100 Mbit/s는 57.6 dB로 ProRes Proxy(95 Mbit/s에서 53.7 dB)보다 좋았고, 150은 59.4 dB로 AW+ LT(200 Mbit/s에서 59.6 dB)와 비슷한 화질을 비트의 70%로 냈으며, 200은 60.8 dB였습니다. Fast 프리셋은 화질이 아주 조금 낮을 뿐이고 그 GPU에서는 속도도 비슷합니다.
- 녹화물은 `hvc1` 태그가 붙은 MOV 안의 HEVC Main 10입니다(모든 프레임이 키프레임, FFmpeg에서 오류 없이 디코딩됨). 아직 Resolve나 Premiere에서는 시험하지 않았습니다.
- 동시에 인코딩되는 프레임은 최대 2개입니다. 일반 소비자용 NVIDIA 카드는 인코더 세션 수를 제한하기 때문입니다. RTX 4070 Laptop에서 4K 프레임당 약 15 ms였고, OBS는 CPU의 약 7%를 썼습니다(그중 약 5%는 장면 자체).

### HEVC 10-bit 4:2:0 Intra (AMD)

Radeon 사용자용입니다. AMD의 비디오 인코더는 4:2:2를 전혀 처리하지 못하고(H.264 인코더는 10비트를 처리하지 못합니다), 그래서 이것은 **10비트 4:2:0**입니다. 색 디테일은 ProRes 422가 유지하는 것의 절반입니다. 모든 프레임이 키프레임이므로 ProRes처럼 이웃 프레임을 디코딩하지 않고도 각 프레임을 자르고 탐색할 수 있으며, 파일은 훨씬 작습니다(테스트 클립에서 High는 ProRes 422의 약 3분의 1). OBS 자체의 "AMD HW H.265 (HEVC)"도 10비트로 녹화할 수 있지만 일반 GOP와 비트레이트 조정이 필요합니다. 이 인코더는 그것을 **Quality**라는 하나의 선택지로 대체합니다.

- **Settings > Advanced > Color Format: P010**(P216이 아님), Rec. 709, Limited.
- **Quality**는 일정하므로 파일 크기는 영상에 따라 달라집니다. 4K 29.97 카메라 영상의 일반적인 크기:

| 프리셋 | 크기 | 4K 카메라 클립의 루마 PSNR |
|---|---|---|
| Small | 약 30 Mbit/s | 52.4 dB |
| Standard | 약 70 Mbit/s | 55.3 dB (ProRes LT: 169 Mbit/s에서 56.2 dB) |
| **High**(기본값) | 약 120 Mbit/s | 56.7 dB |
| Maximum | 약 170 Mbit/s | 57.8 dB (인코더의 상한) |

  PSNR 수치는 4:2:0으로 변환한 카메라 클립과 비교한 것이므로 이 인코더에 공정합니다. ProRes는 이 인코더가 낼 수 없는 4:2:2 색을 유지합니다. 노이즈가 많은 영상은 같은 프리셋에서 더 큰 파일이 됩니다.
- Radeon RX 9070 XT에서 측정: 4K 프레임당 약 3.8 ms, 4K 29.97에서 반복 프레임 없음. AMD 인코더는 각 패킷을 한 프레임 늦게 돌려주므로 Stop을 누르기 직전의 마지막 프레임은 버려집니다.
- 녹화물은 MOV 안의 HEVC Main 10입니다(모든 프레임이 키프레임, 타임스탬프가 안정적, FFmpeg에서 오류 없이 디코딩됨). 아직 Resolve나 Premiere에서는 시험하지 않았습니다.

## 10비트 4:2:2 캡처 (P210), Windows

OBS 자체의 Video Capture Device 소스는 10비트 4:2:2를 가져올 수 없습니다. DirectShow 캡처가 P010(10비트 4:2:0)에서 멈춥니다. Magewell Pro Capture 같은 일부 캡처 카드는 10비트 4:2:2를 P210으로 제공합니다. StudioEncoder는 이를 위한 소스를 추가합니다: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**.

- Device에서 카드를 고르거나 "First device that offers P210"로 둡니다.
- Resolution과 Frame rate를 카메라의 HDMI 신호에 맞춥니다(기본값 3840x2160, 29.97).
- 10비트가 인코더에 전달되도록 **Settings > Video > Advanced > Color Format을 P216**으로 설정합니다. P216에서는 OBS가 장면을 16비트 부동소수점으로 합성합니다.
- 카드의 오디오는 별도의 Audio Input Capture 소스로 추가합니다.

프레임은 OBS를 위해 P210에서 I210으로 변환됩니다. 무손실 시프트와 플레인 분리이며, Ryzen 9 9950X3D에서 4K 프레임당 약 2.4 ms입니다. Magewell Pro Capture HDMI 4K를 3840x2160 29.97에서 테스트했습니다. 반복 프레임이 없었고 샘플에는 실제 10비트 값이 담겨 있었습니다. `tools/p210-probe.cpp`는 카드의 P210 출력에 실제 10비트 데이터가 들어 있는지 확인하는 작은 명령줄 프로그램입니다.

## 요구 사항

- Windows 10/11 x64
- OBS Studio 33.0.x (33.0.0 기준으로 빌드, 33.0.1에서 테스트)

## 설치

OBS를 닫고 릴리스 zip의 `obs-studioencoder` 폴더를 복사합니다:

- **설치형 OBS:** `C:\ProgramData\obs-studio\plugins\obs-studioencoder\`로. `ProgramData`는 숨겨져 있으니 파일 탐색기에 경로를 직접 입력하세요.
- **포터블 OBS:** `<OBS folder>\plugins\obs-studioencoder\`로, `bin`, `data`, `config`와 같은 위치에.

폴더에는 `obs-studioencoder.dll`과 `data\locale\`이 들어 있어야 합니다. OBS에서 Tools → Plugin Manager에 "StudioEncoder"가 표시되어야 하고, Tools → StudioEncoder로 설정을 열 수 있습니다.

예전 `obs-plugins\64bit`나 `data\obs-plugins` 폴더는 사용하지 마세요. OBS 33은 그 위치를 레거시로 표시합니다.

## 설정

1. Settings → Advanced: **Color Format P216**(두 HEVC 인코더는 **P010**), Color Space Rec. 709, Color Range Limited.

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings의 Advanced 탭: Colour Format P216 (16-bit, 4:2:2, 2 planes), Rec. 709, Limited가 강조 표시됨" width="560">

2. Settings → Video: 캔버스(Base)와 출력 해상도(Output)를 녹화 크기에 맞추고, 프레임 레이트는 소스에 맞춥니다. 스크린샷은 29.97에서 3840x2160입니다.

   <img src="../images/obs-video.png" alt="OBS Settings의 Video 탭: Base와 Output 해상도 3840x2160, 29.97 fps" width="560">

3. Settings → Output: Output Mode **Advanced**, Recording 탭, Type **Standard**, Recording Format **MOV**.
4. Video Encoder: 위의 인코더 중 하나("SE:"로 시작). Encoder Settings 아래의 "Required video settings" 줄은 이 인코더에 필요한 Color Format, Color Space, Color Range를 다시 알려 줍니다.

   <img src="../images/obs-output-encoder.png" alt="OBS Settings의 Output 탭, Recording: Standard 유형, QuickTime (.mov), Video Encoder SE: ProRes (AW+, fast)가 강조 표시됨" width="560">

   인코더 설정에서:
   - **Profile**(ProRes 인코더): Proxy, LT, 422 또는 HQ. **Bitrate**와 **Speed**(NVENC 인코더): 위를 참고하세요.
   - **Frames in flight**: 0(자동, 권장). 프레임은 필요에 따라 CPU 코어당 하나씩 병렬로 인코딩됩니다.
   - **Automatic sizing**(Frames in flight가 0일 때만): *Grow when frames are late*(기본값) 또는 *Size from measured frame cost*(실험적). 두 번째는 워커 4개로 시작해 프레임 하나의 시간을 재고, 필요한 워커 수를 한 번에 정합니다. 가벼운 인코더에서는 메모리를 덜 쓰고 무거운 인코더에서는 더 빨리 올라갑니다. CPU 부하는 둘 다 같습니다.
   - **On overload**: *Warn in the log* 또는 *Stop recording*.
   - **Log verbosity**: Info는 타이밍과 자동 조정을 보여 줍니다.

## 과부하

인코더가 따라가지 못하면 OBS는 타임라인을 유지하기 위해 이전 프레임을 반복하고, 그 구간에서 녹화가 멈춘 것처럼 보입니다. 플러그인은 녹화마다 이렇게 반복된 프레임을 셉니다(OBS가 같은 프레임을 연속으로 두 번 넘기면 이를 알아챕니다):
- 그런 일이 생긴 초마다 로그에 `OVERLOAD` 경고를 쓰고, 녹화가 끝날 때 요약을 남깁니다.
- **On overload: Stop recording**을 선택하면 과부하 상태가 3초 연속되었을 때 녹화를 끝내고 오류를 표시합니다.

과부하 경고가 보이면 prores_aw 인코더나 더 가벼운 프로파일로 바꾸세요.

## 알려진 제한

- 입력은 P216이어야 합니다(HEVC 인코더는 P010). OBS 33은 다른 10비트 4:2:2 출력 포맷을 제공하지 않습니다.
- Stop을 누른 뒤 캡처되었지만 아직 인코딩 중인 프레임은 버려집니다. 의도된 동작이며, 녹화는 정지 지점에서 끝납니다.
- 하이브리드 MOV는 테스트하지 않았습니다.
- Windows. Linux에서도 빌드되고 실행됩니다(Ubuntu와 CachyOS에서 테스트. `docs/HANDOFF.md`의 "Build and test on Linux"와 "Linux results" 참고).

## 빌드

빌드 안내(툴체인과 단계)는 [영어 README](../../README.md#build)에만 있습니다.

## 라이선스

OBS 플러그인에 요구되는 대로 GPL-2.0-or-later입니다(`LICENSE` 참고). 서드파티 코드:
- FFmpeg(LGPL), OBS와 함께 배포되는 그대로.
- FilmCraft ProRes 크레이트(MIT OR Apache-2.0, MIT로 사용).
- AW+(`src/prores-aw.c`): FFmpeg의 prores_aw 인코더, 순방향 DCT, ProRes 테이블의 포팅(LGPL 2.1 이상, Anatoliy Wasserman, Konstantin Shishkov, Independent JPEG Group), GPL로 사용.
- `oxideav-prores`와 `oxideav-core`(MIT), OxideAV ProRes 인코더.
