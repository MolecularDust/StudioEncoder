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
    <td align="center"><b>हिन्दी</b></td>
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

# StudioEncoder: OBS Studio के लिए एडिटिंग-अनुकूल एन्कोडर

> यह अनुवाद एक AI ने लिखा है और इसे मूल भाषियों ने जाँचा नहीं है। OBS Studio के मेनू, सेटिंग और एन्कोडर के नाम वैसे ही अंग्रेज़ी में रखे गए हैं जैसे OBS उन्हें दिखाता है। मान्य पाठ [अंग्रेज़ी README](../../README.md) है।

Windows के लिए एक OBS Studio प्लगइन, जो OBS Studio 33 और उसके बाद के संस्करणों में 10-बिट all-intra एन्कोडर जोड़ता है, ताकि 4K रिकॉर्डिंग MOV में ऐसी हो जो आसानी से एडिट हो सके।

- **AW+** 10-बिट 4:2:2 रंग वाला, ProRes-संगत CPU एन्कोडर है। यह FFmpeg के ProRes एन्कोडर (`prores_aw`) का बेहतर संस्करण है और एक राउंडिंग बग ठीक करता है, जिससे ProRes Proxy मोड में फ़ुटेज धुँधला हो जाता था। इसे गति के लिए भी अनुकूलित किया गया है, इसलिए यह FFmpeg के एन्कोडर की लगभग आधी CPU इस्तेमाल करता है।
- **10-बिट 4:2:2 वाले GPU एन्कोडर:** NVIDIA RTX 50 सीरीज़ कार्ड के लिए H.264, और Intel QuickSync के ज़रिए HEVC (Intel Panther Lake CPU पर परखा गया)।
- **10-बिट 4:2:0 वाले GPU एन्कोडर:** AMD GPU और RTX 50 सीरीज़ से पुराने NVIDIA कार्ड के लिए all-intra HEVC। इन GPU के लिए OBS में पहले से हार्डवेयर HEVC एन्कोडर हैं, लेकिन StudioEncoder जटिल सेटिंग की जगह सरल प्रीसेट देकर इन्हें इस्तेमाल करना आसान बनाता है। AMD GPU 4:2:2 एन्कोड नहीं कर सकते, इसलिए ये 4:2:0 ही रहते हैं।

<p align="center"><img src="../images/settings-window.png" alt="OBS में StudioEncoder की सेटिंग विंडो (Tools > StudioEncoder): चेकबॉक्स वाली एन्कोडर सूची और समेटे जा सकने वाले 10-bit 4:2:0 और Experimental समूह" width="420"></p>

OBS 33.0 से, Custom Output (FFmpeg) रास्ते में एक बग है जो कभी-कभी «non monotonically increasing dts» त्रुटि पैदा करता है और नई रिकॉर्डिंग को शुरू होने से रोक सकता है। StudioEncoder इस रास्ते से बचता है: इसके एन्कोडर **Standard** रिकॉर्डिंग मोड में चलते हैं, इसलिए सहजता से चलते हैं।

> **Apple ProRes नहीं है।** यह प्लगइन अनौपचारिक है और Apple से संबद्ध या उसके द्वारा समर्थित नहीं है। Apple ProRes लाइसेंस वाला, प्रमाणित कोडेक है। यहाँ के एन्कोडर सार्वजनिक रूप से प्रलेखित फ़ॉर्मेट (SMPTE RDD 36) के स्वतंत्र कार्यान्वयन हैं: FFmpeg का `prores_aw` (और उससे बना AW+ एन्कोडर), FilmCraft Rust एन्कोडर और OxideAV Rust एन्कोडर। इनका आउटपुट DaVinci Resolve जैसे आम एडिटरों में चलता है, पर Apple द्वारा प्रमाणित नहीं है।

## हार्डवेयर

कैप्चर कार्ड तय करता है कि आप सच में 10-बिट 4:2:2 रिकॉर्ड कर रहे हैं या नहीं, क्योंकि उसे PC को 10-बिट 4:2:2 देना होता है।

**कार्ड।** PC पर 4K 10-बिट 4:2:2 कैप्चर करने का सबसे सस्ता तरीका Blackmagic Design Intensity या DeckLink कार्ड, या Magewell Pro Capture कार्ड है। AJA और Datapath कार्ड भी यह करते हैं, लेकिन बहुत महँगे हैं।

**OBS में P210।** OBS अपने Video Capture Device सोर्स में P210 पिक्सेल फ़ॉर्मेट (10-बिट 4:2:2) कैप्चर नहीं कर सकता, जो 10-बिट 4:2:0 पर रुक जाता है। Blackmagic कार्ड (v210) DeckLink सोर्स के ज़रिए OBS में पहले से चलते हैं। StudioEncoder Magewell कार्ड और P210 आउटपुट करने वाले किसी भी अन्य कार्ड का समर्थन जोड़ता है, जो OBS में कभी नहीं था। **SE: 10-bit 4:2:2 capture (P210)** सोर्स इस्तेमाल करें।

**PCIe कार्ड।** पूरी क्षमता से चलने के लिए कार्ड को **x4 लेन** वाले स्लॉट की ज़रूरत होती है। उसे M.2 सॉकेट या SATA पोर्ट के साथ लेन साझा न करने दें, इससे स्लॉट x2 या x1 हो सकता है। अपने मदरबोर्ड का मैन्युअल देखें।

**USB कार्ड।** 4K30 10-बिट 4:2:2 को पैक्ड v210 के रूप में लगभग 5.3 Gbit/s, या P210 के रूप में 8.0 Gbit/s चाहिए। 5 Gbit/s का USB 3.0 लिंक व्यवहार में केवल लगभग 3.5 से 4 Gbit/s ले जाता है, इसलिए यह नहीं कर सकता। आपको सचमुच का 10 Gbit/s USB 3.2 Gen 2 पोर्ट, उपयुक्त केबल और 10-बिट 4:2:2 आउटपुट करने वाला कार्ड चाहिए, और 4K30 P210 बस किसी तरह समाएगा। 4K60 किसी भी USB लिंक पर नहीं समाता। ज़्यादातर USB कैप्चर कार्ड केवल 8-बिट 4:2:2 या 10-बिट 4:2:0 देते हैं, इसलिए खरीदने से पहले कार्ड के आउटपुट फ़ॉर्मेट जाँच लें।

## एन्कोडर

| OBS में एन्कोडर | कार्यान्वयन | टिप्पणियाँ |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: प्लगइन में बनाया गया FFmpeg का `prores_aw`, सुधरी हुई राउंडिंग के साथ | **अनुशंसित।** CPU पर हल्का: AVX2 वाले CPU पर इसका हिस्सा FFmpeg के अपने prores_aw का लगभग आधा है, और पुराने या कमज़ोर CPU पर यह सादे C पर लौट आता है (फिर भी FFmpeg से तेज़)। Proxy बारीक विवरण बनाए रखता है; FFmpeg का prores_aw Proxy को धुँधला कर देता है। |
| ProRes-compatible (Rust / FilmCraft) (experimental) | [FilmCraft](https://github.com/storytold/filmcraft) Rust एन्कोडर | प्रायोगिक। 4K पर भारी CPU उपयोग। |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | प्रायोगिक। 4K पर prores_aw से लगभग 2.5 गुना CPU (22-थ्रेड लैपटॉप के 12% के मुक़ाबले 30%) और लाइव कैमरा फ़ुटेज पर प्रोफ़ाइल 422 में लगभग एक-तिहाई बड़ी फ़ाइलें, प्रति बिट गुणवत्ता में कोई बढ़त नहीं। बहुत नया कोड (0.1)। |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | GPU पर छोटी फ़ाइलें। **RTX 50 सीरीज़** कार्ड चाहिए। नीचे देखें। |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | **RTX 50 सीरीज़ से पुराने** NVIDIA कार्ड के लिए (उनमें 10-बिट H.264 नहीं है): 10-बिट **4:2:0**, हर फ़्रेम कीफ़्रेम, एक बिटरेट सेटिंग। नीचे देखें। |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | Radeon GPU पर 10-बिट **4:2:0**, हर फ़्रेम कीफ़्रेम, चार गुणवत्ता प्रीसेट के साथ। नीचे देखें। |

प्रोफ़ाइल: Proxy (`apco`), LT (`apcs`), 422 (`apcn`), HQ (`apch`)।

### कौन-से एन्कोडर दिखें, यह चुनना (Tools > StudioEncoder)

OBS की एन्कोडर सूची डिफ़ॉल्ट रूप से छोटी रहती है। **Tools > StudioEncoder** एक विंडो खोलता है जो इस प्लगइन के हर एन्कोडर को तीन खंडों में सूचीबद्ध करती है: **Recommended (10-bit 4:2:2)** (AW+ और RTX 50 H.264 एन्कोडर), **10-bit 4:2:0** (NVIDIA और AMD के HEVC एन्कोडर) और **Experimental** (FilmCraft और OxideAV, डिफ़ॉल्ट रूप से बंद)। आख़िरी दो समेटे जा सकने वाले समूह हैं जो बंद अवस्था में शुरू होते हैं; किसी को खोलने के लिए छोटे तीर पर क्लिक करें। जो चाहें उन पर टिक करें और OK दबाएँ; बदलाव OBS दोबारा शुरू करने के बाद लागू होता है।

<p align="center"><img src="../images/settings-window.png" alt="OBS में StudioEncoder की सेटिंग विंडो (Tools > StudioEncoder): चेकबॉक्स वाली एन्कोडर सूची और समेटे जा सकने वाले 10-bit 4:2:0 और Experimental समूह" width="420"></p>

बंद किया गया एन्कोडर केवल ड्रॉपडाउन से छिपता है। जो प्रोफ़ाइल पहले से उसका उपयोग करती है वह चलती रहती है। इस प्लगइन के सभी एन्कोडर OBS की एन्कोडर सूची में «SE:» से शुरू होते हैं ताकि आसानी से पहचाने जा सकें।


### AW+ क्यों (सुधरा हुआ prores_aw)

FFmpeg का prores_aw हर विवरण गुणांक को उसके क्वांटाइज़र स्टेप से भाग देता है और **शून्य की ओर काट देता है**, इसलिए पूरे स्टेप से छोटी हर चीज़ फेंक दी जाती है। Proxy में, जहाँ स्टेप मोटे होते हैं, इससे बारीक बनावट मिट जाती है: चेहरे और कपड़े धुँधले दिखते हैं। Apple का एन्कोडर वही Proxy मैट्रिक्स और क्वांटाइज़र इस्तेमाल करता है पर वह विवरण बचा रखता है। AW+ वही prores_aw है जिसे प्लगइन में लाकर काटने की जगह राउंडिंग लगाई गई है (हर प्रोफ़ाइल का ऑफ़सेट: Proxy 4/8, LT 3/8, 422 3/8, HQ 2/8 स्टेप, प्रति बिट सर्वश्रेष्ठ गुणवत्ता के लिए चुना गया)।

एक 4K कैमरा क्लिप पर मापा गया (PSNR, जितना अधिक उतना बेहतर):

| प्रोफ़ाइल | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s, 54.0 dB | **94 Mbit/s, 56.2 dB** | 96 Mbit/s, 56.5 dB |
| LT | 170 Mbit/s, 59.7 dB | 198 Mbit/s, 61.6 dB | 235 Mbit/s, 62.2 dB |
| HQ | 492 Mbit/s, 71.1 dB | 560 Mbit/s, 73.5 dB | 520 Mbit/s, 72.2 dB |

राउंडिंग बंद होने पर AW+ ऐसी फ़ाइलें बनाता है जो FFmpeg के prores_aw की फ़ाइलों से बाइट-दर-बाइट एक जैसी होती हैं (चारों प्रोफ़ाइल पर जाँचा गया)।
OBS शुरू होते समय GPU एन्कोडर आपके ग्राफ़िक्स कार्ड को जाँचते हैं। अगर कार्ड किसी एन्कोडर को नहीं चला सकता (H.264 एन्कोडर के लिए RTX 50 सीरीज़ से पुराना NVIDIA कार्ड, NVIDIA HEVC एन्कोडर के लिए 10-बिट HEVC वाला NVIDIA कार्ड न होना, AMD वाले के लिए Radeon न होना), तो वह एन्कोडर सूची से छिप जाता है और इस विंडो में धूसर दिखता है।

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

ProRes का जगह बचाने वाला विकल्प, जो GPU पर एन्कोड होता है। हर फ़्रेम कीफ़्रेम है (all-intra), High 4:2:2 प्रोफ़ाइल, 10-बिट, स्थिर बिटरेट, इसलिए यह ProRes की तरह एडिट होता है पर गुणवत्ता में थोड़ी कमी के बदले छोटा होता है।

- **NVIDIA RTX 50-सीरीज़ GPU** (Blackwell) और हालिया ड्राइवर चाहिए। पुराने NVIDIA कार्ड 10-बिट H.264 या 4:2:2 एन्कोड नहीं कर सकते; उन पर रिकॉर्डिंग शुरू नहीं होगी और OBS त्रुटि दिखाएगा। वहाँ ProRes (prores_aw) इस्तेमाल करें।
- **Bitrate**: 100 Mbit/s (छोटा, लगभग ProRes LT जैसी गुणवत्ता), **200 (डिफ़ॉल्ट**, ProRes LT से बेहतर, ProRes 422 का लगभग आधा आकार), 300 (उच्च)। 400 से ऊपर कुछ नहीं जुड़ता।
- **Speed**: Quality (p5, डिफ़ॉल्ट) या Fast (p1, गुणवत्ता थोड़ी कम)।
- फ़्रेम बिना रूपांतरण के P216 के रूप में GPU को जाते हैं। एक साथ अधिकतम 2 फ़्रेम प्रक्रिया में रहते हैं, क्योंकि उपभोक्ता NVIDIA कार्ड एन्कोडर सत्रों की संख्या सीमित करते हैं।
- RTX 5080 पर एक FFmpeg परीक्षण में (4K 29.97 कैमरा फ़ुटेज) यह p5 पर लगभग 77 fps और p1 पर 101 fps पर चला।

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

**RTX 50 सीरीज़ से पुराने** NVIDIA कार्ड के लिए (GTX 10 सीरीज़ और नए चलने चाहिए; RTX 4070 Laptop पर परखा गया)। उन कार्डों में न 10-बिट H.264 मोड है न 4:2:2, इसलिए यह **10-बिट 4:2:0** HEVC (Main 10) है: रंग का विवरण उसका आधा है जो ProRes 422 रखता है, लेकिन हर फ़्रेम कीफ़्रेम है, इसलिए यह ProRes की तरह कटता और स्क्रब होता है, छोटी फ़ाइलों में। RTX 50 कार्ड पर आपको ऊपर वाला H.264 4:2:2 एन्कोडर भी मिलता है, जो रंग का विवरण बनाए रखता है।

- **Settings > Advanced > Color Format: P010** (P216 नहीं), Rec. 709, Limited। तस्वीर वैसी ही GPU को जाती है जैसी OBS बनाता है, बिना रूपांतरण के।
- **Bitrate** (30 से 400 Mbit/s, डिफ़ॉल्ट 150) और **Speed** (Quality p5, या Fast p1)। एक 4K कैमरा क्लिप पर परीक्षण में (स्रोत के मुक़ाबले लूमा गुणवत्ता): 100 Mbit/s पर 57.6 dB मापा गया, ProRes Proxy (95 पर 53.7 dB) से बेहतर; 150 पर 59.4 dB, लगभग AW+ LT (200 पर 59.6 dB) जितना, उसके 70% बिट के साथ; 200 पर 60.8 dB। Fast प्रीसेट गुणवत्ता में केवल थोड़ा कम है और उस GPU पर लगभग उतना ही तेज़।
- रिकॉर्डिंग `hvc1` टैग वाले MOV में HEVC Main 10 हैं (सारे फ़्रेम कीफ़्रेम, FFmpeg में बिना त्रुटि डिकोड होते हैं)। अभी Resolve या Premiere में नहीं आज़माया गया।
- एक साथ अधिकतम 2 फ़्रेम एन्कोड होते हैं, क्योंकि उपभोक्ता NVIDIA कार्ड एन्कोडर सत्रों की संख्या सीमित करते हैं। RTX 4070 Laptop पर प्रति 4K फ़्रेम लगभग 15 ms; OBS ने CPU का लगभग 7% इस्तेमाल किया (उसमें से लगभग 5% सीन का अपना है)।

### HEVC 10-bit 4:2:0 Intra (AMD)

Radeon उपयोगकर्ताओं के लिए। AMD का वीडियो एन्कोडर 4:2:2 बिल्कुल नहीं कर सकता (और उसका H.264 एन्कोडर 10-बिट नहीं कर सकता), इसलिए यह **10-बिट 4:2:0** है: रंग का विवरण उसका आधा है जो ProRes 422 रखता है। हर फ़्रेम कीफ़्रेम है, इसलिए ProRes की तरह हर फ़्रेम को पड़ोसी फ़्रेम डिकोड किए बिना काटा और स्क्रब किया जा सकता है, बहुत छोटी फ़ाइलों में (परीक्षण क्लिप पर High लगभग ProRes 422 का एक-तिहाई है)। OBS का अपना «AMD HW H.265 (HEVC)» भी 10-बिट रिकॉर्ड कर सकता है, लेकिन उसे सामान्य GOP और बिटरेट ट्यूनिंग चाहिए; यह एन्कोडर उसे एक ही विकल्प, **Quality**, से बदल देता है।

- **Settings > Advanced > Color Format: P010** (P216 नहीं), Rec. 709, Limited।
- **Quality** स्थिर है, इसलिए फ़ाइल का आकार तस्वीर पर निर्भर करता है। 4K 29.97 कैमरा फ़ुटेज के लिए सामान्य आकार:

| प्रीसेट | आकार | 4K कैमरा क्लिप पर लूमा PSNR |
|---|---|---|
| Small | लगभग 30 Mbit/s | 52.4 dB |
| Standard | लगभग 70 Mbit/s | 55.3 dB (ProRes LT: 169 Mbit/s पर 56.2 dB) |
| **High** (डिफ़ॉल्ट) | लगभग 120 Mbit/s | 56.7 dB |
| Maximum | लगभग 170 Mbit/s | 57.8 dB (एन्कोडर की सीमा) |

  PSNR के आँकड़े 4:2:0 में बदले गए कैमरा क्लिप से तुलना करते हैं, इसलिए इस एन्कोडर के लिए निष्पक्ष हैं; ProRes 4:2:2 रंग रखता है जो यह नहीं रख सकता। शोर वाला फ़ुटेज उसी प्रीसेट पर बड़ी फ़ाइलें बनाता है।
- Radeon RX 9070 XT पर मापा गया: प्रति 4K फ़्रेम लगभग 3.8 ms, 4K 29.97 पर कोई दोहराया फ़्रेम नहीं। AMD एन्कोडर हर पैकेट एक फ़्रेम देर से लौटाता है, इसलिए Stop दबाने से ठीक पहले का आख़िरी फ़्रेम छोड़ दिया जाता है।
- रिकॉर्डिंग MOV में HEVC Main 10 हैं (सारे फ़्रेम कीफ़्रेम, स्थिर टाइमस्टैम्प, FFmpeg में बिना त्रुटि डिकोड होते हैं)। अभी Resolve या Premiere में नहीं आज़माया गया।

## 10-बिट 4:2:2 कैप्चर (P210), Windows

OBS का अपना Video Capture Device सोर्स 10-बिट 4:2:2 अंदर नहीं ला सकता: उसका DirectShow कैप्चर P010 (10-बिट 4:2:0) पर रुक जाता है। कुछ कैप्चर कार्ड, जैसे Magewell Pro Capture, 10-बिट 4:2:2 को P210 के रूप में देते हैं। StudioEncoder उनके लिए एक सोर्स जोड़ता है: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**।

- Device के अंतर्गत कार्ड चुनें, या «First device that offers P210» पर रहने दें।
- Resolution और Frame rate को कैमरे के HDMI सिग्नल से मिलाएँ (डिफ़ॉल्ट 3840x2160, 29.97)।
- **Settings > Video > Advanced > Color Format को P216** पर सेट करें ताकि 10 बिट एन्कोडर तक पहुँचें। P216 के साथ OBS सीन को 16-बिट फ़्लोटिंग पॉइंट में मिलाता है।
- कार्ड का ऑडियो एक अलग Audio Input Capture सोर्स से जोड़ें।

फ़्रेम OBS के लिए P210 से I210 में बदले जाते हैं: बिना हानि के शिफ़्ट और प्लेन का बँटवारा, Ryzen 9 9950X3D पर प्रति 4K फ़्रेम लगभग 2.4 ms। Magewell Pro Capture HDMI 4K के साथ 3840x2160 29.97 पर परखा गया: कोई दोहराया फ़्रेम नहीं, और नमूनों में असली 10-बिट मान हैं। `tools/p210-probe.cpp` एक छोटा कमांड-लाइन प्रोग्राम है जो जाँचता है कि किसी कार्ड के P210 आउटपुट में असली 10-बिट डेटा है या नहीं।

## आवश्यकताएँ

- Windows 10/11 x64
- OBS Studio 33.0.x (33.0.0 के विरुद्ध बना; 33.0.1 पर परखा गया)

## इंस्टॉल

OBS बंद करें, फिर रिलीज़ zip से `obs-studioencoder` फ़ोल्डर कॉपी करें:

- **इंस्टॉल किया हुआ OBS:** `C:\ProgramData\obs-studio\plugins\obs-studioencoder\` में। `ProgramData` छिपा रहता है; पथ File Explorer में टाइप करें।
- **पोर्टेबल OBS:** `<OBS folder>\plugins\obs-studioencoder\` में, `bin`, `data` और `config` के बगल में।

फ़ोल्डर में `obs-studioencoder.dll` और `data\locale\` होने चाहिए। OBS में Tools → Plugin Manager में «StudioEncoder» दिखना चाहिए, और Tools → StudioEncoder इसकी सेटिंग खोलता है।

पुराने `obs-plugins\64bit` या `data\obs-plugins` फ़ोल्डर इस्तेमाल न करें। OBS 33 इन जगहों को लेगेसी चिह्नित करता है।

## सेटिंग

1. Settings → Advanced: **Color Format P216** (दोनों HEVC एन्कोडर के लिए **P010**), Color Space Rec. 709, Color Range Limited।

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings, Advanced टैब: Colour Format P216 (16-bit, 4:2:2, 2 planes), Rec. 709, Limited, हाइलाइट किया हुआ" width="560">

2. Settings → Video: कैनवास (Base) और आउटपुट रिज़ॉल्यूशन (Output) को अपनी रिकॉर्डिंग के आकार पर और फ़्रेम रेट को अपने सोर्स से मिलाकर सेट करें। स्क्रीनशॉट में 29.97 पर 3840x2160 दिख रहा है।

   <img src="../images/obs-video.png" alt="OBS Settings, Video टैब: Base और Output रिज़ॉल्यूशन 3840x2160, 29.97 fps" width="560">

3. Settings → Output: Output Mode **Advanced**, Recording टैब, Type **Standard**, Recording Format **MOV**।
4. Video Encoder: ऊपर के एन्कोडरों में से एक («SE:» से शुरू होता है)। Encoder Settings के नीचे की पंक्ति, «Required video settings», वह Color Format, Color Space और Color Range दोहराती है जो इस एन्कोडर को चाहिए।

   <img src="../images/obs-output-encoder.png" alt="OBS Settings, Output टैब, Recording: Standard टाइप, QuickTime (.mov), Video Encoder SE: ProRes (AW+, fast) हाइलाइट किया हुआ" width="560">

   इसकी सेटिंग में:
   - **Profile** (ProRes एन्कोडर): Proxy, LT, 422 या HQ। **Bitrate** और **Speed** (NVENC एन्कोडर): ऊपर देखें।
   - **Frames in flight**: 0 (स्वचालित, अनुशंसित)। फ़्रेम ज़रूरत के अनुसार समानांतर एन्कोड होते हैं, प्रति CPU कोर एक।
   - **Automatic sizing** (केवल Frames in flight 0 होने पर): *Grow when frames are late* (डिफ़ॉल्ट) या *Size from measured frame cost* (प्रायोगिक)। दूसरा 4 वर्कर से शुरू होता है, एक फ़्रेम का समय नापता है, फिर ज़रूरी वर्करों की संख्या एक ही चरण में तय कर देता है। इससे हल्के एन्कोडरों में कम मेमोरी लगती है और भारी में तेज़ी से ऊपर पहुँचता है। CPU लोड दोनों में समान है।
   - **On overload**: *Warn in the log* या *Stop recording*।
   - **Log verbosity**: Info समय-मापन और स्वचालित ट्यूनिंग दिखाता है।

## ओवरलोड

अगर कोई एन्कोडर साथ नहीं दे पाता, तो OBS टाइमलाइन बनाए रखने के लिए पिछला फ़्रेम दोहराता है, और रिकॉर्डिंग उन जगहों पर जमी हुई दिखती है। प्लगइन हर रिकॉर्डिंग के लिए इन दोहराए गए फ़्रेमों को गिनता है (वह तब पहचान लेता है जब OBS उसे लगातार दो बार वही फ़्रेम देता है):
- जिस भी सेकंड में ऐसा होता है, उसके लिए यह लॉग में `OVERLOAD` चेतावनी लिखता है, और रिकॉर्डिंग ख़त्म होने पर एक सारांश।
- **On overload: Stop recording** के साथ, यह लगातार 3 ओवरलोड सेकंड के बाद रिकॉर्डिंग समाप्त कर देता है और त्रुटि दिखाता है।

अगर आपको ओवरलोड चेतावनियाँ दिखें, तो prores_aw एन्कोडर या किसी हल्की प्रोफ़ाइल पर जाएँ।

## ज्ञात सीमाएँ

- इनपुट P216 होना चाहिए (HEVC एन्कोडर के लिए P010)। OBS 33 कोई और 10-बिट 4:2:2 आउटपुट फ़ॉर्मेट नहीं देता।
- Stop दबाने के बाद कैप्चर हुए, पर अभी एन्कोड हो रहे फ़्रेम छोड़ दिए जाते हैं। यह अपेक्षित है; रिकॉर्डिंग रोकने के बिंदु पर ख़त्म होती है।
- हाइब्रिड MOV परखा नहीं गया।
- Windows। Linux बनता और चलता है (Ubuntu और CachyOS पर परखा गया; `docs/HANDOFF.md`, «Build and test on Linux» और «Linux results» देखें)।

## बिल्ड

बिल्ड निर्देश (टूलचेन और चरण) केवल [अंग्रेज़ी README](../../README.md#build) में हैं।

## लाइसेंस

GPL-2.0-or-later (`LICENSE` देखें), जैसा OBS प्लगइन के लिए आवश्यक है। तृतीय-पक्ष कोड:
- FFmpeg (LGPL), जैसा OBS के साथ आता है।
- FilmCraft ProRes क्रेट (MIT OR Apache-2.0, MIT के अंतर्गत उपयोग किया गया)।
- AW+ (`src/prores-aw.c`): FFmpeg के prores_aw एन्कोडर, फ़ॉरवर्ड DCT और ProRes तालिकाओं का पोर्ट (LGPL 2.1 या बाद का; Anatoliy Wasserman, Konstantin Shishkov, Independent JPEG Group), GPL के अंतर्गत उपयोग किया गया।
- `oxideav-prores` और `oxideav-core` (MIT), OxideAV ProRes एन्कोडर।
