<!-- languages:start -->
<div align="center">

<details>
<summary><b>🌐 Language · Idioma · Langue · Sprache · Язык · 语言 · 言語 · 언어 · भाषा · لغة</b></summary>
<br>

<table>
  <tr>
    <td align="center"><a href="../../README.md">English</a></td>
    <td align="center"><b>العربية</b></td>
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
    <td align="center"><a href="README.vi.md">Tiếng Việt</a></td>
  </tr>
</table>

</details>

</div>
<!-- languages:end -->

# StudioEncoder: مشفّرات ملائمة للمونتاج لبرنامج OBS Studio

> كُتبت هذه الترجمة بواسطة الذكاء الاصطناعي ولم يراجعها ناطقون أصليون. أسماء القوائم والإعدادات والمشفّرات في OBS Studio مكتوبة بالإنجليزية كما يعرضها OBS. المرجع المعتمد هو [README الإنجليزي](../../README.md).

إضافة لبرنامج OBS Studio على نظام Windows تضيف مشفّرات 10 بت بإطارات داخلية كاملة (all-intra) إلى OBS Studio 33 وما بعده، لتسجيل 4K بصيغة MOV يسهل مونتاجه.

- **AW+** هو مشفّر يعمل على المعالج (CPU) ومتوافق مع ProRes، بألوان 10 بت 4:2:2. وهو نسخة محسّنة من مشفّر ProRes في FFmpeg (`prores_aw`) ويصلح خطأ تقريب كان يجعل الصورة ضبابية في وضع ProRes Proxy. كما أنه محسّن للسرعة، فيستهلك نحو نصف ما يستهلكه مشفّر FFmpeg من المعالج.
- **مشفّرات GPU بـ 10 بت 4:2:2:** H.264 لبطاقات NVIDIA من سلسلة RTX 50، وHEVC عبر Intel QuickSync (جُرّب على معالج Intel Panther Lake).
- **مشفّرات GPU بـ 10 بت 4:2:0:** HEVC بإطارات داخلية كاملة لمعالجات AMD الرسومية ولبطاقات NVIDIA الأقدم من سلسلة RTX 50. لدى OBS بالفعل مشفّرات HEVC عتادية لهذه المعالجات، لكن StudioEncoder يسهّل استخدامها بإعدادات مسبقة بسيطة بدل الإعدادات المعقدة. لا تستطيع معالجات AMD الرسومية ترميز 4:2:2، لذا تبقى هذه بنظام 4:2:0.

<p align="center"><img src="../images/settings-window.png" alt="نافذة إعدادات StudioEncoder (Tools > StudioEncoder) في OBS: قائمة مشفّرات بمربعات اختيار ومجموعتان قابلتان للطي هما 10-bit 4:2:0 وExperimental" width="420"></p>

منذ OBS 33.0، في مسار Custom Output (FFmpeg) خطأ يسبب أحيانًا الخطأ «non monotonically increasing dts» وقد يمنع تسجيلًا جديدًا من البدء. يتجنب StudioEncoder هذا المسار: تعمل مشفّراته في وضع التسجيل **Standard**، فتعمل بسلاسة.

> **ليس Apple ProRes.** هذه الإضافة غير رسمية وليست تابعة لشركة Apple ولا معتمدة منها. Apple ProRes ترميز مرخّص ومعتمد. المشفّرات هنا تطبيقات مستقلة للصيغة الموثقة علنًا (SMPTE RDD 36): `prores_aw` من FFmpeg (ومشفّر AW+ المبني عليه)، ومشفّر FilmCraft المكتوب بلغة Rust، ومشفّر OxideAV المكتوب بلغة Rust. يُشغَّل ناتجها في برامج المونتاج الشائعة مثل DaVinci Resolve، لكنه غير معتمد من Apple.

## العتاد

بطاقة الالتقاط هي التي تحدد ما إذا كنت تسجّل فعلًا بـ 10 بت 4:2:2، لأن عليها أن تسلّم 10 بت 4:2:2 إلى الحاسوب.

**البطاقات.** أرخص طريقة لالتقاط 4K بـ 10 بت 4:2:2 على حاسوب هي بطاقة Blackmagic Design Intensity أو DeckLink، أو بطاقة Magewell Pro Capture. بطاقات AJA وDatapath تفعل ذلك أيضًا، لكن بسعر أعلى بكثير.

**P210 في OBS.** لا يستطيع OBS التقاط تنسيق البكسل P210 (10 بت 4:2:2) في مصدره Video Capture Device، الذي يتوقف عند 10 بت 4:2:0. تعمل بطاقات Blackmagic (v210) في OBS أصلًا عبر مصدر DeckLink. يضيف StudioEncoder دعمًا لبطاقات Magewell وأي بطاقة أخرى تُخرج P210، وهو ما لم يكن في OBS من قبل. استخدم المصدر **SE: 10-bit 4:2:2 capture (P210)**.

**بطاقات PCIe.** تحتاج البطاقة إلى فتحة موصولة بـ **4 مسارات (x4)** لتعمل بكامل طاقتها. لا تتركها تتشارك المسارات مع مقبس M.2 أو منافذ SATA، فقد يخفض ذلك الفتحة إلى x2 أو x1. راجع دليل اللوحة الأم.

**بطاقات USB.** يحتاج 4K30 بـ 10 بت 4:2:2 إلى نحو 5.3 Gbit/s بصيغة v210 المعبّأة، أو 8.0 Gbit/s بصيغة P210. لا يحمل وصلة USB 3.0 بسرعة 5 Gbit/s عمليًا إلا نحو 3.5 إلى 4 Gbit/s، لذا لا تكفي لذلك. تحتاج إلى منفذ USB 3.2 Gen 2 حقيقي بسرعة 10 Gbit/s، وكابل مناسب، وبطاقة تُخرج 10 بت 4:2:2، وسيتسع 4K30 P210 بالكاد. لا يتسع 4K60 في أي وصلة USB. تقدم معظم بطاقات الالتقاط عبر USB 4:2:2 بـ 8 بت أو 4:2:0 بـ 10 بت فقط، فتحقق من صيغ إخراج البطاقة قبل الشراء.

## المشفّرات

| المشفّر في OBS | التنفيذ | ملاحظات |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: مشفّر `prores_aw` من FFmpeg مدمج في الإضافة، مع تقريب مصحَّح | **موصى به.** خفيف على المعالج: في المعالجات التي تدعم AVX2 تبلغ حصته نحو نصف حصة prores_aw في FFmpeg نفسه، ويعود إلى لغة C العادية (وما زال أسرع من FFmpeg) في المعالجات الأقدم أو الضعيفة. يحافظ Proxy على التفاصيل الدقيقة، أما prores_aw في FFmpeg فيجعل Proxy ضبابيًا. |
| ProRes-compatible (Rust / FilmCraft) (experimental) | مشفّر [FilmCraft](https://github.com/storytold/filmcraft) المكتوب بـ Rust | تجريبي. استهلاك مرتفع للمعالج في 4K. |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | تجريبي. نحو 2.5 ضعف استهلاك المعالج مقارنة بـ prores_aw في 4K (30% مقابل 12% على حاسوب محمول بـ 22 خيطًا) وملفات أكبر بنحو الثلث في الملف التعريفي 422 على لقطات كاميرا حية، دون أفضلية في الجودة لكل بت. شيفرة حديثة جدًا (0.1). |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | ملفات أصغر على GPU. يحتاج إلى بطاقة من **سلسلة RTX 50**. انظر أدناه. |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | لبطاقات NVIDIA **الأقدم من سلسلة RTX 50** (لا يتوفر H.264 بـ 10 بت فيها): **4:2:0** بـ 10 بت، كل إطار هو إطار مفتاحي، مع إعداد لمعدل البت. انظر أدناه. |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | **4:2:0** بـ 10 بت، كل إطار هو إطار مفتاحي، على معالج Radeon الرسومي، مع أربعة إعدادات جودة مسبقة. انظر أدناه. |

الملفات التعريفية: Proxy (`apco`)، LT (`apcs`)، 422 (`apcn`)، HQ (`apch`).

### اختيار المشفّرات التي تظهر (Tools > StudioEncoder)

تبقى قائمة المشفّرات في OBS قصيرة افتراضيًا. تفتح **Tools > StudioEncoder** نافذة تعرض كل مشفّرات هذه الإضافة في ثلاثة أقسام: **Recommended (10-bit 4:2:2)** (مشفّر AW+ ومشفّر H.264 لـ RTX 50)، و**10-bit 4:2:0** (مشفّرا HEVC لـ NVIDIA وAMD)، و**Experimental** (مشفّرا FilmCraft وOxideAV، وهما معطّلان افتراضيًا). القسمان الأخيران مجموعتان قابلتان للطي وتبدآن مغلقتين؛ انقر السهم الصغير لفتح إحداهما. ضع علامة على ما تريد واضغط OK؛ يسري التغيير بعد إعادة تشغيل OBS.

<p align="center"><img src="../images/settings-window.png" alt="نافذة إعدادات StudioEncoder (Tools > StudioEncoder) في OBS: قائمة مشفّرات بمربعات اختيار ومجموعتان قابلتان للطي هما 10-bit 4:2:0 وExperimental" width="420"></p>

المشفّر المعطَّل يُخفى من القائمة المنسدلة فقط. أما الملف التعريفي الذي يستخدمه بالفعل فيستمر في العمل. تبدأ كل مشفّرات هذه الإضافة بـ «SE:» في قائمة مشفّرات OBS ليسهل تمييزها.


### لماذا AW+ (نسخة مصحَّحة من prores_aw)

يقسم prores_aw في FFmpeg كل معامل تفصيلي على خطوة التكميم الخاصة به و**يقتطع نحو الصفر**، فيضيع كل ما هو أصغر من خطوة كاملة. في Proxy، حيث الخطوات خشنة، يزيل ذلك الملمس الدقيق: تبدو الوجوه والأقمشة ممسوحة. يستخدم مشفّر Apple مصفوفة Proxy والمكمِّم نفسيهما لكنه يحتفظ بهذه التفاصيل. AW+ هو prores_aw منقول إلى الإضافة مع تقريب بدل الاقتطاع (الإزاحة لكل ملف تعريفي: Proxy 4/8، LT 3/8، 422 3/8، HQ 2/8 من الخطوة، اختيرت لأفضل جودة لكل بت).

قيس على مقطع كاميرا 4K (PSNR، الأعلى أفضل):

| الملف التعريفي | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s، 54.0 dB | **94 Mbit/s، 56.2 dB** | 96 Mbit/s، 56.5 dB |
| LT | 170 Mbit/s، 59.7 dB | 198 Mbit/s، 61.6 dB | 235 Mbit/s، 62.2 dB |
| HQ | 492 Mbit/s، 71.1 dB | 560 Mbit/s، 73.5 dB | 520 Mbit/s، 72.2 dB |

عند إيقاف التقريب، ينتج AW+ ملفات مطابقة بايتًا ببايت لملفات prores_aw في FFmpeg (جرى التحقق على الملفات التعريفية الأربعة).
تفحص مشفّرات GPU بطاقتك الرسومية عند بدء OBS. إذا لم تستطع البطاقة تشغيل أحدها (بطاقة NVIDIA أقدم من سلسلة RTX 50 لمشفّر H.264، أو عدم وجود بطاقة NVIDIA تدعم HEVC بـ 10 بت لمشفّر NVIDIA HEVC، أو عدم وجود Radeon لمشفّر AMD)، يُخفى ذلك المشفّر من القائمة ويظهر باللون الرمادي في هذه النافذة.

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

بديل لـ ProRes يوفر المساحة، ويُرمَّز على GPU. كل إطار هو إطار مفتاحي (all-intra)، بالملف التعريفي High 4:2:2، بـ 10 بت، ومعدل بت ثابت، فيُمنتَج مثل ProRes لكنه أصغر مع خسارة طفيفة في الجودة.

- **يحتاج إلى معالج رسومي NVIDIA من سلسلة RTX 50** (Blackwell) وبرنامج تشغيل حديث. لا تستطيع بطاقات NVIDIA الأقدم ترميز H.264 بـ 10 بت أو 4:2:2؛ وعليها لا يبدأ التسجيل ويعرض OBS خطأً. استخدم ProRes (prores_aw) هناك.
- **Bitrate**: 100 Mbit/s (صغير، بجودة قريبة من ProRes LT)، **200 (الافتراضي**، أفضل من ProRes LT، وبنحو نصف حجم ProRes 422)، 300 (عالٍ). ما فوق 400 لا يضيف شيئًا.
- **Speed**: Quality (p5، الافتراضي) أو Fast (p1، جودة أقل قليلًا).
- تُرسل الإطارات إلى GPU بصيغة P216 دون تحويل. لا يزيد عدد الإطارات الجارية معًا عن 2، لأن بطاقات NVIDIA الاستهلاكية تحدّ من عدد جلسات المشفّر.
- في اختبار FFmpeg على RTX 5080 (لقطات كاميرا 4K 29.97) عمل بنحو 77 إطارًا في الثانية عند p5 و101 عند p1.

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

لبطاقات NVIDIA **الأقدم من سلسلة RTX 50** (يُفترض أن تعمل سلسلة GTX 10 وما بعدها؛ جُرّب على RTX 4070 Laptop). ليس في تلك البطاقات وضع H.264 بـ 10 بت ولا 4:2:2، لذا فهذا HEVC (Main 10) بنظام **4:2:0 بـ 10 بت**: تفاصيل الألوان نصف ما يحتفظ به ProRes 422، لكن كل إطار هو إطار مفتاحي فيُقص ويُتصفَّح مثل ProRes، بملفات أصغر. على بطاقة RTX 50 يتوفر لك أيضًا مشفّر H.264 4:2:2 المذكور أعلاه الذي يحتفظ بتفاصيل الألوان.

- **Settings > Advanced > Color Format: P010** (وليس P216)، Rec. 709، Limited. تُرسل الصورة إلى GPU كما ينتجها OBS دون تحويل.
- **Bitrate** (من 30 إلى 400 Mbit/s، الافتراضي 150) و**Speed** (Quality p5 أو Fast p1). في اختبار على مقطع كاميرا 4K واحد (جودة اللمعان مقابل المصدر): سجّل 100 Mbit/s القيمة 57.6 dB، وهي أفضل من ProRes Proxy (53.7 dB عند 95)؛ وسجّل 150 القيمة 59.4 dB، أي نحو جودة AW+ LT (59.6 dB عند 200) بـ 70% من بتاتها؛ وسجّل 200 القيمة 60.8 dB. الإعداد المسبق Fast أقل جودة بقدر طفيف فقط وبنفس السرعة تقريبًا على ذلك GPU.
- التسجيلات بصيغة HEVC Main 10 داخل MOV مع الوسم `hvc1` (كل الإطارات مفتاحية، وتُفك دون أخطاء في FFmpeg). لم تُجرَّب بعد في Resolve أو Premiere.
- يُرمَّز إطاران كحد أقصى في آن واحد، لأن بطاقات NVIDIA الاستهلاكية تحدّ من عدد جلسات المشفّر. نحو 15 ms لكل إطار 4K على RTX 4070 Laptop؛ واستخدم OBS نحو 7% من المعالج (نحو 5% منها للمشهد نفسه).

### HEVC 10-bit 4:2:0 Intra (AMD)

لمستخدمي Radeon. لا يستطيع مشفّر الفيديو في AMD ترميز 4:2:2 إطلاقًا (ولا يستطيع مشفّر H.264 فيه ترميز 10 بت)، لذا فهذا **4:2:0 بـ 10 بت**: تفاصيل الألوان نصف ما يحتفظ به ProRes 422. كل إطار هو إطار مفتاحي، فيمكن قص كل إطار وتصفحه دون فك الإطارات المجاورة، كما في ProRes، بملفات أصغر بكثير (High نحو ثلث ProRes 422 على مقطع الاختبار). يستطيع «AMD HW H.265 (HEVC)» في OBS نفسه التسجيل بـ 10 بت أيضًا، لكنه يحتاج إلى GOP عادي وضبط لمعدل البت؛ ويستبدل هذا المشفّر ذلك بخيار واحد هو **Quality**.

- **Settings > Advanced > Color Format: P010** (وليس P216)، Rec. 709، Limited.
- **Quality** ثابتة، فيعتمد حجم الملف على الصورة. الأحجام المعتادة للقطات كاميرا 4K 29.97:

| الإعداد المسبق | الحجم | PSNR للمعان على مقطع كاميرا 4K |
|---|---|---|
| Small | نحو 30 Mbit/s | 52.4 dB |
| Standard | نحو 70 Mbit/s | 55.3 dB (ProRes LT: 56.2 dB عند 169 Mbit/s) |
| **High** (الافتراضي) | نحو 120 Mbit/s | 56.7 dB |
| Maximum | نحو 170 Mbit/s | 57.8 dB (سقف المشفّر) |

  تقارن قيم PSNR بمقطع الكاميرا بعد تحويله إلى 4:2:0، فهي منصفة لهذا المشفّر؛ أما ProRes فيحتفظ بألوان 4:2:2 لا يستطيعها هذا المشفّر. اللقطات المليئة بالضجيج تعطي ملفات أكبر عند الإعداد نفسه.
- قيس على Radeon RX 9070 XT: نحو 3.8 ms لكل إطار 4K، دون إطارات مكررة عند 4K 29.97. يعيد مشفّر AMD كل حزمة متأخرة بإطار واحد، فيُسقَط آخر إطار قبل ضغطك على Stop.
- التسجيلات بصيغة HEVC Main 10 داخل MOV (كل الإطارات مفتاحية، وطوابع زمنية ثابتة، وتُفك دون أخطاء في FFmpeg). لم تُجرَّب بعد في Resolve أو Premiere.

## التقاط 10 بت 4:2:2 (P210)، Windows

لا يستطيع مصدر Video Capture Device في OBS إدخال 10 بت 4:2:2: فالتقاط DirectShow فيه يتوقف عند P010 (10 بت 4:2:0). تقدم بعض بطاقات الالتقاط، مثل Magewell Pro Capture، صيغة 10 بت 4:2:2 بصيغة P210. يضيف StudioEncoder مصدرًا لها: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**.

- اختر البطاقة تحت Device، أو اتركها على «First device that offers P210».
- اضبط Resolution وFrame rate لتطابق إشارة HDMI الصادرة من الكاميرا (الافتراضي 3840x2160، 29.97).
- اضبط **Settings > Video > Advanced > Color Format على P216** كي تصل البتات العشر إلى المشفّر. مع P216 يمزج OBS المشهد بالفاصلة العائمة 16 بت.
- أضف صوت البطاقة بمصدر Audio Input Capture منفصل.

تُحوَّل الإطارات من P210 إلى I210 من أجل OBS: إزاحة بلا فقدان وفصل للمستويات، بنحو 2.4 ms لكل إطار 4K على Ryzen 9 9950X3D. جُرّب مع Magewell Pro Capture HDMI 4K بدقة 3840x2160 29.97: دون إطارات مكررة، وتحمل العيّنات قيم 10 بت حقيقية. `tools/p210-probe.cpp` برنامج صغير لسطر الأوامر يتحقق مما إذا كان خرج P210 لبطاقة ما يحمل بيانات 10 بت حقيقية.

## المتطلبات

- Windows 10/11 x64
- OBS Studio 33.0.x (بُني مقابل 33.0.0؛ جُرّب على 33.0.1)

## التثبيت

أغلق OBS، ثم انسخ المجلد `obs-studioencoder` من ملف zip الخاص بالإصدار:

- **OBS المثبَّت:** إلى `C:\ProgramData\obs-studio\plugins\obs-studioencoder\`. المجلد `ProgramData` مخفي؛ اكتب المسار في مستكشف الملفات.
- **OBS المحمول:** إلى `<OBS folder>\plugins\obs-studioencoder\`، بجوار `bin` و`data` و`config`.

يجب أن يحتوي المجلد على `obs-studioencoder.dll` و`data\locale\`. في OBS يجب أن يعرض Tools → Plugin Manager الإضافة «StudioEncoder»، وأن يفتح Tools → StudioEncoder إعداداتها.

لا تستخدم المجلدين القديمين `obs-plugins\64bit` أو `data\obs-plugins`. يعدّ OBS 33 هذين الموضعين قديمين.

## الإعدادات

1. Settings → Advanced: **Color Format P216** (و**P010** لمشفّري HEVC)، Color Space Rec. 709، Color Range Limited.

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings، تبويب Advanced: Colour Format P216 (16-bit, 4:2:2, 2 planes)، Rec. 709، Limited، مظلَّلة" width="560">

2. Settings → Video: اضبط اللوحة (Base) ودقة الإخراج (Output) على حجم تسجيلك ومعدل الإطارات ليطابق مصدرك. تُظهر لقطة الشاشة 3840x2160 بمعدل 29.97.

   <img src="../images/obs-video.png" alt="OBS Settings، تبويب Video: دقة Base وOutput هي 3840x2160، 29.97 fps" width="560">

3. Settings → Output: Output Mode **Advanced**، تبويب Recording، Type **Standard**، Recording Format **MOV**.
4. Video Encoder: أحد المشفّرات أعلاه (يبدأ بـ «SE:»). يكرر السطر تحت Encoder Settings، «Required video settings»، ما يحتاجه هذا المشفّر من Color Format وColor Space وColor Range.

   <img src="../images/obs-output-encoder.png" alt="OBS Settings، تبويب Output، Recording: النوع Standard، QuickTime (.mov)، Video Encoder SE: ProRes (AW+, fast) مظلَّل" width="560">

   في إعداداته:
   - **Profile** (مشفّرات ProRes): Proxy أو LT أو 422 أو HQ. **Bitrate** و**Speed** (مشفّر NVENC): انظر أعلاه.
   - **Frames in flight**: 0 (تلقائي، موصى به). تُرمَّز الإطارات بالتوازي، إطار لكل نواة معالج، حسب الحاجة.
   - **Automatic sizing** (فقط عندما يكون Frames in flight صفرًا): *Grow when frames are late* (الافتراضي) أو *Size from measured frame cost* (تجريبي). يبدأ الثاني بـ 4 عمّال (workers)، ويقيس زمن إطار، ثم يضبط عدد العمّال اللازم دفعة واحدة. يستهلك ذلك ذاكرة أقل مع المشفّرات الخفيفة ويصل أسرع مع الثقيلة. حِمل المعالج واحد في الحالتين.
   - **On overload**: *Warn in the log* أو *Stop recording*.
   - **Log verbosity**: يعرض Info التوقيت والضبط التلقائي.

## الحِمل الزائد

إذا لم يستطع مشفّر مجاراة الإطارات، يكرر OBS الإطار السابق للحفاظ على المخطط الزمني، فيبدو التسجيل متجمدًا في تلك المواضع. تحسب الإضافة هذه الإطارات المكررة لكل تسجيل (تلاحظ ذلك عندما يمرر لها OBS الإطار نفسه مرتين متتاليتين):
- تكتب تحذير `OVERLOAD` في السجل لكل ثانية يحدث فيها ذلك، وملخصًا عند انتهاء التسجيل.
- مع **On overload: Stop recording** تنهي التسجيل بعد 3 ثوانٍ متتالية من الحِمل الزائد وتعرض خطأً.

إذا رأيت تحذيرات الحِمل الزائد، فانتقل إلى مشفّر prores_aw أو إلى ملف تعريفي أخف.

## القيود المعروفة

- يجب أن يكون الدخل P216 (وP010 لمشفّري HEVC). لا يقدم OBS 33 صيغة إخراج أخرى بـ 10 بت 4:2:2.
- تُهمل الإطارات التي التُقطت بعد ضغطك Stop لكنها ما زالت قيد الترميز. هذا متوقع؛ ينتهي التسجيل عند نقطة الإيقاف.
- لم يُختبر MOV الهجين.
- Windows. يُبنى Linux ويعمل (جُرّب على Ubuntu وCachyOS؛ انظر `docs/HANDOFF.md`، «Build and test on Linux» و«Linux results»).

## البناء

تعليمات البناء (سلسلة الأدوات والخطوات) موجودة فقط في [README الإنجليزي](../../README.md#build).

## الترخيص

GPL-2.0-or-later (انظر `LICENSE`)، كما هو مطلوب لإضافات OBS. شيفرة الأطراف الثالثة:
- FFmpeg (LGPL)، كما يُشحن مع OBS.
- حزمة FilmCraft ProRes (MIT OR Apache-2.0، تُستخدم بموجب MIT).
- AW+ (`src/prores-aw.c`): نقل لمشفّر prores_aw من FFmpeg، ولتحويل DCT الأمامي، وجداول ProRes (LGPL 2.1 أو أحدث؛ Anatoliy Wasserman وKonstantin Shishkov وIndependent JPEG Group)، تُستخدم بموجب GPL.
- `oxideav-prores` و`oxideav-core` (MIT)، مشفّر ProRes من OxideAV.
