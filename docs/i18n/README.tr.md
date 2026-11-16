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
    <td align="center"><b>Türkçe</b></td>
    <td align="center"><a href="README.vi.md">Tiếng Việt</a></td>
  </tr>
</table>

</details>

</div>
<!-- languages:end -->

# StudioEncoder: OBS Studio için kurguya uygun kodlayıcılar

> Bu çeviri bir yapay zekâ tarafından yazıldı ve anadili konuşanlarca gözden geçirilmedi. OBS Studio'daki menü, ayar ve kodlayıcı adları OBS'nin gösterdiği gibi İngilizce bırakılmıştır. Esas alınacak metin [İngilizce README](../../README.md) dosyasıdır.

OBS Studio 33 ve üzerine, kurguda akıcı çalışan MOV'a 4K kayıt için tüm kareleri anahtar kare olan (all-intra) 10 bit kodlayıcılar ekleyen, Windows için bir OBS Studio eklentisi.

- **AW+**, 10 bit 4:2:2 renkli, ProRes ile uyumlu bir CPU kodlayıcısıdır. FFmpeg'in ProRes kodlayıcısının (`prores_aw`) geliştirilmiş bir sürümüdür ve ProRes Proxy modunda görüntüyü bulanıklaştıran bir yuvarlama hatasını düzeltir. Ayrıca hız için optimize edilmiştir; FFmpeg'in kodlayıcısının yaklaşık yarısı kadar CPU kullanır.
- **10 bit 4:2:2 destekli GPU kodlayıcıları:** NVIDIA RTX 50 serisi kartlar için H.264 ve Intel QuickSync üzerinden HEVC (bir Intel Panther Lake CPU'da denendi).
- **10 bit 4:2:0 destekli GPU kodlayıcıları:** AMD GPU'lar ve RTX 50 serisinden eski NVIDIA kartlar için all-intra HEVC. OBS'nin bu GPU'lar için zaten donanım HEVC kodlayıcıları var, ancak StudioEncoder bunları karmaşık ayarlar yerine basit hazır ayarlarla kullanmayı kolaylaştırır. AMD GPU'lar 4:2:2 kodlayamaz, bu yüzden bunlar 4:2:0 kalır.

OBS 33.0'dan itibaren Custom Output (FFmpeg) yolunda zaman zaman «non monotonically increasing dts» hatasına yol açan ve yeni bir kaydın başlamasını engelleyebilen bir hata var. StudioEncoder bu yoldan kaçınır: kodlayıcıları **Standard** kayıt modunda çalışır, böylece sorunsuz çalışır.

> **Apple ProRes değildir.** Bu eklenti resmî değildir; Apple ile bağlantılı değildir ve Apple tarafından onaylanmamıştır. Apple ProRes lisanslı ve sertifikalı bir codec'tir. Buradaki kodlayıcılar, kamuya açık biçimin (SMPTE RDD 36) bağımsız uygulamalarıdır: FFmpeg'in `prores_aw` kodlayıcısı (ve ondan türetilen AW+ kodlayıcısı), FilmCraft Rust kodlayıcısı ve OxideAV Rust kodlayıcısı. Çıktıları DaVinci Resolve gibi yaygın kurgu programlarında oynatılır, ancak Apple tarafından sertifikalandırılmamıştır.

## Donanım

Gerçekten 10 bit 4:2:2 kaydedip kaydetmediğinizi yakalama kartı belirler, çünkü PC'ye 10 bit 4:2:2 iletmesi gerekir.

**Kartlar.** Bir PC'de 4K 10 bit 4:2:2 yakalamanın en ucuz yolu bir Blackmagic Design Intensity veya DeckLink kartı ya da bir Magewell Pro Capture kartıdır. AJA ve Datapath kartları da bunu yapar, ancak çok daha pahalıdır.

**OBS'de P210.** OBS, kendi Video Capture Device kaynağında P210 piksel biçimini (10 bit 4:2:2) yakalayamaz; kaynak 10 bit 4:2:0'da durur. Blackmagic kartları (v210) OBS'de DeckLink kaynağı üzerinden zaten çalışır. StudioEncoder, Magewell kartlarına ve P210 çıkışı veren diğer tüm kartlara destek ekler; OBS'de bu hiç olmamıştı. **SE: 10-bit 4:2:2 capture (P210)** kaynağını kullanın.

**PCIe kartları.** Kartın tam kapasiteyle çalışması için **x4 hatla** bağlı bir yuvaya ihtiyacı vardır. Hatlarını bir M.2 soketi veya SATA portlarıyla paylaşmasına izin vermeyin; bu, yuvayı x2 veya x1'e düşürebilir. Anakartınızın kılavuzuna bakın.

**USB kartları.** 4K30 10 bit 4:2:2, paketlenmiş v210 olarak yaklaşık 5,3 Gbit/s, P210 olarak 8,0 Gbit/s gerektirir. 5 Gbit/s'lik bir USB 3.0 bağlantısı pratikte yalnızca yaklaşık 3,5 ila 4 Gbit/s taşır, bu yüzden bunu yapamaz. Gerçek bir 10 Gbit/s USB 3.2 Gen 2 portuna, uygun bir kabloya ve 10 bit 4:2:2 çıkışı veren bir karta ihtiyacınız vardır; 4K30 P210 ancak zar zor sığar. 4K60 hiçbir USB bağlantısına sığmaz. Çoğu USB yakalama kartı yalnızca 8 bit 4:2:2 veya 10 bit 4:2:0 sunar, bu yüzden satın almadan önce kartın çıkış biçimlerine bakın.

## Kodlayıcılar

| OBS'deki kodlayıcı | Uygulama | Notlar |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: eklentiye gömülü, yuvarlaması düzeltilmiş FFmpeg `prores_aw` | **Önerilir.** CPU'ya hafiftir: AVX2'li CPU'larda payı FFmpeg'in kendi prores_aw'sinin yaklaşık yarısıdır, eski veya düşük seviye CPU'larda saf C'ye geri döner (yine de FFmpeg'den hızlıdır). Proxy ince ayrıntıyı korur; FFmpeg'in prores_aw'si Proxy'yi bulanıklaştırır. |
| ProRes-compatible (Rust / FilmCraft) (experimental) | [FilmCraft](https://github.com/storytold/filmcraft) Rust kodlayıcısı | Deneysel. 4K'da yoğun CPU kullanımı. |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | Deneysel. 4K'da prores_aw'nin yaklaşık 2,5 katı CPU (22 iş parçacıklı bir dizüstünde %12'ye karşı %30) ve canlı kamera görüntüsünde 422 profilinde yaklaşık üçte bir daha büyük dosyalar; bit başına kalite avantajı yok. Çok genç kod (0.1). |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | GPU'da daha küçük dosyalar. **RTX 50 serisi** bir kart gerekir. Aşağıya bakın. |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | **RTX 50 serisinden eski** NVIDIA kartlar için (onlarda 10 bit H.264 yok): 10 bit **4:2:0**, her kare bir anahtar kare, bir bit hızı ayarı. Aşağıya bakın. |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | Bir Radeon GPU'da 10 bit **4:2:0**, her kare bir anahtar kare, dört kalite hazır ayarıyla. Aşağıya bakın. |

Profiller: Proxy (`apco`), LT (`apcs`), 422 (`apcn`), HQ (`apch`).

### Hangi kodlayıcıların görüneceğini seçme (Tools > StudioEncoder)

OBS'nin kodlayıcı listesi varsayılan olarak kısa kalır. **Tools > StudioEncoder**, bu eklentinin her kodlayıcısını üç bölümde listeleyen bir pencere açar: **Recommended (10-bit 4:2:2)** (AW+ ve RTX 50 H.264 kodlayıcısı), **10-bit 4:2:0** (NVIDIA ve AMD HEVC kodlayıcıları) ve **Experimental** (FilmCraft ve OxideAV, varsayılan olarak kapalı). Son ikisi kapalı başlayan, daraltılabilir gruplardır; birini açmak için küçük oka tıklayın. İstediklerinizi işaretleyip OK'e basın; değişiklik OBS'yi yeniden başlattıktan sonra geçerli olur.

<p align="center"><img src="../images/settings-window.png" alt="OBS'de StudioEncoder ayar penceresi (Tools > StudioEncoder): onay kutulu bir kodlayıcı listesi ve daraltılabilir 10-bit 4:2:0 ile Experimental grupları" width="420"></p>

Kapatılan bir kodlayıcı yalnızca açılır listeden gizlenir. Onu zaten kullanan bir profil çalışmaya devam eder. Bu eklentideki tüm kodlayıcılar OBS'nin kodlayıcı listesinde kolay fark edilsin diye «SE:» ile başlar.


### Neden AW+ (düzeltilmiş prores_aw)

FFmpeg'in prores_aw'si her ayrıntı katsayısını nicemleme adımına böler ve **sıfıra doğru keser**; böylece tam bir adımdan küçük her şey atılır. Adımların kaba olduğu Proxy'de bu, ince dokuyu siler: yüzler ve kumaşlar silik görünür. Apple'ın kodlayıcısı aynı Proxy matrisini ve niceleyiciyi kullanır ama bu ayrıntıyı korur. AW+, kesme yerine yuvarlama ile eklentiye taşınmış prores_aw'dir (profil başına ofset: Proxy 4/8, LT 3/8, 422 3/8, HQ 2/8 adım; bit başına en iyi kalite için seçildi).

4K bir kamera klibinde ölçüldü (PSNR, yüksek olan iyi):

| Profil | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s, 54.0 dB | **94 Mbit/s, 56.2 dB** | 96 Mbit/s, 56.5 dB |
| LT | 170 Mbit/s, 59.7 dB | 198 Mbit/s, 61.6 dB | 235 Mbit/s, 62.2 dB |
| HQ | 492 Mbit/s, 71.1 dB | 560 Mbit/s, 73.5 dB | 520 Mbit/s, 72.2 dB |

Yuvarlama kapatıldığında AW+, FFmpeg'in prores_aw'si ile bayt bayt aynı dosyaları üretir (dört profilde de denetlendi).
GPU kodlayıcıları OBS başlarken ekran kartınızı denetler. Kart birini çalıştıramıyorsa (H.264 kodlayıcısı için RTX 50 serisinden eski bir NVIDIA kartı, NVIDIA HEVC kodlayıcısı için 10 bit HEVC destekli NVIDIA kartı olmaması, AMD kodlayıcısı için Radeon olmaması), o kodlayıcı listeden gizlenir ve bu pencerede gri gösterilir.

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

ProRes'e yer tasarrufu sağlayan, GPU'da kodlanan bir alternatif. Her kare bir anahtar karedir (all-intra), High 4:2:2 profili, 10 bit, sabit bit hızı; yani ProRes gibi kurgulanır ama küçük bir kalite kaybıyla daha küçüktür.

- **NVIDIA RTX 50 serisi bir GPU** (Blackwell) ve güncel bir sürücü gerekir. Daha eski NVIDIA kartlar 10 bit H.264 veya 4:2:2 kodlayamaz; onlarda kayıt başlamaz ve OBS bir hata gösterir. Orada ProRes (prores_aw) kullanın.
- **Bitrate**: 100 Mbit/s (küçük, yaklaşık ProRes LT kalitesi), **200 (varsayılan**, ProRes LT'den iyi, ProRes 422'nin yaklaşık yarı boyutu), 300 (yüksek). 400'ün üstü bir şey katmaz.
- **Speed**: Quality (p5, varsayılan) veya Fast (p1, biraz daha düşük kalite).
- Kareler GPU'ya dönüştürülmeden P216 olarak gider. Aynı anda en fazla 2 kare işlenir, çünkü tüketici sınıfı NVIDIA kartları kodlayıcı oturumu sayısını sınırlar.
- Bir RTX 5080 üzerinde yapılan FFmpeg testinde (4K 29,97 kamera görüntüsü) p5'te yaklaşık 77 fps, p1'de 101 fps ile çalıştı.

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

**RTX 50 serisinden eski** NVIDIA kartlar için (GTX 10 serisi ve yenileri çalışmalı; bir RTX 4070 Laptop'ta denendi). Bu kartlarda 10 bit H.264 modu da 4:2:2 de yoktur; bu yüzden bu, **10 bit 4:2:0** HEVC'dir (Main 10): renk ayrıntısı ProRes 422'nin koruduğunun yarısıdır, ama her kare bir anahtar karedir; yani ProRes gibi kesilir ve gezilir, daha küçük dosyalarla. Bir RTX 50 kartta ayrıca renk ayrıntısını koruyan yukarıdaki H.264 4:2:2 kodlayıcısı da vardır.

- **Settings > Advanced > Color Format: P010** (P216 değil), Rec. 709, Limited. Görüntü, OBS'nin ürettiği haliyle, dönüştürme olmadan GPU'ya gider.
- **Bitrate** (30 ila 400 Mbit/s, varsayılan 150) ve **Speed** (Quality p5 veya Fast p1). Tek bir 4K kamera klibinde yapılan testte (kaynağa göre luma kalitesi): 100 Mbit/s 57,6 dB ölçüldü, ProRes Proxy'den (95'te 53,7 dB) iyi; 150, 59,4 dB ölçüldü, bitlerinin %70'iyle AW+ LT ile (200'de 59,6 dB) hemen hemen aynı; 200, 60,8 dB ölçüldü. Fast hazır ayarı yalnızca biraz daha düşük kalitede ve o GPU'da yaklaşık aynı hızda.
- Kayıtlar, `hvc1` etiketli MOV içinde HEVC Main 10'dur (tüm kareler anahtar kare, FFmpeg'de hatasız çözülür). Resolve veya Premiere'de henüz denenmedi.
- Aynı anda en fazla 2 kare kodlanır, çünkü tüketici sınıfı NVIDIA kartları kodlayıcı oturumu sayısını sınırlar. RTX 4070 Laptop'ta 4K kare başına yaklaşık 15 ms; OBS CPU'nun yaklaşık %7'sini kullandı (bunun yaklaşık %5'i sahnenin kendisi).

### HEVC 10-bit 4:2:0 Intra (AMD)

Radeon kullanıcıları için. AMD'nin video kodlayıcısı 4:2:2'yi hiç yapamaz (H.264 kodlayıcısı da 10 bit yapamaz); bu yüzden bu, **10 bit 4:2:0**'dır: renk ayrıntısı ProRes 422'nin koruduğunun yarısıdır. Her kare bir anahtar karedir; böylece ProRes'te olduğu gibi her kare komşularını çözmeden kesilebilir ve gezilebilir, çok daha küçük dosyalarla (test klibinde High, ProRes 422'nin yaklaşık üçte biri). OBS'nin kendi «AMD HW H.265 (HEVC)» kodlayıcısı da 10 bit kaydedebilir ama normal bir GOP ve bit hızı ayarı gerektirir; bu kodlayıcı bunu tek bir **Quality** seçimiyle değiştirir.

- **Settings > Advanced > Color Format: P010** (P216 değil), Rec. 709, Limited.
- **Quality** sabittir, bu yüzden dosya boyutu görüntüye bağlıdır. 4K 29,97 kamera görüntüsü için tipik boyutlar:

| Hazır ayar | Boyut | 4K kamera klibinde luma PSNR |
|---|---|---|
| Small | yaklaşık 30 Mbit/s | 52,4 dB |
| Standard | yaklaşık 70 Mbit/s | 55,3 dB (ProRes LT: 169 Mbit/s'de 56,2 dB) |
| **High** (varsayılan) | yaklaşık 120 Mbit/s | 56,7 dB |
| Maximum | yaklaşık 170 Mbit/s | 57,8 dB (kodlayıcının tavanı) |

  PSNR değerleri 4:2:0'a dönüştürülmüş kamera klibiyle karşılaştırır; bu yüzden bu kodlayıcı için adildir; ProRes, bu kodlayıcının yapamadığı 4:2:2 rengi korur. Gürültülü görüntü aynı hazır ayarda daha büyük dosya üretir.
- Bir Radeon RX 9070 XT üzerinde ölçüldü: 4K kare başına yaklaşık 3,8 ms, 4K 29,97'de tekrarlanan kare yok. AMD kodlayıcısı her paketi bir kare geç döndürür; bu yüzden Stop'a bastığınız andan önceki son kare atılır.
- Kayıtlar MOV içinde HEVC Main 10'dur (tüm kareler anahtar kare, zaman damgaları düzenli, FFmpeg'de hatasız çözülür). Resolve veya Premiere'de henüz denenmedi.

## 10 bit 4:2:2 yakalama (P210), Windows

OBS'nin kendi Video Capture Device kaynağı 10 bit 4:2:2'yi içeri alamaz: DirectShow yakalaması P010'da (10 bit 4:2:0) durur. Magewell Pro Capture gibi bazı yakalama kartları 10 bit 4:2:2'yi P210 olarak sunar. StudioEncoder onlar için bir kaynak ekler: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**.

- Kartı Device altında seçin ya da «First device that offers P210» üzerinde bırakın.
- Resolution ve Frame rate'i kameranın HDMI sinyaliyle eşleştirin (varsayılan 3840x2160, 29.97).
- 10 bitin kodlayıcıya ulaşması için **Settings > Video > Advanced > Color Format'ı P216** yapın. P216 ile OBS sahneyi 16 bit kayan noktada karıştırır.
- Kartın sesini ayrı bir Audio Input Capture kaynağıyla ekleyin.

Kareler OBS için P210'dan I210'a dönüştürülür: kayıpsız bir kaydırma ve düzlem ayırma, bir Ryzen 9 9950X3D'de 4K kare başına yaklaşık 2,4 ms. 3840x2160 29.97'de bir Magewell Pro Capture HDMI 4K ile denendi: tekrarlanan kare yok ve örnekler gerçek 10 bit değerler taşıyor. `tools/p210-probe.cpp`, bir kartın P210 çıkışının gerçek 10 bit veri taşıyıp taşımadığını denetleyen küçük bir komut satırı programıdır.

## Gereksinimler

- Windows 10/11 x64
- OBS Studio 33.0.x (33.0.0 ile derlendi; 33.0.1'de denendi)

## Kurulum

OBS'yi kapatın, sonra sürüm zip'indeki `obs-studioencoder` klasörünü kopyalayın:

- **Kurulu OBS:** `C:\ProgramData\obs-studio\plugins\obs-studioencoder\` konumuna. `ProgramData` gizlidir; yolu Dosya Gezgini'ne yazın.
- **Taşınabilir OBS:** `<OBS folder>\plugins\obs-studioencoder\` konumuna, `bin`, `data` ve `config` klasörlerinin yanına.

Klasör `obs-studioencoder.dll` ve `data\locale\` içermelidir. OBS'de Tools → Plugin Manager «StudioEncoder»'ı listelemeli ve Tools → StudioEncoder ayarlarını açmalıdır.

Eski `obs-plugins\64bit` veya `data\obs-plugins` klasörlerini kullanmayın. OBS 33 bu konumları eski olarak işaretler.

## Ayarlar

1. Settings → Advanced: **Color Format P216** (iki HEVC kodlayıcısı için **P010**), Color Space Rec. 709, Color Range Limited.

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings, Advanced sekmesi: Colour Format P216 (16-bit, 4:2:2, 2 planes), Rec. 709, Limited, vurgulanmış" width="560">

2. Settings → Video: tuvali (Base) ve çıkış çözünürlüğünü (Output) kayıt boyutunuza, kare hızını kaynağınıza uyacak şekilde ayarlayın. Ekran görüntüsünde 29.97'de 3840x2160 görülüyor.

   <img src="../images/obs-video.png" alt="OBS Settings, Video sekmesi: Base ve Output çözünürlüğü 3840x2160, 29.97 fps" width="560">

3. Settings → Output: Output Mode **Advanced**, Recording sekmesi, Type **Standard**, Recording Format **MOV**.
4. Video Encoder: yukarıdaki kodlayıcılardan biri («SE:» ile başlar). Encoder Settings altındaki «Required video settings» satırı, bu kodlayıcının ihtiyaç duyduğu Color Format, Color Space ve Color Range'i tekrarlar.

   <img src="../images/obs-output-encoder.png" alt="OBS Settings, Output sekmesi, Recording: Standard türü, QuickTime (.mov), Video Encoder SE: ProRes (AW+, fast) vurgulanmış" width="560">

   Ayarlarında:
   - **Profile** (ProRes kodlayıcıları): Proxy, LT, 422 veya HQ. **Bitrate** ve **Speed** (NVENC kodlayıcısı): yukarıya bakın.
   - **Frames in flight**: 0 (otomatik, önerilir). Kareler gerektiğinde, CPU çekirdeği başına bir tane olacak şekilde paralel kodlanır.
   - **Automatic sizing** (yalnızca Frames in flight 0 iken): *Grow when frames are late* (varsayılan) veya *Size from measured frame cost* (deneysel). İkincisi 4 worker ile başlar, bir kareyi zamanlar, sonra gereken worker sayısını tek adımda ayarlar. Bu, hafif kodlayıcılarda daha az bellek kullanır ve ağır olanlarda daha hızlı yükselir. CPU yükü her iki durumda aynıdır.
   - **On overload**: *Warn in the log* veya *Stop recording*.
   - **Log verbosity**: Info zamanlamayı ve otomatik ayarı gösterir.

## Aşırı yük

Bir kodlayıcı yetişemezse OBS zaman çizelgesini korumak için bir önceki kareyi tekrarlar ve kayıt o noktalarda donmuş gibi görünür. Eklenti bu tekrarlanan kareleri her kayıt için sayar (OBS ona art arda iki kez aynı kareyi verdiğinde fark eder):
- Bunun olduğu her saniye için günlüğe bir `OVERLOAD` uyarısı yazar, kayıt bittiğinde de bir özet.
- **On overload: Stop recording** ile, art arda 3 aşırı yüklü saniyeden sonra kaydı bitirir ve bir hata gösterir.

Aşırı yük uyarıları görürseniz prores_aw kodlayıcısına ya da daha hafif bir profile geçin.

## Bilinen sınırlar

- Girdi P216 olmalıdır (HEVC kodlayıcıları için P010). OBS 33 başka bir 10 bit 4:2:2 çıkış biçimi sunmaz.
- Stop'a bastıktan sonra yakalanan ama hâlâ kodlanmakta olan kareler atılır. Bu beklenen bir durumdur; kayıt durdurma noktasında biter.
- Hibrit MOV denenmedi.
- Windows. Linux derlenir ve çalışır (Ubuntu ve CachyOS'ta denendi; bkz. `docs/HANDOFF.md`, «Build and test on Linux» ve «Linux results»).

## Derleme

Derleme yönergeleri (araç zinciri ve adımlar) yalnızca [İngilizce README](../../README.md#build) içindedir.

## Lisans

OBS eklentileri için gerektiği üzere GPL-2.0-or-later (bkz. `LICENSE`). Üçüncü taraf kod:
- FFmpeg (LGPL), OBS ile birlikte geldiği haliyle.
- FilmCraft ProRes crate'i (MIT OR Apache-2.0, MIT altında kullanılır).
- AW+ (`src/prores-aw.c`): FFmpeg'in prores_aw kodlayıcısının, ileri DCT'sinin ve ProRes tablolarının bir portu (LGPL 2.1 veya sonrası; Anatoliy Wasserman, Konstantin Shishkov, Independent JPEG Group), GPL altında kullanılır.
- `oxideav-prores` ve `oxideav-core` (MIT), OxideAV ProRes kodlayıcısı.
