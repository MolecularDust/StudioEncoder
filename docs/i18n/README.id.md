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
    <td align="center"><b>Bahasa Indonesia</b></td>
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
    <td align="center"><a href="README.vi.md">Tiếng Việt</a></td>
  </tr>
</table>

</details>

</div>
<!-- languages:end -->

# StudioEncoder: encoder yang ramah untuk penyuntingan di OBS Studio

> Terjemahan ini ditulis oleh AI dan belum ditinjau oleh penutur asli. Nama menu, pengaturan, dan encoder OBS Studio ditulis dalam bahasa Inggris, sesuai tampilan di OBS. Rujukan utamanya adalah [README bahasa Inggris](../../README.md).

Plugin OBS Studio untuk Windows yang menambahkan encoder all-intra 10-bit ke OBS Studio 33 ke atas, untuk rekaman 4K ke MOV yang lancar disunting.

- **AW+** adalah encoder CPU yang kompatibel dengan ProRes, dengan warna 10-bit 4:2:2. Ini adalah versi yang disempurnakan dari encoder ProRes FFmpeg (`prores_aw`) dan memperbaiki bug pembulatan yang membuat gambar buram pada mode ProRes Proxy. Encoder ini juga dioptimalkan untuk kecepatan, sehingga memakai sekitar setengah CPU dibandingkan encoder FFmpeg.
- **Encoder GPU dengan 10-bit 4:2:2:** H.264 untuk kartu NVIDIA seri RTX 50, dan HEVC lewat Intel QuickSync (diuji pada CPU Intel Panther Lake).
- **Encoder GPU dengan 10-bit 4:2:0:** HEVC all-intra untuk GPU AMD dan untuk kartu NVIDIA yang lebih lama dari seri RTX 50. OBS sudah punya encoder HEVC perangkat keras untuk GPU ini, tetapi StudioEncoder membuatnya lebih mudah dipakai, dengan preset sederhana alih-alih pengaturan yang rumit. GPU AMD tidak bisa meng-encode 4:2:2, jadi encoder ini tetap 4:2:0.

<p align="center"><img src="../images/settings-window.png" alt="Jendela pengaturan StudioEncoder (Tools > StudioEncoder) di OBS: daftar encoder dengan kotak centang dan grup 10-bit 4:2:0 dan Experimental yang bisa dilipat" width="420"></p>

Sejak OBS 33.0, jalur Custom Output (FFmpeg) punya bug yang sesekali menimbulkan error «non monotonically increasing dts» dan bisa membuat rekaman baru gagal dimulai. StudioEncoder menghindari jalur ini: encodernya berjalan dalam mode rekaman **Standard**, sehingga berjalan lancar.

> **Bukan Apple ProRes.** Plugin ini tidak resmi dan tidak berafiliasi dengan atau didukung oleh Apple. Apple ProRes adalah codec berlisensi dan tersertifikasi. Encoder di sini adalah implementasi independen dari format yang didokumentasikan secara publik (SMPTE RDD 36): `prores_aw` milik FFmpeg (dan encoder AW+ yang dibangun darinya), encoder Rust FilmCraft, dan encoder Rust OxideAV. Hasilnya bisa diputar di editor umum, seperti DaVinci Resolve, tetapi tidak disertifikasi oleh Apple.

## Perangkat keras

Kartu capture menentukan apakah Anda benar-benar merekam 10-bit 4:2:2, karena kartu itu harus mengirim 10-bit 4:2:2 ke PC.

**Kartu.** Cara termurah untuk meng-capture 4K 10-bit 4:2:2 di PC adalah kartu Blackmagic Design Intensity atau DeckLink, atau kartu Magewell Pro Capture. Kartu AJA dan Datapath juga bisa, tetapi dengan harga jauh lebih tinggi.

**P210 di OBS.** OBS tidak bisa meng-capture format piksel P210 (10-bit 4:2:2) di sumber Video Capture Device miliknya sendiri, yang berhenti di 10-bit 4:2:0. Kartu Blackmagic (v210) sudah berfungsi di OBS lewat sumber DeckLink. StudioEncoder menambahkan dukungan untuk kartu Magewell dan kartu lain yang mengeluarkan P210, yang belum pernah dimiliki OBS. Gunakan sumber **SE: 10-bit 4:2:2 capture (P210)**.

**Kartu PCIe.** Kartu membutuhkan slot dengan jalur **x4** agar berjalan pada kapasitas penuh. Jangan biarkan berbagi jalur dengan soket M.2 atau port SATA, karena itu bisa menurunkan slot menjadi x2 atau x1. Periksa manual motherboard Anda.

**Kartu USB.** 4K30 10-bit 4:2:2 membutuhkan sekitar 5,3 Gbit/s sebagai v210 terkemas, atau 8,0 Gbit/s sebagai P210. Tautan USB 3.0 5 Gbit/s dalam praktik hanya membawa sekitar 3,5 sampai 4 Gbit/s, jadi tidak cukup. Anda butuh port USB 3.2 Gen 2 10 Gbit/s yang sebenarnya, kabel yang sesuai, dan kartu yang mengeluarkan 10-bit 4:2:2, dan 4K30 P210 hanya akan muat dengan pas-pasan. 4K60 tidak muat di tautan USB mana pun. Sebagian besar kartu capture USB hanya menawarkan 4:2:2 8-bit atau 4:2:0 10-bit, jadi periksa format keluaran kartu sebelum membeli.

## Encoder

| Encoder di OBS | Implementasi | Catatan |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: `prores_aw` FFmpeg yang dibangun ke dalam plugin, dengan pembulatan yang diperbaiki | **Disarankan.** Ringan untuk CPU: pada CPU dengan AVX2 porsinya sekitar setengah dari prores_aw FFmpeg sendiri, dan beralih ke C biasa (tetap lebih cepat dari FFmpeg) pada CPU lama atau kelas bawah. Proxy mempertahankan detail halus; prores_aw FFmpeg membuat Proxy buram. |
| ProRes-compatible (Rust / FilmCraft) (experimental) | Encoder Rust [FilmCraft](https://github.com/storytold/filmcraft) | Eksperimental. Pemakaian CPU berat pada 4K. |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | Eksperimental. Sekitar 2,5 kali CPU prores_aw pada 4K (30% berbanding 12% pada laptop 22 thread) dan file sekitar sepertiga lebih besar pada profil 422 untuk rekaman kamera langsung, tanpa keunggulan kualitas per bit. Kode yang sangat muda (0.1). |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | File lebih kecil di GPU. Membutuhkan kartu **seri RTX 50**. Lihat di bawah. |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | Untuk kartu NVIDIA yang **lebih lama dari seri RTX 50** (di sana tidak ada H.264 10-bit): **4:2:0** 10-bit, setiap frame adalah keyframe, satu pengaturan bitrate. Lihat di bawah. |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | **4:2:0** 10-bit, setiap frame adalah keyframe, pada GPU Radeon, dengan empat preset kualitas. Lihat di bawah. |

Profil: Proxy (`apco`), LT (`apcs`), 422 (`apcn`), HQ (`apch`).

### Memilih encoder yang ditampilkan (Tools > StudioEncoder)

Daftar encoder OBS secara default dibuat singkat. **Tools > StudioEncoder** membuka jendela yang mencantumkan setiap encoder dalam plugin ini dalam tiga bagian: **Recommended (10-bit 4:2:2)** (AW+ dan encoder H.264 RTX 50), **10-bit 4:2:0** (encoder HEVC NVIDIA dan AMD), dan **Experimental** (FilmCraft dan OxideAV, nonaktif secara default). Dua yang terakhir adalah grup yang bisa dilipat dan awalnya tertutup; klik panah kecil untuk membukanya. Centang yang Anda inginkan lalu tekan OK; perubahan berlaku setelah OBS dimulai ulang.

<p align="center"><img src="../images/settings-window.png" alt="Jendela pengaturan StudioEncoder (Tools > StudioEncoder) di OBS: daftar encoder dengan kotak centang dan grup 10-bit 4:2:0 dan Experimental yang bisa dilipat" width="420"></p>

Encoder yang dimatikan hanya disembunyikan dari dropdown. Profil yang sudah memakainya tetap berfungsi. Semua encoder dari plugin ini diawali «SE:» di daftar encoder OBS agar mudah dikenali.


### Mengapa AW+ (prores_aw yang diperbaiki)

prores_aw FFmpeg membagi setiap koefisien detail dengan langkah kuantisasinya dan **memotong ke arah nol**, sehingga apa pun yang lebih kecil dari satu langkah penuh terbuang. Pada Proxy, di mana langkahnya kasar, ini menghilangkan tekstur halus: wajah dan kain tampak kabur. Encoder Apple memakai matriks Proxy dan kuantisasi yang sama tetapi mempertahankan detail itu. AW+ adalah prores_aw yang dipindahkan ke plugin dengan pembulatan, bukan pemotongan (offset per profil: Proxy 4/8, LT 3/8, 422 3/8, HQ 2/8 langkah, dipilih untuk kualitas terbaik per bit).

Diukur pada klip kamera 4K (PSNR, makin tinggi makin baik):

| Profil | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s, 54.0 dB | **94 Mbit/s, 56.2 dB** | 96 Mbit/s, 56.5 dB |
| LT | 170 Mbit/s, 59.7 dB | 198 Mbit/s, 61.6 dB | 235 Mbit/s, 62.2 dB |
| HQ | 492 Mbit/s, 71.1 dB | 560 Mbit/s, 73.5 dB | 520 Mbit/s, 72.2 dB |

Dengan pembulatan dimatikan, AW+ menghasilkan file yang identik byte demi byte dengan prores_aw FFmpeg (diperiksa pada keempat profil).
Encoder GPU memeriksa kartu grafis Anda saat OBS dimulai. Jika kartu tidak bisa menjalankannya (kartu NVIDIA yang lebih lama dari seri RTX 50 untuk encoder H.264, tidak ada kartu NVIDIA dengan HEVC 10-bit untuk encoder HEVC NVIDIA, tidak ada Radeon untuk encoder AMD), encoder itu disembunyikan dari daftar dan ditampilkan abu-abu di jendela ini.

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

Alternatif ProRes yang hemat ruang, di-encode di GPU. Setiap frame adalah keyframe (all-intra), profil High 4:2:2, 10-bit, bitrate konstan, jadi disunting seperti ProRes tetapi lebih kecil dengan sedikit penurunan kualitas.

- **Membutuhkan GPU NVIDIA seri RTX 50** (Blackwell) dan driver terbaru. Kartu NVIDIA yang lebih lama tidak bisa meng-encode H.264 10-bit atau 4:2:2; pada kartu itu rekaman tidak akan mulai dan OBS menampilkan error. Gunakan ProRes (prores_aw) di sana.
- **Bitrate**: 100 Mbit/s (kecil, kualitas sekitar ProRes LT), **200 (default**, lebih baik dari ProRes LT, sekitar setengah ukuran ProRes 422), 300 (tinggi). Di atas 400 tidak menambah apa-apa.
- **Speed**: Quality (p5, default) atau Fast (p1, kualitas sedikit lebih rendah).
- Frame dikirim ke GPU sebagai P216 tanpa konversi. Paling banyak 2 frame diproses sekaligus, karena kartu NVIDIA konsumen membatasi jumlah sesi encoder.
- Dalam uji FFmpeg pada RTX 5080 (rekaman kamera 4K 29,97) ini berjalan sekitar 77 fps pada p5 dan 101 fps pada p1.

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

Untuk kartu NVIDIA yang **lebih lama dari seri RTX 50** (GTX seri 10 ke atas seharusnya bisa; diuji pada RTX 4070 Laptop). Kartu itu tidak punya mode H.264 10-bit dan tidak punya 4:2:2, jadi ini adalah HEVC (Main 10) **4:2:0 10-bit**: detail warnanya setengah dari yang dipertahankan ProRes 422, tetapi setiap frame adalah keyframe, sehingga bisa dipotong dan digeser seperti ProRes, dalam file yang lebih kecil. Pada kartu RTX 50 Anda juga mendapat encoder H.264 4:2:2 di atas, yang mempertahankan detail warna.

- **Settings > Advanced > Color Format: P010** (bukan P216), Rec. 709, Limited. Gambar dikirim ke GPU seperti yang dihasilkan OBS, tanpa konversi.
- **Bitrate** (30 sampai 400 Mbit/s, default 150) dan **Speed** (Quality p5, atau Fast p1). Dalam uji pada satu klip kamera 4K (kualitas luma terhadap sumber): 100 Mbit/s terukur 57,6 dB, lebih baik dari ProRes Proxy (53,7 dB pada 95); 150 terukur 59,4 dB, kira-kira sama dengan AW+ LT (59,6 dB pada 200) dengan 70% bitnya; 200 terukur 60,8 dB. Preset Fast hanya sedikit lebih rendah kualitasnya dan hampir secepat itu pada GPU tersebut.
- Rekaman berupa HEVC Main 10 dalam MOV dengan tag `hvc1` (semua frame adalah keyframe, didekode tanpa error di FFmpeg). Belum dicoba di Resolve atau Premiere.
- Paling banyak 2 frame di-encode sekaligus, karena kartu NVIDIA konsumen membatasi jumlah sesi encoder. Sekitar 15 ms per frame 4K pada RTX 4070 Laptop; OBS memakai sekitar 7% CPU (sekitar 5% di antaranya adalah scene itu sendiri).

### HEVC 10-bit 4:2:0 Intra (AMD)

Untuk pengguna Radeon. Encoder video AMD sama sekali tidak bisa 4:2:2 (dan encoder H.264-nya tidak bisa 10-bit), jadi ini **4:2:0 10-bit**: detail warnanya setengah dari yang dipertahankan ProRes 422. Setiap frame adalah keyframe, sehingga setiap frame bisa dipotong dan digeser tanpa mendekode frame di sebelahnya, seperti ProRes, dalam file yang jauh lebih kecil (High sekitar sepertiga ProRes 422 pada klip uji). «AMD HW H.265 (HEVC)» bawaan OBS juga bisa merekam 10-bit, tetapi butuh GOP normal dan penyetelan bitrate; encoder ini menggantinya dengan satu pilihan, **Quality**.

- **Settings > Advanced > Color Format: P010** (bukan P216), Rec. 709, Limited.
- **Quality** bersifat konstan, jadi ukuran file bergantung pada gambar. Ukuran umum untuk rekaman kamera 4K 29,97:

| Preset | Ukuran | PSNR luma pada klip kamera 4K |
|---|---|---|
| Small | sekitar 30 Mbit/s | 52,4 dB |
| Standard | sekitar 70 Mbit/s | 55,3 dB (ProRes LT: 56,2 dB pada 169 Mbit/s) |
| **High** (default) | sekitar 120 Mbit/s | 56,7 dB |
| Maximum | sekitar 170 Mbit/s | 57,8 dB (batas atas encoder) |

  Angka PSNR dibandingkan dengan klip kamera yang dikonversi ke 4:2:0, jadi adil bagi encoder ini; ProRes mempertahankan warna 4:2:2 yang tidak bisa dilakukan encoder ini. Rekaman yang berderau menghasilkan file lebih besar pada preset yang sama.
- Diukur pada Radeon RX 9070 XT: sekitar 3,8 ms per frame 4K, tidak ada frame berulang pada 4K 29,97. Encoder AMD mengembalikan setiap paket terlambat satu frame, jadi frame terakhir sebelum Anda menekan Stop dibuang.
- Rekaman berupa HEVC Main 10 dalam MOV (semua frame adalah keyframe, timestamp stabil, didekode tanpa error di FFmpeg). Belum dicoba di Resolve atau Premiere.

## Capture 10-bit 4:2:2 (P210), Windows

Sumber Video Capture Device milik OBS tidak bisa memasukkan 10-bit 4:2:2: capture DirectShow-nya berhenti di P010 (10-bit 4:2:0). Beberapa kartu capture, seperti Magewell Pro Capture, menawarkan 10-bit 4:2:2 sebagai P210. StudioEncoder menambahkan sumber untuk kartu tersebut: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**.

- Pilih kartu di Device, atau biarkan pada «First device that offers P210».
- Atur Resolution dan Frame rate agar sesuai dengan sinyal HDMI kamera (default 3840x2160, 29.97).
- Atur **Settings > Video > Advanced > Color Format ke P216** agar 10 bit sampai ke encoder. Dengan P216, OBS memadukan scene dalam floating point 16-bit.
- Tambahkan audio kartu dengan sumber Audio Input Capture terpisah.

Frame dikonversi dari P210 ke I210 untuk OBS: pergeseran tanpa kehilangan dan pemisahan plane, sekitar 2,4 ms per frame 4K pada Ryzen 9 9950X3D. Diuji dengan Magewell Pro Capture HDMI 4K pada 3840x2160 29.97: tidak ada frame berulang, dan sampelnya membawa nilai 10-bit yang sebenarnya. `tools/p210-probe.cpp` adalah program baris perintah kecil yang memeriksa apakah keluaran P210 sebuah kartu berisi data 10-bit yang sebenarnya.

## Persyaratan

- Windows 10/11 x64
- OBS Studio 33.0.x (dibangun dengan 33.0.0; diuji pada 33.0.1)

## Instalasi

Tutup OBS, lalu salin folder `obs-studioencoder` dari zip rilis:

- **OBS terinstal:** ke `C:\ProgramData\obs-studio\plugins\obs-studioencoder\`. `ProgramData` tersembunyi; ketik jalurnya di File Explorer.
- **OBS portabel:** ke `<OBS folder>\plugins\obs-studioencoder\`, di samping `bin`, `data`, dan `config`.

Folder itu harus berisi `obs-studioencoder.dll` dan `data\locale\`. Di OBS, Tools → Plugin Manager seharusnya mencantumkan «StudioEncoder», dan Tools → StudioEncoder membuka pengaturannya.

Jangan gunakan folder lama `obs-plugins\64bit` atau `data\obs-plugins`. OBS 33 menandai lokasi itu sebagai warisan.

## Pengaturan

1. Settings → Advanced: **Color Format P216** (**P010** untuk kedua encoder HEVC), Color Space Rec. 709, Color Range Limited.

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings, tab Advanced: Colour Format P216 (16-bit, 4:2:2, 2 planes), Rec. 709, Limited, disorot" width="560">

2. Settings → Video: atur kanvas (Base) dan resolusi keluaran (Output) sesuai ukuran rekaman Anda dan frame rate agar sama dengan sumber Anda. Tangkapan layar menunjukkan 3840x2160 pada 29.97.

   <img src="../images/obs-video.png" alt="OBS Settings, tab Video: resolusi Base dan Output 3840x2160, 29.97 fps" width="560">

3. Settings → Output: Output Mode **Advanced**, tab Recording, Type **Standard**, Recording Format **MOV**.
4. Video Encoder: salah satu encoder di atas (diawali «SE:»). Baris di bawah Encoder Settings, «Required video settings», mengulang Color Format, Color Space, dan Color Range yang dibutuhkan encoder ini.

   <img src="../images/obs-output-encoder.png" alt="OBS Settings, tab Output, Recording: tipe Standard, QuickTime (.mov), Video Encoder SE: ProRes (AW+, fast) disorot" width="560">

   Dalam pengaturannya:
   - **Profile** (encoder ProRes): Proxy, LT, 422, atau HQ. **Bitrate** dan **Speed** (encoder NVENC): lihat di atas.
   - **Frames in flight**: 0 (otomatis, disarankan). Frame di-encode secara paralel, satu per inti CPU, sesuai kebutuhan.
   - **Automatic sizing** (hanya dengan Frames in flight 0): *Grow when frames are late* (default) atau *Size from measured frame cost* (eksperimental). Yang kedua mulai dengan 4 worker, mengukur waktu satu frame, lalu menetapkan jumlah worker yang dibutuhkan sekaligus. Itu memakai lebih sedikit memori untuk encoder ringan dan naik lebih cepat untuk yang berat. Beban CPU sama pada keduanya.
   - **On overload**: *Warn in the log* atau *Stop recording*.
   - **Log verbosity**: Info menampilkan pengukuran waktu dan penyetelan otomatis.

## Kelebihan beban

Jika sebuah encoder tidak bisa mengimbangi, OBS mengulang frame sebelumnya untuk menjaga timeline, dan rekaman tampak membeku di titik-titik itu. Plugin menghitung frame berulang itu untuk setiap rekaman (ia menyadari saat OBS memberinya frame yang sama dua kali berturut-turut):
- Plugin menulis peringatan `OVERLOAD` ke log untuk setiap detik saat itu terjadi, dan ringkasan saat rekaman berakhir.
- Dengan **On overload: Stop recording**, plugin mengakhiri rekaman setelah 3 detik kelebihan beban berturut-turut dan menampilkan error.

Jika Anda melihat peringatan kelebihan beban, beralihlah ke encoder prores_aw atau profil yang lebih ringan.

## Batasan yang diketahui

- Masukan harus P216 (P010 untuk encoder HEVC). OBS 33 tidak menawarkan format keluaran 10-bit 4:2:2 lain.
- Frame yang di-capture setelah Anda menekan Stop, tetapi masih di-encode, dibuang. Ini wajar; rekaman berakhir di titik berhenti.
- MOV hybrid belum diuji.
- Windows. Linux bisa dibangun dan berjalan (diuji pada Ubuntu dan CachyOS; lihat `docs/HANDOFF.md`, «Build and test on Linux» dan «Linux results»).

## Membangun

Petunjuk membangun (toolchain dan langkah-langkahnya) hanya ada di [README bahasa Inggris](../../README.md#build).

## Lisensi

GPL-2.0-or-later (lihat `LICENSE`), sebagaimana disyaratkan untuk plugin OBS. Kode pihak ketiga:
- FFmpeg (LGPL), seperti yang disertakan dengan OBS.
- Crate ProRes FilmCraft (MIT OR Apache-2.0, digunakan di bawah MIT).
- AW+ (`src/prores-aw.c`): port dari encoder prores_aw FFmpeg, DCT maju, dan tabel ProRes (LGPL 2.1 atau lebih baru; Anatoliy Wasserman, Konstantin Shishkov, Independent JPEG Group), digunakan di bawah GPL.
- `oxideav-prores` dan `oxideav-core` (MIT), encoder ProRes OxideAV.
