<!-- languages:start -->
<div align="center">

<details>
<summary><b>🌐 Language · Idioma · Langue · Sprache · Язык · 语言 · 言語 · 언어 · भाषा · لغة</b></summary>
<br>

<table>
  <tr>
    <td align="center"><a href="../../README.md">English</a></td>
    <td align="center"><a href="README.ar.md">العربية</a></td>
    <td align="center"><b>বাংলা</b></td>
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
    <td align="center"><a href="README.vi.md">Tiếng Việt</a></td>
  </tr>
</table>

</details>

</div>
<!-- languages:end -->

# StudioEncoder: OBS Studio-র জন্য এডিটিং-বান্ধব এনকোডার

> এই অনুবাদ একটি AI লিখেছে এবং মাতৃভাষী কেউ এটি যাচাই করেননি। OBS Studio-র মেনু, সেটিংস ও এনকোডারের নাম OBS যেভাবে দেখায় সেভাবেই ইংরেজিতে রাখা হয়েছে। নির্ভরযোগ্য পাঠ হলো [ইংরেজি README](../../README.md)।

Windows-এর জন্য একটি OBS Studio প্লাগইন, যা OBS Studio 33 ও তার পরের সংস্করণে 10-বিট all-intra এনকোডার যোগ করে, যাতে 4K রেকর্ডিং MOV-তে এমনভাবে হয় যা সহজে এডিট করা যায়।

- **AW+** হলো 10-বিট 4:2:2 রঙসহ ProRes-সামঞ্জস্যপূর্ণ একটি CPU এনকোডার। এটি FFmpeg-এর ProRes এনকোডারের (`prores_aw`) উন্নত সংস্করণ এবং একটি রাউন্ডিং বাগ ঠিক করে, যার কারণে ProRes Proxy মোডে ফুটেজ ঝাপসা হয়ে যেত। এটি গতির জন্যও অপ্টিমাইজ করা, তাই FFmpeg-এর এনকোডারের প্রায় অর্ধেক CPU ব্যবহার করে।
- **10-বিট 4:2:2 সহ GPU এনকোডার:** NVIDIA RTX 50 সিরিজ কার্ডের জন্য H.264, এবং Intel QuickSync-এর মাধ্যমে HEVC (Intel Panther Lake CPU-তে পরীক্ষিত)।
- **10-বিট 4:2:0 সহ GPU এনকোডার:** AMD GPU এবং RTX 50 সিরিজের চেয়ে পুরনো NVIDIA কার্ডের জন্য all-intra HEVC। এই GPU-গুলোর জন্য OBS-এ আগে থেকেই হার্ডওয়্যার HEVC এনকোডার আছে, কিন্তু StudioEncoder দুর্বোধ্য সেটিংসের বদলে সহজ প্রিসেট দিয়ে সেগুলো ব্যবহার করা সহজ করে। AMD GPU 4:2:2 এনকোড করতে পারে না, তাই এগুলো 4:2:0-ই থাকে।

OBS 33.0 থেকে Custom Output (FFmpeg) পথে একটি বাগ আছে, যা মাঝে মাঝে «non monotonically increasing dts» ত্রুটি ঘটায় এবং নতুন রেকর্ডিং শুরু হওয়া আটকে দিতে পারে। StudioEncoder এই পথ এড়িয়ে চলে: এর এনকোডারগুলো **Standard** রেকর্ডিং মোডে চলে, তাই মসৃণভাবে চলে।

> **Apple ProRes নয়।** এই প্লাগইন অনানুষ্ঠানিক এবং Apple-এর সঙ্গে যুক্ত নয় বা Apple-এর অনুমোদিতও নয়। Apple ProRes একটি লাইসেন্সপ্রাপ্ত, প্রত্যয়িত কোডেক। এখানকার এনকোডারগুলো প্রকাশ্যে নথিভুক্ত ফরম্যাটের (SMPTE RDD 36) স্বাধীন বাস্তবায়ন: FFmpeg-এর `prores_aw` (এবং তা থেকে তৈরি AW+ এনকোডার), FilmCraft Rust এনকোডার এবং OxideAV Rust এনকোডার। এদের আউটপুট DaVinci Resolve-এর মতো প্রচলিত এডিটরে চলে, কিন্তু Apple দ্বারা প্রত্যয়িত নয়।

## হার্ডওয়্যার

আপনি সত্যিই 10-বিট 4:2:2 রেকর্ড করছেন কি না তা ক্যাপচার কার্ড ঠিক করে, কারণ তাকে PC-তে 10-বিট 4:2:2 পৌঁছে দিতে হয়।

**কার্ড।** PC-তে 4K 10-বিট 4:2:2 ক্যাপচারের সবচেয়ে সস্তা উপায় হলো Blackmagic Design Intensity বা DeckLink কার্ড, অথবা Magewell Pro Capture কার্ড। AJA ও Datapath কার্ডও এটা করে, কিন্তু অনেক বেশি দামে।

**OBS-এ P210।** OBS তার নিজস্ব Video Capture Device সোর্সে P210 পিক্সেল ফরম্যাট (10-বিট 4:2:2) ক্যাপচার করতে পারে না; সেটি 10-বিট 4:2:0-তে থেমে যায়। Blackmagic কার্ড (v210) DeckLink সোর্সের মাধ্যমে OBS-এ আগেই কাজ করে। StudioEncoder Magewell কার্ড এবং P210 আউটপুট দেয় এমন যেকোনো অন্য কার্ডের সমর্থন যোগ করে, যা OBS-এ কখনো ছিল না। **SE: 10-bit 4:2:2 capture (P210)** সোর্সটি ব্যবহার করুন।

**PCIe কার্ড।** পূর্ণ ক্ষমতায় চলতে কার্ডের **x4 লেন** যুক্ত একটি স্লট দরকার। এটিকে M.2 সকেট বা SATA পোর্টের সঙ্গে লেন ভাগ করতে দেবেন না, কারণ তাতে স্লট x2 বা x1-এ নেমে যেতে পারে। আপনার মাদারবোর্ডের ম্যানুয়াল দেখুন।

**USB কার্ড।** 4K30 10-বিট 4:2:2-এর জন্য প্যাক করা v210 হিসেবে প্রায় 5.3 Gbit/s, বা P210 হিসেবে 8.0 Gbit/s লাগে। 5 Gbit/s USB 3.0 লিংক বাস্তবে মাত্র প্রায় 3.5 থেকে 4 Gbit/s বহন করে, তাই এটি পারে না। আপনার দরকার সত্যিকারের 10 Gbit/s USB 3.2 Gen 2 পোর্ট, উপযুক্ত কেবল এবং 10-বিট 4:2:2 আউটপুট দেয় এমন একটি কার্ড, আর 4K30 P210 কোনোমতে ধরবে। 4K60 কোনো USB লিংকেই ধরে না। বেশিরভাগ USB ক্যাপচার কার্ড শুধু 8-বিট 4:2:2 বা 10-বিট 4:2:0 দেয়, তাই কেনার আগে কার্ডের আউটপুট ফরম্যাট দেখে নিন।

## এনকোডার

| OBS-এ এনকোডার | বাস্তবায়ন | মন্তব্য |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: প্লাগইনে বসানো FFmpeg-এর `prores_aw`, সংশোধিত রাউন্ডিংসহ | **সুপারিশকৃত।** CPU-র ওপর হালকা: AVX2 সহ CPU-তে এর অংশ FFmpeg-এর নিজস্ব prores_aw-র প্রায় অর্ধেক, আর পুরনো বা নিম্নমানের CPU-তে এটি সাধারণ C-তে ফিরে যায় (তবুও FFmpeg-এর চেয়ে দ্রুত)। Proxy সূক্ষ্ম বিবরণ ধরে রাখে; FFmpeg-এর prores_aw Proxy-কে ঝাপসা করে। |
| ProRes-compatible (Rust / FilmCraft) (experimental) | [FilmCraft](https://github.com/storytold/filmcraft) Rust এনকোডার | পরীক্ষামূলক। 4K-তে ভারী CPU ব্যবহার। |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | পরীক্ষামূলক। 4K-তে prores_aw-র প্রায় 2.5 গুণ CPU (22-থ্রেডের ল্যাপটপে 12%-এর বিপরীতে 30%) এবং লাইভ ক্যামেরা ফুটেজে প্রোফাইল 422-এ প্রায় এক-তৃতীয়াংশ বড় ফাইল, প্রতি বিটে মানের কোনো সুবিধা নেই। খুবই নতুন কোড (0.1)। |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | GPU-তে ছোট ফাইল। **RTX 50 সিরিজ** কার্ড লাগে। নিচে দেখুন। |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | **RTX 50 সিরিজের চেয়ে পুরনো** NVIDIA কার্ডের জন্য (সেগুলোতে 10-বিট H.264 নেই): 10-বিট **4:2:0**, প্রতিটি ফ্রেম কিফ্রেম, একটি বিটরেট সেটিং। নিচে দেখুন। |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | Radeon GPU-তে 10-বিট **4:2:0**, প্রতিটি ফ্রেম কিফ্রেম, চারটি মানের প্রিসেটসহ। নিচে দেখুন। |

প্রোফাইল: Proxy (`apco`), LT (`apcs`), 422 (`apcn`), HQ (`apch`)।

### কোন এনকোডার দেখা যাবে তা বাছাই (Tools > StudioEncoder)

OBS-এর এনকোডার তালিকা ডিফল্টভাবে ছোট থাকে। **Tools > StudioEncoder** একটি উইন্ডো খোলে, যা এই প্লাগইনের প্রতিটি এনকোডারকে তিনটি অংশে তালিকাভুক্ত করে: **Recommended (10-bit 4:2:2)** (AW+ এবং RTX 50 H.264 এনকোডার), **10-bit 4:2:0** (NVIDIA ও AMD-র HEVC এনকোডার) এবং **Experimental** (FilmCraft ও OxideAV, ডিফল্টে বন্ধ)। শেষ দুটি গুটিয়ে ফেলা যায় এমন গ্রুপ, যা বন্ধ অবস্থায় শুরু হয়; একটি খুলতে ছোট তীরে ক্লিক করুন। যেগুলো চান সেগুলোতে টিক দিন এবং OK চাপুন; পরিবর্তন OBS পুনরায় চালু করার পর কার্যকর হয়।

<p align="center"><img src="../images/settings-window.png" alt="OBS-এ StudioEncoder সেটিংস উইন্ডো (Tools > StudioEncoder): চেকবক্সসহ এনকোডারের তালিকা এবং গুটিয়ে ফেলা যায় এমন 10-bit 4:2:0 ও Experimental গ্রুপ" width="420"></p>

বন্ধ করা এনকোডার শুধু ড্রপডাউন থেকে লুকানো থাকে। যে প্রোফাইল আগে থেকেই সেটি ব্যবহার করে তা কাজ করতে থাকে। এই প্লাগইনের সব এনকোডার OBS-এর এনকোডার তালিকায় «SE:» দিয়ে শুরু হয়, যাতে সহজে চোখে পড়ে।


### কেন AW+ (সংশোধিত prores_aw)

FFmpeg-এর prores_aw প্রতিটি বিবরণ সহগকে তার কোয়ান্টাইজার ধাপ দিয়ে ভাগ করে এবং **শূন্যের দিকে ছেঁটে ফেলে**, ফলে পূর্ণ এক ধাপের চেয়ে ছোট সবকিছু বাদ পড়ে। যেখানে ধাপ মোটা, সেই Proxy-তে এতে সূক্ষ্ম টেক্সচার মুছে যায়: মুখ ও কাপড় ঘষা-ঘষা দেখায়। Apple-এর এনকোডার একই Proxy ম্যাট্রিক্স ও কোয়ান্টাইজার ব্যবহার করে কিন্তু সেই বিবরণ ধরে রাখে। AW+ হলো প্লাগইনে বসানো prores_aw, যেখানে ছাঁটার বদলে রাউন্ডিং ব্যবহার করা হয় (প্রোফাইলভেদে অফসেট: Proxy 4/8, LT 3/8, 422 3/8, HQ 2/8 ধাপ, প্রতি বিটে সেরা মানের জন্য বাছাই করা)।

একটি 4K ক্যামেরা ক্লিপে মাপা (PSNR, বেশি হলে ভালো):

| প্রোফাইল | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s, 54.0 dB | **94 Mbit/s, 56.2 dB** | 96 Mbit/s, 56.5 dB |
| LT | 170 Mbit/s, 59.7 dB | 198 Mbit/s, 61.6 dB | 235 Mbit/s, 62.2 dB |
| HQ | 492 Mbit/s, 71.1 dB | 560 Mbit/s, 73.5 dB | 520 Mbit/s, 72.2 dB |

রাউন্ডিং বন্ধ থাকলে AW+ এমন ফাইল তৈরি করে যা FFmpeg-এর prores_aw-র ফাইলের সঙ্গে বাইটে বাইটে অভিন্ন (চারটি প্রোফাইলেই যাচাই করা)।
OBS শুরু হওয়ার সময় GPU এনকোডারগুলো আপনার গ্রাফিক্স কার্ড পরীক্ষা করে। কার্ড কোনোটি চালাতে না পারলে (H.264 এনকোডারের জন্য RTX 50 সিরিজের চেয়ে পুরনো NVIDIA কার্ড, NVIDIA HEVC এনকোডারের জন্য 10-বিট HEVC সহ NVIDIA কার্ড না থাকা, AMD এনকোডারের জন্য Radeon না থাকা), সেই এনকোডার তালিকা থেকে লুকিয়ে যায় এবং এই উইন্ডোতে ধূসর দেখায়।

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

ProRes-এর জায়গা বাঁচানো বিকল্প, GPU-তে এনকোড হয়। প্রতিটি ফ্রেম কিফ্রেম (all-intra), High 4:2:2 প্রোফাইল, 10-বিট, ধ্রুব বিটরেট, তাই এটি ProRes-এর মতো এডিট হয় কিন্তু মানে সামান্য ছাড় দিয়ে আকারে ছোট।

- **NVIDIA RTX 50-সিরিজ GPU** (Blackwell) এবং সাম্প্রতিক ড্রাইভার লাগে। পুরনো NVIDIA কার্ড 10-বিট H.264 বা 4:2:2 এনকোড করতে পারে না; সেগুলোতে রেকর্ডিং শুরু হবে না এবং OBS ত্রুটি দেখাবে। সেখানে ProRes (prores_aw) ব্যবহার করুন।
- **Bitrate**: 100 Mbit/s (ছোট, মান প্রায় ProRes LT-র মতো), **200 (ডিফল্ট**, ProRes LT-র চেয়ে ভালো, ProRes 422-এর প্রায় অর্ধেক আকার), 300 (উচ্চ)। 400-র ওপরে কিছু যোগ হয় না।
- **Speed**: Quality (p5, ডিফল্ট) বা Fast (p1, মান সামান্য কম)।
- ফ্রেম রূপান্তর ছাড়াই P216 হিসেবে GPU-তে যায়। একসঙ্গে সর্বোচ্চ 2টি ফ্রেম প্রক্রিয়ায় থাকে, কারণ সাধারণ ভোক্তা NVIDIA কার্ড এনকোডার সেশনের সংখ্যা সীমিত করে।
- RTX 5080-তে একটি FFmpeg পরীক্ষায় (4K 29.97 ক্যামেরা ফুটেজ) এটি p5-এ প্রায় 77 fps এবং p1-এ 101 fps-এ চলেছে।

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

**RTX 50 সিরিজের চেয়ে পুরনো** NVIDIA কার্ডের জন্য (GTX 10 সিরিজ ও নতুনগুলো চলার কথা; RTX 4070 Laptop-এ পরীক্ষিত)। সেই কার্ডগুলোতে 10-বিট H.264 মোড নেই, 4:2:2-ও নেই, তাই এটি **10-বিট 4:2:0** HEVC (Main 10): রঙের বিবরণ ProRes 422 যা ধরে রাখে তার অর্ধেক, কিন্তু প্রতিটি ফ্রেম কিফ্রেম, তাই এটি ProRes-এর মতো কাটা ও স্ক্রাব করা যায়, ছোট ফাইলে। RTX 50 কার্ডে আপনি উপরের H.264 4:2:2 এনকোডারও পাবেন, যা রঙের বিবরণ ধরে রাখে।

- **Settings > Advanced > Color Format: P010** (P216 নয়), Rec. 709, Limited। ছবি OBS যেভাবে তৈরি করে সেভাবেই, রূপান্তর ছাড়া, GPU-তে যায়।
- **Bitrate** (30 থেকে 400 Mbit/s, ডিফল্ট 150) এবং **Speed** (Quality p5, বা Fast p1)। একটি 4K ক্যামেরা ক্লিপে পরীক্ষায় (সোর্সের তুলনায় লুমা মান): 100 Mbit/s-এ 57.6 dB মাপা গেছে, ProRes Proxy-র (95-এ 53.7 dB) চেয়ে ভালো; 150-তে 59.4 dB, প্রায় AW+ LT-র (200-তে 59.6 dB) সমান, তার 70% বিট দিয়ে; 200-তে 60.8 dB। Fast প্রিসেট মানে শুধু সামান্য কম এবং সেই GPU-তে প্রায় সমান দ্রুত।
- রেকর্ডিং `hvc1` ট্যাগসহ MOV-এ HEVC Main 10 (সব ফ্রেম কিফ্রেম, FFmpeg-এ ত্রুটি ছাড়া ডিকোড হয়)। Resolve বা Premiere-এ এখনো চেষ্টা করা হয়নি।
- একসঙ্গে সর্বোচ্চ 2টি ফ্রেম এনকোড হয়, কারণ সাধারণ ভোক্তা NVIDIA কার্ড এনকোডার সেশনের সংখ্যা সীমিত করে। RTX 4070 Laptop-এ প্রতি 4K ফ্রেমে প্রায় 15 ms; OBS প্রায় 7% CPU ব্যবহার করেছে (তার প্রায় 5% সিনের নিজের)।

### HEVC 10-bit 4:2:0 Intra (AMD)

Radeon ব্যবহারকারীদের জন্য। AMD-র ভিডিও এনকোডার 4:2:2 একেবারেই করতে পারে না (আর তার H.264 এনকোডার 10-বিট করতে পারে না), তাই এটি **10-বিট 4:2:0**: রঙের বিবরণ ProRes 422 যা ধরে রাখে তার অর্ধেক। প্রতিটি ফ্রেম কিফ্রেম, তাই ProRes-এর মতোই প্রতিটি ফ্রেম প্রতিবেশী ফ্রেম ডিকোড না করেই কাটা ও স্ক্রাব করা যায়, অনেক ছোট ফাইলে (পরীক্ষার ক্লিপে High প্রায় ProRes 422-এর এক-তৃতীয়াংশ)। OBS-এর নিজস্ব «AMD HW H.265 (HEVC)»-ও 10-বিট রেকর্ড করতে পারে, কিন্তু তার জন্য স্বাভাবিক GOP এবং বিটরেট টিউনিং লাগে; এই এনকোডার তার জায়গায় একটিমাত্র বাছাই দেয়, **Quality**।

- **Settings > Advanced > Color Format: P010** (P216 নয়), Rec. 709, Limited।
- **Quality** ধ্রুব, তাই ফাইলের আকার ছবির ওপর নির্ভর করে। 4K 29.97 ক্যামেরা ফুটেজের জন্য সাধারণ আকার:

| প্রিসেট | আকার | 4K ক্যামেরা ক্লিপে লুমা PSNR |
|---|---|---|
| Small | প্রায় 30 Mbit/s | 52.4 dB |
| Standard | প্রায় 70 Mbit/s | 55.3 dB (ProRes LT: 169 Mbit/s-এ 56.2 dB) |
| **High** (ডিফল্ট) | প্রায় 120 Mbit/s | 56.7 dB |
| Maximum | প্রায় 170 Mbit/s | 57.8 dB (এনকোডারের সর্বোচ্চ সীমা) |

  PSNR-এর সংখ্যাগুলো 4:2:0-তে রূপান্তরিত ক্যামেরা ক্লিপের সঙ্গে তুলনা করা, তাই এই এনকোডারের জন্য ন্যায্য; ProRes 4:2:2 রঙ ধরে রাখে যা এটি পারে না। কোলাহলপূর্ণ ফুটেজ একই প্রিসেটে বড় ফাইল তৈরি করে।
- Radeon RX 9070 XT-তে মাপা: প্রতি 4K ফ্রেমে প্রায় 3.8 ms, 4K 29.97-এ কোনো পুনরাবৃত্ত ফ্রেম নেই। AMD এনকোডার প্রতিটি প্যাকেট এক ফ্রেম দেরিতে ফেরত দেয়, তাই Stop চাপার ঠিক আগের শেষ ফ্রেমটি বাদ যায়।
- রেকর্ডিং MOV-এ HEVC Main 10 (সব ফ্রেম কিফ্রেম, স্থির টাইমস্ট্যাম্প, FFmpeg-এ ত্রুটি ছাড়া ডিকোড হয়)। Resolve বা Premiere-এ এখনো চেষ্টা করা হয়নি।

## 10-বিট 4:2:2 ক্যাপচার (P210), Windows

OBS-এর নিজস্ব Video Capture Device সোর্স 10-বিট 4:2:2 ভেতরে আনতে পারে না: তার DirectShow ক্যাপচার P010-এ (10-বিট 4:2:0) থেমে যায়। কিছু ক্যাপচার কার্ড, যেমন Magewell Pro Capture, 10-বিট 4:2:2 P210 হিসেবে দেয়। StudioEncoder সেগুলোর জন্য একটি সোর্স যোগ করে: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**।

- Device-এর অধীনে কার্ড বাছুন, অথবা «First device that offers P210»-এ রেখে দিন।
- Resolution ও Frame rate ক্যামেরার HDMI সিগন্যালের সঙ্গে মেলান (ডিফল্ট 3840x2160, 29.97)।
- **Settings > Video > Advanced > Color Format-কে P216** করুন, যাতে 10 বিট এনকোডারে পৌঁছায়। P216-এর সঙ্গে OBS সিনকে 16-বিট ফ্লোটিং পয়েন্টে মেশায়।
- কার্ডের অডিও আলাদা একটি Audio Input Capture সোর্স দিয়ে যোগ করুন।

ফ্রেম OBS-এর জন্য P210 থেকে I210-তে রূপান্তরিত হয়: ক্ষতিহীন শিফট ও প্লেন আলাদা করা, Ryzen 9 9950X3D-তে প্রতি 4K ফ্রেমে প্রায় 2.4 ms। 3840x2160 29.97-এ একটি Magewell Pro Capture HDMI 4K দিয়ে পরীক্ষিত: কোনো পুনরাবৃত্ত ফ্রেম নেই, আর স্যাম্পলে প্রকৃত 10-বিট মান আছে। `tools/p210-probe.cpp` একটি ছোট কমান্ড-লাইন প্রোগ্রাম, যা দেখে কোনো কার্ডের P210 আউটপুটে প্রকৃত 10-বিট ডেটা আছে কি না।

## প্রয়োজনীয়তা

- Windows 10/11 x64
- OBS Studio 33.0.x (33.0.0-র বিপরীতে তৈরি; 33.0.1-এ পরীক্ষিত)

## ইনস্টল

OBS বন্ধ করুন, তারপর রিলিজ zip থেকে `obs-studioencoder` ফোল্ডারটি কপি করুন:

- **ইনস্টল করা OBS:** `C:\ProgramData\obs-studio\plugins\obs-studioencoder\`-এ। `ProgramData` লুকানো; File Explorer-এ পাথটি টাইপ করুন।
- **পোর্টেবল OBS:** `<OBS folder>\plugins\obs-studioencoder\`-এ, `bin`, `data` ও `config`-এর পাশে।

ফোল্ডারে `obs-studioencoder.dll` ও `data\locale\` থাকতে হবে। OBS-এ Tools → Plugin Manager-এ «StudioEncoder» দেখা যাওয়া উচিত, এবং Tools → StudioEncoder এর সেটিংস খোলে।

পুরনো `obs-plugins\64bit` বা `data\obs-plugins` ফোল্ডার ব্যবহার করবেন না। OBS 33 সেই জায়গাগুলোকে লেগেসি হিসেবে চিহ্নিত করে।

## সেটিংস

1. Settings → Advanced: **Color Format P216** (দুটি HEVC এনকোডারের জন্য **P010**), Color Space Rec. 709, Color Range Limited।

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings, Advanced ট্যাব: Colour Format P216 (16-bit, 4:2:2, 2 planes), Rec. 709, Limited, হাইলাইট করা" width="560">

2. Settings → Video: ক্যানভাস (Base) ও আউটপুট রেজোলিউশন (Output) আপনার রেকর্ডিংয়ের আকারে এবং ফ্রেম রেট আপনার সোর্সের সঙ্গে মিলিয়ে সেট করুন। স্ক্রিনশটে 29.97-এ 3840x2160 দেখানো হয়েছে।

   <img src="../images/obs-video.png" alt="OBS Settings, Video ট্যাব: Base ও Output রেজোলিউশন 3840x2160, 29.97 fps" width="560">

3. Settings → Output: Output Mode **Advanced**, Recording ট্যাব, Type **Standard**, Recording Format **MOV**।
4. Video Encoder: উপরের এনকোডারগুলোর একটি («SE:» দিয়ে শুরু)। Encoder Settings-এর নিচের লাইন, «Required video settings», এই এনকোডারের প্রয়োজনীয় Color Format, Color Space ও Color Range পুনরায় দেখায়।

   <img src="../images/obs-output-encoder.png" alt="OBS Settings, Output ট্যাব, Recording: Standard টাইপ, QuickTime (.mov), Video Encoder SE: ProRes (AW+, fast) হাইলাইট করা" width="560">

   এর সেটিংসে:
   - **Profile** (ProRes এনকোডার): Proxy, LT, 422 বা HQ। **Bitrate** ও **Speed** (NVENC এনকোডার): উপরে দেখুন।
   - **Frames in flight**: 0 (স্বয়ংক্রিয়, সুপারিশকৃত)। প্রয়োজন অনুযায়ী ফ্রেমগুলো সমান্তরালে এনকোড হয়, প্রতি CPU কোরে একটি।
   - **Automatic sizing** (শুধু Frames in flight 0 হলে): *Grow when frames are late* (ডিফল্ট) বা *Size from measured frame cost* (পরীক্ষামূলক)। দ্বিতীয়টি 4টি ওয়ার্কার দিয়ে শুরু করে, একটি ফ্রেমের সময় মাপে, তারপর প্রয়োজনীয় ওয়ার্কার সংখ্যা এক ধাপে ঠিক করে। এতে হালকা এনকোডারে কম মেমরি লাগে আর ভারীগুলোতে দ্রুত উঠে যায়। CPU লোড দুটোতেই এক।
   - **On overload**: *Warn in the log* বা *Stop recording*।
   - **Log verbosity**: Info সময়ের হিসাব ও স্বয়ংক্রিয় টিউনিং দেখায়।

## ওভারলোড

কোনো এনকোডার তাল মেলাতে না পারলে OBS টাইমলাইন ঠিক রাখতে আগের ফ্রেমটি পুনরাবৃত্তি করে, আর সেসব জায়গায় রেকর্ডিং আটকে থাকা দেখায়। প্লাগইন প্রতিটি রেকর্ডিংয়ের জন্য এই পুনরাবৃত্ত ফ্রেম গোনে (OBS পরপর দুবার একই ফ্রেম দিলে সে তা টের পায়):
- যে প্রতি সেকেন্ডে এটা ঘটে তার জন্য লগে একটি `OVERLOAD` সতর্কতা লেখে, এবং রেকর্ডিং শেষে একটি সারসংক্ষেপ।
- **On overload: Stop recording** দিলে, পরপর 3 সেকেন্ড ওভারলোডের পর রেকর্ডিং শেষ করে এবং ত্রুটি দেখায়।

ওভারলোড সতর্কতা দেখলে prores_aw এনকোডারে বা হালকা কোনো প্রোফাইলে যান।

## জানা সীমাবদ্ধতা

- ইনপুট P216 হতে হবে (HEVC এনকোডারের জন্য P010)। OBS 33 অন্য কোনো 10-বিট 4:2:2 আউটপুট ফরম্যাট দেয় না।
- Stop চাপার পরে ক্যাপচার হওয়া কিন্তু এখনো এনকোড হতে থাকা ফ্রেম বাদ দেওয়া হয়। এটি প্রত্যাশিত; রেকর্ডিং থামার বিন্দুতে শেষ হয়।
- হাইব্রিড MOV পরীক্ষা করা হয়নি।
- Windows। Linux-এ বিল্ড হয় ও চলে (Ubuntu ও CachyOS-এ পরীক্ষিত; `docs/HANDOFF.md`, «Build and test on Linux» ও «Linux results» দেখুন)।

## বিল্ড

বিল্ডের নির্দেশনা (টুলচেন ও ধাপ) শুধু [ইংরেজি README](../../README.md#build)-তে আছে।

## লাইসেন্স

GPL-2.0-or-later (`LICENSE` দেখুন), OBS প্লাগইনের জন্য যেমন প্রয়োজন। তৃতীয় পক্ষের কোড:
- FFmpeg (LGPL), OBS-এর সঙ্গে যেভাবে আসে সেভাবে।
- FilmCraft ProRes ক্রেট (MIT OR Apache-2.0, MIT-র অধীনে ব্যবহৃত)।
- AW+ (`src/prores-aw.c`): FFmpeg-এর prores_aw এনকোডার, ফরওয়ার্ড DCT ও ProRes টেবিলের একটি পোর্ট (LGPL 2.1 বা পরবর্তী; Anatoliy Wasserman, Konstantin Shishkov, Independent JPEG Group), GPL-এর অধীনে ব্যবহৃত।
- `oxideav-prores` ও `oxideav-core` (MIT), OxideAV ProRes এনকোডার।
