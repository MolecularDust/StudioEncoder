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
    <td align="center"><a href="README.ko.md">한국어</a></td>
    <td align="center"><a href="README.fa.md">فارسی</a></td>
    <td align="center"><a href="README.pt-BR.md">Português (Brasil)</a></td>
    <td align="center"><a href="README.ru.md">Русский</a></td>
  </tr>
  <tr>
    <td align="center"><a href="README.es.md">Español</a></td>
    <td align="center"><a href="README.te.md">తెలుగు</a></td>
    <td align="center"><a href="README.th.md">ไทย</a></td>
    <td align="center"><a href="README.tr.md">Türkçe</a></td>
    <td align="center"><b>Tiếng Việt</b></td>
  </tr>
</table>

</details>

</div>
<!-- languages:end -->

# StudioEncoder: các bộ mã hóa thân thiện với dựng phim cho OBS Studio

> Bản dịch này do AI viết và chưa được người bản ngữ kiểm tra. Tên menu, cài đặt và bộ mã hóa của OBS Studio được giữ nguyên tiếng Anh, đúng như OBS hiển thị. Văn bản chuẩn là [README tiếng Anh](../../README.md).

Một plugin OBS Studio cho Windows, thêm các bộ mã hóa all-intra 10-bit vào OBS Studio 33 trở lên, để quay 4K sang MOV mà dựng phim mượt mà.

- **AW+** là bộ mã hóa CPU tương thích ProRes với màu 10-bit 4:2:2. Đây là phiên bản cải tiến của bộ mã hóa ProRes của FFmpeg (`prores_aw`) và sửa một lỗi làm tròn khiến hình bị mờ ở chế độ ProRes Proxy. Nó cũng được tối ưu về tốc độ, nên dùng khoảng một nửa CPU của bộ mã hóa FFmpeg.
- **Bộ mã hóa GPU với 10-bit 4:2:2:** H.264 cho card NVIDIA dòng RTX 50, và HEVC qua Intel QuickSync (đã thử trên CPU Intel Panther Lake).
- **Bộ mã hóa GPU với 10-bit 4:2:0:** HEVC all-intra cho GPU AMD và cho card NVIDIA cũ hơn dòng RTX 50. OBS đã có sẵn bộ mã hóa HEVC phần cứng cho các GPU này, nhưng StudioEncoder giúp dùng dễ hơn, với các thiết lập sẵn đơn giản thay cho những cài đặt khó hiểu. GPU AMD không mã hóa được 4:2:2 nên các bộ này vẫn là 4:2:0.

<p align="center"><img src="../images/settings-window.png" alt="Cửa sổ cài đặt StudioEncoder (Tools > StudioEncoder) trong OBS: danh sách bộ mã hóa có hộp kiểm và các nhóm 10-bit 4:2:0 và Experimental có thể thu gọn" width="420"></p>

Kể từ OBS 33.0, đường dẫn Custom Output (FFmpeg) có một lỗi thỉnh thoảng gây ra lỗi «non monotonically increasing dts» và có thể khiến bản ghi mới không bắt đầu được. StudioEncoder tránh đường dẫn này: các bộ mã hóa của nó chạy ở chế độ ghi **Standard**, nên chạy trơn tru.

> **Không phải Apple ProRes.** Plugin này không chính thức, không liên kết với Apple và không được Apple chứng nhận. Apple ProRes là một codec có giấy phép và được chứng nhận. Các bộ mã hóa ở đây là bản triển khai độc lập của định dạng đã được công bố công khai (SMPTE RDD 36): `prores_aw` của FFmpeg (và bộ mã hóa AW+ xây dựng từ đó), bộ mã hóa Rust FilmCraft và bộ mã hóa Rust OxideAV. Đầu ra của chúng phát được trong các phần mềm dựng phổ biến, như DaVinci Resolve, nhưng không được Apple chứng nhận.

## Phần cứng

Card capture quyết định bạn có thật sự ghi 10-bit 4:2:2 hay không, vì nó phải đưa 10-bit 4:2:2 vào PC.

**Card.** Cách rẻ nhất để capture 4K 10-bit 4:2:2 trên PC là card Blackmagic Design Intensity hoặc DeckLink, hay card Magewell Pro Capture. Card AJA và Datapath cũng làm được, nhưng giá cao hơn nhiều.

**P210 trong OBS.** OBS không capture được định dạng điểm ảnh P210 (10-bit 4:2:2) trong nguồn Video Capture Device của chính nó, vốn dừng ở 10-bit 4:2:0. Card Blackmagic (v210) đã chạy trong OBS qua nguồn DeckLink. StudioEncoder thêm hỗ trợ cho card Magewell và mọi card khác xuất P210, điều OBS chưa từng có. Hãy dùng nguồn **SE: 10-bit 4:2:2 capture (P210)**.

**Card PCIe.** Card cần khe nối **x4 làn** để chạy hết công suất. Đừng để nó dùng chung làn với khe M.2 hoặc cổng SATA, vì như vậy có thể hạ khe xuống x2 hoặc x1. Hãy xem sách hướng dẫn bo mạch chủ.

**Card USB.** 4K30 10-bit 4:2:2 cần khoảng 5,3 Gbit/s dưới dạng v210 đóng gói, hoặc 8,0 Gbit/s dưới dạng P210. Một liên kết USB 3.0 5 Gbit/s thực tế chỉ truyền được khoảng 3,5 đến 4 Gbit/s nên không làm được. Bạn cần cổng USB 3.2 Gen 2 10 Gbit/s thật sự, cáp phù hợp và card xuất 10-bit 4:2:2, và 4K30 P210 chỉ vừa vặn. 4K60 không vừa với bất kỳ liên kết USB nào. Phần lớn card capture USB chỉ có 4:2:2 8-bit hoặc 4:2:0 10-bit, vì vậy hãy kiểm tra các định dạng đầu ra của card trước khi mua.

## Bộ mã hóa

| Bộ mã hóa trong OBS | Cách triển khai | Ghi chú |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: `prores_aw` của FFmpeg được tích hợp vào plugin, với phép làm tròn đã sửa | **Khuyên dùng.** Nhẹ CPU: trên CPU có AVX2, phần của nó khoảng một nửa so với prores_aw của chính FFmpeg, và nó quay về C thuần (vẫn nhanh hơn FFmpeg) trên CPU cũ hoặc yếu. Proxy giữ được chi tiết nhỏ; prores_aw của FFmpeg làm Proxy bị mờ. |
| ProRes-compatible (Rust / FilmCraft) (experimental) | Bộ mã hóa Rust [FilmCraft](https://github.com/storytold/filmcraft) | Thử nghiệm. Tốn nhiều CPU ở 4K. |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | Thử nghiệm. Khoảng 2,5 lần CPU của prores_aw ở 4K (30% so với 12% trên laptop 22 luồng) và tệp lớn hơn khoảng một phần ba ở profile 422 với cảnh quay camera thực, không có ưu thế chất lượng trên mỗi bit. Mã còn rất non (0.1). |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | Tệp nhỏ hơn trên GPU. Cần card **dòng RTX 50**. Xem bên dưới. |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | Cho card NVIDIA **cũ hơn dòng RTX 50** (không có H.264 10-bit ở đó): **4:2:0** 10-bit, mỗi khung hình là một keyframe, một cài đặt bitrate. Xem bên dưới. |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | **4:2:0** 10-bit, mỗi khung hình là một keyframe, trên GPU Radeon, với bốn thiết lập chất lượng sẵn. Xem bên dưới. |

Profile: Proxy (`apco`), LT (`apcs`), 422 (`apcn`), HQ (`apch`).

### Chọn bộ mã hóa nào sẽ hiện ra (Tools > StudioEncoder)

Danh sách bộ mã hóa của OBS mặc định được giữ ngắn. **Tools > StudioEncoder** mở một cửa sổ liệt kê mọi bộ mã hóa của plugin này trong ba phần: **Recommended (10-bit 4:2:2)** (AW+ và bộ mã hóa H.264 cho RTX 50), **10-bit 4:2:0** (các bộ mã hóa HEVC của NVIDIA và AMD) và **Experimental** (FilmCraft và OxideAV, mặc định tắt). Hai nhóm cuối là nhóm có thể thu gọn và ban đầu đóng; bấm mũi tên nhỏ để mở một nhóm. Đánh dấu những bộ bạn muốn rồi bấm OK; thay đổi có hiệu lực sau khi khởi động lại OBS.

<p align="center"><img src="../images/settings-window.png" alt="Cửa sổ cài đặt StudioEncoder (Tools > StudioEncoder) trong OBS: danh sách bộ mã hóa có hộp kiểm và các nhóm 10-bit 4:2:0 và Experimental có thể thu gọn" width="420"></p>

Một bộ mã hóa bị tắt chỉ bị ẩn khỏi danh sách thả xuống. Một profile đã dùng nó vẫn hoạt động. Mọi bộ mã hóa của plugin này đều bắt đầu bằng «SE:» trong danh sách bộ mã hóa của OBS để dễ nhận ra.


### Vì sao có AW+ (prores_aw đã sửa)

prores_aw của FFmpeg chia mỗi hệ số chi tiết cho bước lượng tử hóa của nó và **cắt về phía không**, nên mọi thứ nhỏ hơn một bước đầy đủ đều bị bỏ. Ở Proxy, nơi các bước thô, điều đó xóa mất họa tiết mịn: khuôn mặt và vải trông nhòe. Bộ mã hóa của Apple dùng cùng ma trận Proxy và bộ lượng tử, nhưng giữ lại chi tiết đó. AW+ là prores_aw được chuyển vào plugin với phép làm tròn thay cho cắt bỏ (độ lệch theo profile: Proxy 4/8, LT 3/8, 422 3/8, HQ 2/8 của một bước, chọn để có chất lượng tốt nhất trên mỗi bit).

Đo trên một clip camera 4K (PSNR, càng cao càng tốt):

| Profile | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s, 54.0 dB | **94 Mbit/s, 56.2 dB** | 96 Mbit/s, 56.5 dB |
| LT | 170 Mbit/s, 59.7 dB | 198 Mbit/s, 61.6 dB | 235 Mbit/s, 62.2 dB |
| HQ | 492 Mbit/s, 71.1 dB | 560 Mbit/s, 73.5 dB | 520 Mbit/s, 72.2 dB |

Khi tắt làm tròn, AW+ tạo ra các tệp giống hệt từng byte với prores_aw của FFmpeg (đã kiểm tra trên cả bốn profile).
Các bộ mã hóa GPU kiểm tra card đồ họa của bạn khi OBS khởi động. Nếu card không chạy được một bộ (card NVIDIA cũ hơn dòng RTX 50 với bộ mã hóa H.264, không có card NVIDIA hỗ trợ HEVC 10-bit với bộ mã hóa HEVC NVIDIA, không có Radeon với bộ mã hóa AMD), bộ mã hóa đó bị ẩn khỏi danh sách và hiện màu xám trong cửa sổ này.

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

Một lựa chọn thay thế ProRes tiết kiệm dung lượng, mã hóa trên GPU. Mỗi khung hình là một keyframe (all-intra), profile High 4:2:2, 10-bit, bitrate không đổi, nên dựng như ProRes nhưng nhỏ hơn, đổi lại chất lượng giảm chút ít.

- **Cần GPU NVIDIA dòng RTX 50** (Blackwell) và driver mới. Card NVIDIA cũ hơn không mã hóa được H.264 10-bit hoặc 4:2:2; trên chúng, bản ghi không bắt đầu và OBS hiện lỗi. Ở đó hãy dùng ProRes (prores_aw).
- **Bitrate**: 100 Mbit/s (nhỏ, chất lượng cỡ ProRes LT), **200 (mặc định**, tốt hơn ProRes LT, khoảng một nửa dung lượng ProRes 422), 300 (cao). Trên 400 không thêm được gì.
- **Speed**: Quality (p5, mặc định) hoặc Fast (p1, chất lượng thấp hơn một chút).
- Khung hình đi vào GPU dưới dạng P216 mà không chuyển đổi. Tối đa 2 khung hình được xử lý cùng lúc, vì card NVIDIA dành cho người dùng phổ thông giới hạn số phiên bộ mã hóa.
- Trong một thử nghiệm FFmpeg trên RTX 5080 (cảnh quay camera 4K 29,97), nó chạy khoảng 77 fps ở p5 và 101 fps ở p1.

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

Cho card NVIDIA **cũ hơn dòng RTX 50** (GTX dòng 10 trở lên được cho là chạy được; đã thử trên RTX 4070 Laptop). Các card đó không có chế độ H.264 10-bit và không có 4:2:2, nên đây là HEVC (Main 10) **4:2:0 10-bit**: chi tiết màu chỉ bằng một nửa so với ProRes 422 giữ được, nhưng mỗi khung hình là một keyframe nên cắt và tua như ProRes, trong các tệp nhỏ hơn. Trên card RTX 50 bạn còn có bộ mã hóa H.264 4:2:2 ở trên, giữ được chi tiết màu.

- **Settings > Advanced > Color Format: P010** (không phải P216), Rec. 709, Limited. Hình đi vào GPU đúng như OBS tạo ra, không chuyển đổi.
- **Bitrate** (30 đến 400 Mbit/s, mặc định 150) và **Speed** (Quality p5, hoặc Fast p1). Trong một thử nghiệm trên một clip camera 4K (chất lượng luma so với nguồn): 100 Mbit/s đo được 57,6 dB, tốt hơn ProRes Proxy (53,7 dB ở 95); 150 đo được 59,4 dB, gần bằng AW+ LT (59,6 dB ở 200) với 70% số bit của nó; 200 đo được 60,8 dB. Thiết lập Fast chỉ thấp hơn một chút về chất lượng và gần như nhanh bằng trên GPU đó.
- Bản ghi là HEVC Main 10 trong MOV với thẻ `hvc1` (mọi khung hình đều là keyframe, giải mã không lỗi trong FFmpeg). Chưa thử trong Resolve hay Premiere.
- Tối đa 2 khung hình được mã hóa cùng lúc, vì card NVIDIA dành cho người dùng phổ thông giới hạn số phiên bộ mã hóa. Khoảng 15 ms mỗi khung hình 4K trên RTX 4070 Laptop; OBS dùng khoảng 7% CPU (khoảng 5% trong đó là chính cảnh).

### HEVC 10-bit 4:2:0 Intra (AMD)

Cho người dùng Radeon. Bộ mã hóa video của AMD hoàn toàn không làm được 4:2:2 (và bộ mã hóa H.264 của nó không làm được 10-bit), nên đây là **4:2:0 10-bit**: chi tiết màu chỉ bằng một nửa so với ProRes 422 giữ được. Mỗi khung hình là một keyframe, nên mỗi khung hình có thể cắt và tua mà không cần giải mã các khung kế bên, như ProRes, trong các tệp nhỏ hơn nhiều (High chỉ bằng khoảng một phần ba ProRes 422 trên clip thử). «AMD HW H.265 (HEVC)» của chính OBS cũng ghi được 10-bit, nhưng cần GOP thông thường và chỉnh bitrate; bộ mã hóa này thay điều đó bằng một lựa chọn duy nhất, **Quality**.

- **Settings > Advanced > Color Format: P010** (không phải P216), Rec. 709, Limited.
- **Quality** không đổi, nên dung lượng tệp phụ thuộc vào hình ảnh. Dung lượng tiêu biểu cho cảnh quay camera 4K 29,97:

| Thiết lập | Dung lượng | PSNR luma trên một clip camera 4K |
|---|---|---|
| Small | khoảng 30 Mbit/s | 52,4 dB |
| Standard | khoảng 70 Mbit/s | 55,3 dB (ProRes LT: 56,2 dB ở 169 Mbit/s) |
| **High** (mặc định) | khoảng 120 Mbit/s | 56,7 dB |
| Maximum | khoảng 170 Mbit/s | 57,8 dB (mức trần của bộ mã hóa) |

  Các con số PSNR so với clip camera đã chuyển sang 4:2:0, nên công bằng với bộ mã hóa này; ProRes giữ màu 4:2:2 mà bộ này không làm được. Cảnh quay nhiễu cho tệp lớn hơn ở cùng thiết lập.
- Đo trên Radeon RX 9070 XT: khoảng 3,8 ms mỗi khung hình 4K, không có khung hình lặp ở 4K 29,97. Bộ mã hóa AMD trả về mỗi gói chậm một khung hình, nên khung hình cuối cùng trước khi bạn bấm Stop bị bỏ.
- Bản ghi là HEVC Main 10 trong MOV (mọi khung hình đều là keyframe, dấu thời gian ổn định, giải mã không lỗi trong FFmpeg). Chưa thử trong Resolve hay Premiere.

## Capture 10-bit 4:2:2 (P210), Windows

Nguồn Video Capture Device của chính OBS không đưa được 10-bit 4:2:2 vào: capture DirectShow của nó dừng ở P010 (10-bit 4:2:0). Một số card capture, như Magewell Pro Capture, cung cấp 10-bit 4:2:2 dưới dạng P210. StudioEncoder thêm một nguồn cho chúng: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**.

- Chọn card ở Device, hoặc để «First device that offers P210».
- Đặt Resolution và Frame rate khớp với tín hiệu HDMI của camera (mặc định 3840x2160, 29.97).
- Đặt **Settings > Video > Advanced > Color Format thành P216** để 10 bit đến được bộ mã hóa. Với P216, OBS trộn cảnh bằng dấu phẩy động 16-bit.
- Thêm âm thanh của card bằng một nguồn Audio Input Capture riêng.

Khung hình được chuyển từ P210 sang I210 cho OBS: một phép dịch bit không mất dữ liệu và tách plane, khoảng 2,4 ms mỗi khung hình 4K trên Ryzen 9 9950X3D. Đã thử với Magewell Pro Capture HDMI 4K ở 3840x2160 29.97: không có khung hình lặp, và các mẫu mang giá trị 10-bit thật. `tools/p210-probe.cpp` là một chương trình dòng lệnh nhỏ kiểm tra xem đầu ra P210 của một card có chứa dữ liệu 10-bit thật hay không.

## Yêu cầu

- Windows 10/11 x64
- OBS Studio 33.0.x (biên dịch với 33.0.0; đã thử trên 33.0.1)

## Cài đặt

Đóng OBS, rồi sao chép thư mục `obs-studioencoder` từ tệp zip của bản phát hành:

- **OBS đã cài đặt:** vào `C:\ProgramData\obs-studio\plugins\obs-studioencoder\`. `ProgramData` bị ẩn; hãy gõ đường dẫn vào File Explorer.
- **OBS portable:** vào `<OBS folder>\plugins\obs-studioencoder\`, cạnh `bin`, `data` và `config`.

Thư mục phải chứa `obs-studioencoder.dll` và `data\locale\`. Trong OBS, Tools → Plugin Manager sẽ liệt kê «StudioEncoder», và Tools → StudioEncoder mở phần cài đặt của nó.

Đừng dùng các thư mục cũ `obs-plugins\64bit` hoặc `data\obs-plugins`. OBS 33 đánh dấu các vị trí đó là cũ.

## Thiết lập

1. Settings → Advanced: **Color Format P216** (**P010** cho hai bộ mã hóa HEVC), Color Space Rec. 709, Color Range Limited.

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings, thẻ Advanced: Colour Format P216 (16-bit, 4:2:2, 2 planes), Rec. 709, Limited, được tô sáng" width="560">

2. Settings → Video: đặt canvas (Base) và độ phân giải đầu ra (Output) theo kích thước bản ghi của bạn, và tốc độ khung hình khớp với nguồn. Ảnh chụp màn hình cho thấy 3840x2160 ở 29.97.

   <img src="../images/obs-video.png" alt="OBS Settings, thẻ Video: độ phân giải Base và Output 3840x2160, 29.97 fps" width="560">

3. Settings → Output: Output Mode **Advanced**, thẻ Recording, Type **Standard**, Recording Format **MOV**.
4. Video Encoder: một trong các bộ mã hóa ở trên (bắt đầu bằng «SE:»). Dòng dưới Encoder Settings, «Required video settings», nhắc lại Color Format, Color Space và Color Range mà bộ mã hóa này cần.

   <img src="../images/obs-output-encoder.png" alt="OBS Settings, thẻ Output, Recording: loại Standard, QuickTime (.mov), Video Encoder SE: ProRes (AW+, fast) được tô sáng" width="560">

   Trong phần cài đặt của nó:
   - **Profile** (bộ mã hóa ProRes): Proxy, LT, 422 hoặc HQ. **Bitrate** và **Speed** (bộ mã hóa NVENC): xem ở trên.
   - **Frames in flight**: 0 (tự động, khuyên dùng). Khung hình được mã hóa song song, mỗi lõi CPU một khung, khi cần.
   - **Automatic sizing** (chỉ khi Frames in flight bằng 0): *Grow when frames are late* (mặc định) hoặc *Size from measured frame cost* (thử nghiệm). Cái thứ hai bắt đầu với 4 worker, đo thời gian một khung hình, rồi đặt số worker cần thiết trong một bước. Cách đó dùng ít bộ nhớ hơn cho bộ mã hóa nhẹ và tăng nhanh hơn cho bộ nặng. Tải CPU như nhau ở cả hai.
   - **On overload**: *Warn in the log* hoặc *Stop recording*.
   - **Log verbosity**: Info hiển thị thời gian và việc tự điều chỉnh.

## Quá tải

Nếu một bộ mã hóa không theo kịp, OBS lặp lại khung hình trước để giữ đúng dòng thời gian, và bản ghi trông như bị đứng ở những chỗ đó. Plugin đếm các khung hình lặp đó cho mỗi bản ghi (nó nhận ra khi OBS đưa cho nó cùng một khung hình hai lần liên tiếp):
- Nó ghi cảnh báo `OVERLOAD` vào nhật ký cho mỗi giây xảy ra điều đó, và một bản tóm tắt khi bản ghi kết thúc.
- Với **On overload: Stop recording**, nó kết thúc bản ghi sau 3 giây quá tải liên tiếp và hiện lỗi.

Nếu thấy cảnh báo quá tải, hãy chuyển sang bộ mã hóa prores_aw hoặc một profile nhẹ hơn.

## Giới hạn đã biết

- Đầu vào phải là P216 (P010 cho các bộ mã hóa HEVC). OBS 33 không có định dạng đầu ra 10-bit 4:2:2 nào khác.
- Các khung hình được capture sau khi bạn bấm Stop nhưng vẫn đang mã hóa sẽ bị bỏ. Điều này là bình thường; bản ghi kết thúc tại điểm dừng.
- MOV hybrid chưa được thử.
- Windows. Linux biên dịch và chạy được (đã thử trên Ubuntu và CachyOS; xem `docs/HANDOFF.md`, «Build and test on Linux» và «Linux results»).

## Biên dịch

Hướng dẫn biên dịch (toolchain và các bước) chỉ có trong [README tiếng Anh](../../README.md#build).

## Giấy phép

GPL-2.0-or-later (xem `LICENSE`), theo yêu cầu đối với plugin OBS. Mã của bên thứ ba:
- FFmpeg (LGPL), như được phân phối cùng OBS.
- Crate ProRes của FilmCraft (MIT OR Apache-2.0, dùng theo MIT).
- AW+ (`src/prores-aw.c`): bản port của bộ mã hóa prores_aw của FFmpeg, DCT thuận và các bảng ProRes (LGPL 2.1 trở lên; Anatoliy Wasserman, Konstantin Shishkov, Independent JPEG Group), dùng theo GPL.
- `oxideav-prores` và `oxideav-core` (MIT), bộ mã hóa ProRes OxideAV.
