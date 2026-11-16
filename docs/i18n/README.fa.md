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
    <td align="center"><b>فارسی</b></td>
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

# StudioEncoder: انکودرهای مناسب تدوین برای OBS Studio

> این ترجمه را یک هوش مصنوعی نوشته و افراد بومی‌زبان آن را بررسی نکرده‌اند. نام منوها، تنظیمات و انکودرهای OBS Studio به همان صورتی که OBS نشان می‌دهد، به انگلیسی آمده است. متن مرجع [README انگلیسی](../../README.md) است.

یک افزونهٔ OBS Studio برای ویندوز که به OBS Studio نسخهٔ ۳۳ و بالاتر انکودرهای ۱۰ بیتی all-intra اضافه می‌کند، برای ضبط 4K در قالب MOV که تدوین روانی دارد.

- **AW+** یک انکودر CPU سازگار با ProRes با رنگ ۱۰ بیتی 4:2:2 است. نسخهٔ بهبودیافتهٔ انکودر ProRes در FFmpeg (`prores_aw`) است و یک خطای گرد کردن را که در حالت ProRes Proxy تصویر را محو می‌کرد برطرف می‌کند. برای سرعت هم بهینه شده است و تقریباً نصف CPU انکودر FFmpeg را مصرف می‌کند.
- **انکودرهای GPU با ۱۰ بیت 4:2:2:** H.264 برای کارت‌های NVIDIA سری RTX 50، و HEVC از طریق Intel QuickSync (روی یک CPU از نوع Intel Panther Lake آزموده شده).
- **انکودرهای GPU با ۱۰ بیت 4:2:0:** HEVC با all-intra برای GPUهای AMD و برای کارت‌های NVIDIA قدیمی‌تر از سری RTX 50. OBS برای این GPUها از قبل انکودرهای سخت‌افزاری HEVC دارد، اما StudioEncoder با پیش‌تنظیم‌های ساده به‌جای تنظیمات پیچیده استفاده از آن‌ها را آسان‌تر می‌کند. GPUهای AMD نمی‌توانند 4:2:2 را انکود کنند، پس این‌ها 4:2:0 می‌مانند.

<p align="center"><img src="../images/settings-window.png" alt="پنجرهٔ تنظیمات StudioEncoder (Tools > StudioEncoder) در OBS: فهرستی از انکودرها با چک‌باکس و گروه‌های جمع‌شدنی 10-bit 4:2:0 و Experimental" width="420"></p>

از OBS 33.0 به بعد، مسیر Custom Output (FFmpeg) خطایی دارد که گاه‌گاه باعث خطای «non monotonically increasing dts» می‌شود و می‌تواند جلوی شروع یک ضبط جدید را بگیرد. StudioEncoder از این مسیر دوری می‌کند: انکودرهایش در حالت ضبط **Standard** اجرا می‌شوند و بدون مشکل کار می‌کنند.

> **Apple ProRes نیست.** این افزونه غیررسمی است و وابسته به Apple نیست و Apple آن را تأیید نکرده است. Apple ProRes یک کدک دارای مجوز و گواهی‌شده است. انکودرهای اینجا پیاده‌سازی‌های مستقل قالب مستندشدهٔ عمومی (SMPTE RDD 36) هستند: `prores_aw` از FFmpeg (و انکودر AW+ که بر پایهٔ آن ساخته شده)، انکودر Rust به نام FilmCraft و انکودر Rust به نام OxideAV. خروجی آن‌ها در تدوین‌گرهای رایج مانند DaVinci Resolve پخش می‌شود، اما Apple آن را گواهی نکرده است.

## سخت‌افزار

کارت کپچر تعیین می‌کند که آیا واقعاً با ۱۰ بیت 4:2:2 ضبط می‌کنید یا نه، چون باید ۱۰ بیت 4:2:2 را به رایانه تحویل دهد.

**کارت‌ها.** ارزان‌ترین راه برای کپچر 4K با ۱۰ بیت 4:2:2 روی رایانه، کارت Blackmagic Design Intensity یا DeckLink یا کارت Magewell Pro Capture است. کارت‌های AJA و Datapath هم این کار را می‌کنند، اما خیلی گران‌ترند.

**P210 در OBS.** OBS نمی‌تواند قالب پیکسل P210 (۱۰ بیت 4:2:2) را در منبع Video Capture Device خودش کپچر کند؛ این منبع به ۱۰ بیت 4:2:0 ختم می‌شود. کارت‌های Blackmagic (v210) از قبل از طریق منبع DeckLink در OBS کار می‌کنند. StudioEncoder پشتیبانی از کارت‌های Magewell و هر کارت دیگری را که P210 خروجی می‌دهد اضافه می‌کند، چیزی که OBS هرگز نداشته است. از منبع **SE: 10-bit 4:2:2 capture (P210)** استفاده کنید.

**کارت‌های PCIe.** کارت برای کار با تمام ظرفیت به اسلاتی با **۴ لاین (x4)** نیاز دارد. نگذارید لاین‌هایش را با سوکت M.2 یا درگاه‌های SATA به اشتراک بگذارد، چون ممکن است اسلات را به x2 یا x1 کاهش دهد. دفترچهٔ مادربرد را ببینید.

**کارت‌های USB.** 4K30 با ۱۰ بیت 4:2:2 به حدود ۵٫۳ Gbit/s در قالب v210 فشرده یا ۸٫۰ Gbit/s در قالب P210 نیاز دارد. یک لینک USB 3.0 با سرعت ۵ Gbit/s در عمل فقط حدود ۳٫۵ تا ۴ Gbit/s منتقل می‌کند، پس از عهدهٔ این کار برنمی‌آید. به یک درگاه واقعی USB 3.2 Gen 2 با سرعت ۱۰ Gbit/s، کابل مناسب و کارتی که ۱۰ بیت 4:2:2 خروجی دهد نیاز دارید و 4K30 P210 به‌زحمت جا می‌شود. 4K60 در هیچ لینک USB جا نمی‌شود. بیشتر کارت‌های کپچر USB فقط 4:2:2 هشت‌بیتی یا 4:2:0 ده‌بیتی ارائه می‌کنند، پس پیش از خرید قالب‌های خروجی کارت را بررسی کنید.

## انکودرها

| انکودر در OBS | پیاده‌سازی | یادداشت‌ها |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: `prores_aw` از FFmpeg که در افزونه تعبیه شده، با گرد کردن اصلاح‌شده | **توصیه‌شده.** سبک برای CPU: روی CPUهای دارای AVX2 سهمش حدود نصف prores_aw خود FFmpeg است و روی CPUهای قدیمی یا ضعیف به C ساده برمی‌گردد (باز هم از FFmpeg سریع‌تر). Proxy جزئیات ریز را حفظ می‌کند؛ prores_aw در FFmpeg Proxy را محو می‌کند. |
| ProRes-compatible (Rust / FilmCraft) (experimental) | انکودر Rust به نام [FilmCraft](https://github.com/storytold/filmcraft) | آزمایشی. مصرف سنگین CPU در 4K. |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | آزمایشی. در 4K حدود ۲٫۵ برابر CPU مصرف prores_aw (۳۰٪ در برابر ۱۲٪ روی یک لپ‌تاپ ۲۲ رشته‌ای) و فایل‌هایی حدود یک‌سوم بزرگ‌تر در پروفایل 422 روی تصویر زندهٔ دوربین، بدون برتری کیفیت به‌ازای هر بیت. کد بسیار تازه (0.1). |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | فایل‌های کوچک‌تر روی GPU. به کارت **سری RTX 50** نیاز دارد. پایین‌تر را ببینید. |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | برای کارت‌های NVIDIA **قدیمی‌تر از سری RTX 50** (آن‌ها H.264 ده‌بیتی ندارند): **4:2:0** ده‌بیتی، هر فریم کی‌فریم است، یک تنظیم بیت‌ریت. پایین‌تر را ببینید. |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | **4:2:0** ده‌بیتی، هر فریم کی‌فریم است، روی GPU مدل Radeon، با چهار پیش‌تنظیم کیفیت. پایین‌تر را ببینید. |

پروفایل‌ها: Proxy (`apco`)، LT (`apcs`)، 422 (`apcn`)، HQ (`apch`).

### انتخاب اینکه کدام انکودرها نشان داده شوند (Tools > StudioEncoder)

فهرست انکودرهای OBS به‌طور پیش‌فرض کوتاه می‌ماند. **Tools > StudioEncoder** پنجره‌ای باز می‌کند که همهٔ انکودرهای این افزونه را در سه بخش فهرست می‌کند: **Recommended (10-bit 4:2:2)** (AW+ و انکودر H.264 برای RTX 50)، **10-bit 4:2:0** (انکودرهای HEVC برای NVIDIA و AMD) و **Experimental** (FilmCraft و OxideAV، به‌طور پیش‌فرض خاموش). دو بخش آخر گروه‌های جمع‌شدنی‌اند که بسته شروع می‌شوند؛ برای باز کردن هرکدام روی فلش کوچک کلیک کنید. آنچه می‌خواهید تیک بزنید و OK را بزنید؛ تغییر پس از راه‌اندازی دوبارهٔ OBS اعمال می‌شود.

<p align="center"><img src="../images/settings-window.png" alt="پنجرهٔ تنظیمات StudioEncoder (Tools > StudioEncoder) در OBS: فهرستی از انکودرها با چک‌باکس و گروه‌های جمع‌شدنی 10-bit 4:2:0 و Experimental" width="420"></p>

انکودر خاموش‌شده فقط از فهرست کشویی پنهان می‌شود. پروفایلی که از قبل از آن استفاده می‌کند به کار ادامه می‌دهد. همهٔ انکودرهای این افزونه در فهرست انکودرهای OBS با «SE:» شروع می‌شوند تا به‌راحتی دیده شوند.


### چرا AW+ (prores_aw اصلاح‌شده)

prores_aw در FFmpeg هر ضریب جزئیات را بر گام کوانتیزه‌سازی‌اش تقسیم می‌کند و **به سمت صفر برش می‌دهد**، پس هر چیز کوچک‌تر از یک گام کامل دور ریخته می‌شود. در Proxy، که گام‌ها درشت‌اند، این کار بافت ریز را حذف می‌کند: چهره‌ها و پارچه‌ها لکه‌دار به نظر می‌رسند. انکودر Apple همان ماتریس Proxy و همان کوانتایزر را به کار می‌برد اما این جزئیات را نگه می‌دارد. AW+ همان prores_aw است که به افزونه منتقل شده و به‌جای برش از گرد کردن استفاده می‌کند (آفست هر پروفایل: Proxy 4/8، LT 3/8، 422 3/8، HQ 2/8 گام، انتخاب‌شده برای بهترین کیفیت به‌ازای هر بیت).

اندازه‌گیری‌شده روی یک کلیپ دوربین 4K (PSNR، بالاتر بهتر است):

| پروفایل | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s، 54.0 dB | **94 Mbit/s، 56.2 dB** | 96 Mbit/s، 56.5 dB |
| LT | 170 Mbit/s، 59.7 dB | 198 Mbit/s، 61.6 dB | 235 Mbit/s، 62.2 dB |
| HQ | 492 Mbit/s، 71.1 dB | 560 Mbit/s، 73.5 dB | 520 Mbit/s، 72.2 dB |

وقتی گرد کردن خاموش باشد، AW+ فایل‌هایی بایت‌به‌بایت یکسان با prores_aw در FFmpeg تولید می‌کند (روی هر چهار پروفایل بررسی شده).
انکودرهای GPU هنگام شروع OBS کارت گرافیک شما را بررسی می‌کنند. اگر کارت نتواند یکی را اجرا کند (کارت NVIDIA قدیمی‌تر از سری RTX 50 برای انکودر H.264، نبودن کارت NVIDIA با HEVC ده‌بیتی برای انکودر HEVC در NVIDIA، نبودن Radeon برای انکودر AMD)، آن انکودر از فهرست پنهان می‌شود و در این پنجره خاکستری نشان داده می‌شود.

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

جایگزینی صرفه‌جو در فضا برای ProRes که روی GPU انکود می‌شود. هر فریم یک کی‌فریم است (all-intra)، پروفایل High 4:2:2، ده‌بیتی، بیت‌ریت ثابت؛ پس مثل ProRes تدوین می‌شود اما با افت اندکی در کیفیت کوچک‌تر است.

- **به GPU از سری RTX 50 شرکت NVIDIA** (Blackwell) و یک درایور به‌روز نیاز دارد. کارت‌های NVIDIA قدیمی‌تر نمی‌توانند H.264 ده‌بیتی یا 4:2:2 را انکود کنند؛ روی آن‌ها ضبط شروع نمی‌شود و OBS خطا نشان می‌دهد. آنجا از ProRes (prores_aw) استفاده کنید.
- **Bitrate**: 100 Mbit/s (کوچک، در حد کیفیت ProRes LT)، **200 (پیش‌فرض**، بهتر از ProRes LT، حدود نصف حجم ProRes 422)، 300 (زیاد). بالاتر از 400 چیزی اضافه نمی‌کند.
- **Speed**: Quality (p5، پیش‌فرض) یا Fast (p1، کیفیت کمی پایین‌تر).
- فریم‌ها بدون تبدیل به‌صورت P216 به GPU می‌روند. حداکثر ۲ فریم هم‌زمان در جریان است، چون کارت‌های NVIDIA مصرفی تعداد نشست‌های انکودر را محدود می‌کنند.
- در یک آزمون FFmpeg روی RTX 5080 (تصویر دوربین 4K 29.97) این انکودر با p5 حدود ۷۷ فریم بر ثانیه و با p1 حدود ۱۰۱ فریم بر ثانیه اجرا شد.

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

برای کارت‌های NVIDIA **قدیمی‌تر از سری RTX 50** (GTX سری 10 و جدیدتر باید کار کند؛ روی RTX 4070 Laptop آزموده شده). آن کارت‌ها نه حالت H.264 ده‌بیتی دارند و نه 4:2:2، پس این HEVC (Main 10) به‌صورت **4:2:0 ده‌بیتی** است: جزئیات رنگ نصف چیزی است که ProRes 422 نگه می‌دارد، اما هر فریم کی‌فریم است، پس مثل ProRes برش و جابه‌جایی می‌شود، با فایل‌های کوچک‌تر. روی کارت RTX 50 انکودر H.264 4:2:2 بالا را هم دارید که جزئیات رنگ را حفظ می‌کند.

- **Settings > Advanced > Color Format: P010** (نه P216)، Rec. 709، Limited. تصویر همان‌طور که OBS تولید می‌کند بدون تبدیل به GPU می‌رود.
- **Bitrate** (از ۳۰ تا ۴۰۰ Mbit/s، پیش‌فرض ۱۵۰) و **Speed** (Quality p5 یا Fast p1). در آزمونی روی یک کلیپ دوربین 4K (کیفیت لومای نسبت به منبع): 100 Mbit/s برابر 57.6 dB اندازه‌گیری شد، بهتر از ProRes Proxy (53.7 dB در 95)؛ 150 برابر 59.4 dB شد، تقریباً مثل AW+ LT (59.6 dB در 200) با ۷۰٪ بیت‌هایش؛ 200 برابر 60.8 dB شد. پیش‌تنظیم Fast فقط کمی کیفیت پایین‌تر دارد و روی آن GPU تقریباً به همان سرعت است.
- ضبط‌ها HEVC Main 10 در MOV با برچسب `hvc1` هستند (همهٔ فریم‌ها کی‌فریم، بدون خطا در FFmpeg رمزگشایی می‌شوند). هنوز در Resolve یا Premiere امتحان نشده است.
- حداکثر ۲ فریم هم‌زمان انکود می‌شود، چون کارت‌های NVIDIA مصرفی تعداد نشست‌های انکودر را محدود می‌کنند. حدود ۱۵ ms برای هر فریم 4K روی RTX 4070 Laptop؛ OBS حدود ۷٪ از CPU را مصرف کرد (حدود ۵٪ از آن مربوط به خود صحنه است).

### HEVC 10-bit 4:2:0 Intra (AMD)

برای کاربران Radeon. انکودر ویدیوی AMD اصلاً نمی‌تواند 4:2:2 را انجام دهد (و انکودر H.264 آن نمی‌تواند ده‌بیتی کار کند)، پس این **4:2:0 ده‌بیتی** است: جزئیات رنگ نصف چیزی است که ProRes 422 نگه می‌دارد. هر فریم کی‌فریم است، پس مثل ProRes هر فریم را می‌توان بدون رمزگشایی فریم‌های همسایه برش داد و جابه‌جا کرد، با فایل‌هایی بسیار کوچک‌تر (در کلیپ آزمایشی High حدود یک‌سوم ProRes 422 است). «AMD HW H.265 (HEVC)» خود OBS هم می‌تواند ده‌بیتی ضبط کند، اما به GOP معمولی و تنظیم بیت‌ریت نیاز دارد؛ این انکودر آن را با یک انتخاب واحد، **Quality**، جایگزین می‌کند.

- **Settings > Advanced > Color Format: P010** (نه P216)، Rec. 709، Limited.
- **Quality** ثابت است، پس حجم فایل به تصویر بستگی دارد. اندازه‌های معمول برای تصویر دوربین 4K 29.97:

| پیش‌تنظیم | اندازه | PSNR لوما روی یک کلیپ دوربین 4K |
|---|---|---|
| Small | حدود 30 Mbit/s | 52.4 dB |
| Standard | حدود 70 Mbit/s | 55.3 dB (ProRes LT: 56.2 dB در 169 Mbit/s) |
| **High** (پیش‌فرض) | حدود 120 Mbit/s | 56.7 dB |
| Maximum | حدود 170 Mbit/s | 57.8 dB (سقف انکودر) |

  ارقام PSNR با کلیپ دوربینِ تبدیل‌شده به 4:2:0 مقایسه می‌شوند، پس برای این انکودر منصفانه‌اند؛ ProRes رنگ 4:2:2 را نگه می‌دارد که این انکودر نمی‌تواند. تصویر پرنویز در همان پیش‌تنظیم فایل‌های بزرگ‌تری می‌سازد.
- اندازه‌گیری‌شده روی Radeon RX 9070 XT: حدود ۳٫۸ ms برای هر فریم 4K، بدون فریم تکراری در 4K 29.97. انکودر AMD هر بسته را یک فریم دیرتر برمی‌گرداند، پس آخرین فریم پیش از فشردن Stop دور ریخته می‌شود.
- ضبط‌ها HEVC Main 10 در MOV هستند (همهٔ فریم‌ها کی‌فریم، مهر زمانی پایدار، بدون خطا در FFmpeg رمزگشایی می‌شوند). هنوز در Resolve یا Premiere امتحان نشده است.

## کپچر ۱۰ بیتی 4:2:2 (P210)، Windows

منبع Video Capture Device خود OBS نمی‌تواند ۱۰ بیت 4:2:2 را وارد کند: کپچر DirectShow آن به P010 (۱۰ بیت 4:2:0) ختم می‌شود. بعضی کارت‌های کپچر، مثل Magewell Pro Capture، ۱۰ بیت 4:2:2 را به‌صورت P210 ارائه می‌کنند. StudioEncoder برای آن‌ها یک منبع اضافه می‌کند: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**.

- کارت را زیر Device انتخاب کنید یا روی «First device that offers P210» بگذارید.
- Resolution و Frame rate را مطابق سیگنال HDMI دوربین تنظیم کنید (پیش‌فرض 3840x2160، 29.97).
- **Settings > Video > Advanced > Color Format** را روی **P216** بگذارید تا ۱۰ بیت به انکودر برسد. با P216، OBS صحنه را با ممیز شناور ۱۶ بیتی ترکیب می‌کند.
- صدای کارت را با یک منبع جدا از نوع Audio Input Capture اضافه کنید.

فریم‌ها برای OBS از P210 به I210 تبدیل می‌شوند: یک شیفت بدون اتلاف و جداسازی پلین‌ها، حدود ۲٫۴ ms برای هر فریم 4K روی Ryzen 9 9950X3D. با Magewell Pro Capture HDMI 4K در 3840x2160 29.97 آزمایش شد: بدون فریم تکراری، و نمونه‌ها مقادیر واقعی ۱۰ بیتی دارند. `tools/p210-probe.cpp` یک برنامهٔ کوچک خط فرمان است که بررسی می‌کند خروجی P210 یک کارت داده‌های واقعی ۱۰ بیتی دارد یا نه.

## نیازمندی‌ها

- Windows 10/11 x64
- OBS Studio 33.0.x (ساخته‌شده با 33.0.0؛ آزموده‌شده روی 33.0.1)

## نصب

OBS را ببندید، سپس پوشهٔ `obs-studioencoder` را از فایل zip انتشار کپی کنید:

- **OBS نصب‌شده:** به `C:\ProgramData\obs-studio\plugins\obs-studioencoder\`. پوشهٔ `ProgramData` پنهان است؛ مسیر را در File Explorer تایپ کنید.
- **OBS قابل‌حمل:** به `<OBS folder>\plugins\obs-studioencoder\`، کنار `bin`، `data` و `config`.

پوشه باید شامل `obs-studioencoder.dll` و `data\locale\` باشد. در OBS، Tools → Plugin Manager باید «StudioEncoder» را فهرست کند و Tools → StudioEncoder تنظیمات آن را باز می‌کند.

از پوشه‌های قدیمی `obs-plugins\64bit` یا `data\obs-plugins` استفاده نکنید. OBS 33 این مکان‌ها را قدیمی علامت‌گذاری می‌کند.

## تنظیمات

1. Settings → Advanced: **Color Format P216** (برای دو انکودر HEVC **P010**)، Color Space Rec. 709، Color Range Limited.

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings، زبانهٔ Advanced: Colour Format P216 (16-bit, 4:2:2, 2 planes)، Rec. 709، Limited، پررنگ‌شده" width="560">

2. Settings → Video: بوم (Base) و وضوح خروجی (Output) را روی اندازهٔ ضبط خود و نرخ فریم را مطابق منبعتان بگذارید. تصویر 3840x2160 با 29.97 را نشان می‌دهد.

   <img src="../images/obs-video.png" alt="OBS Settings، زبانهٔ Video: وضوح Base و Output برابر 3840x2160، 29.97 fps" width="560">

3. Settings → Output: Output Mode **Advanced**، زبانهٔ Recording، Type **Standard**، Recording Format **MOV**.
4. Video Encoder: یکی از انکودرهای بالا (با «SE:» شروع می‌شود). خط زیر Encoder Settings، یعنی «Required video settings»، Color Format، Color Space و Color Range مورد نیاز این انکودر را تکرار می‌کند.

   <img src="../images/obs-output-encoder.png" alt="OBS Settings، زبانهٔ Output، Recording: نوع Standard، QuickTime (.mov)، Video Encoder SE: ProRes (AW+, fast) پررنگ‌شده" width="560">

   در تنظیمات آن:
   - **Profile** (انکودرهای ProRes): Proxy، LT، 422 یا HQ. **Bitrate** و **Speed** (انکودر NVENC): بالا را ببینید.
   - **Frames in flight**: 0 (خودکار، توصیه‌شده). فریم‌ها در صورت نیاز به‌طور موازی انکود می‌شوند، یکی برای هر هستهٔ CPU.
   - **Automatic sizing** (فقط وقتی Frames in flight برابر 0 است): *Grow when frames are late* (پیش‌فرض) یا *Size from measured frame cost* (آزمایشی). دومی با ۴ worker شروع می‌کند، زمان یک فریم را می‌سنجد، سپس تعداد لازم worker را یک‌جا تنظیم می‌کند. این کار برای انکودرهای سبک حافظهٔ کمتری مصرف می‌کند و برای سنگین‌ها سریع‌تر بالا می‌رود. بار CPU در هر دو یکسان است.
   - **On overload**: *Warn in the log* یا *Stop recording*.
   - **Log verbosity**: Info زمان‌بندی و تنظیم خودکار را نشان می‌دهد.

## بار اضافه

اگر انکودری نتواند همگام بماند، OBS فریم قبلی را تکرار می‌کند تا خط زمانی حفظ شود و ضبط در آن نقاط ثابت به نظر می‌رسد. افزونه این فریم‌های تکراری را برای هر ضبط می‌شمارد (وقتی OBS دو بار پشت‌سرهم همان فریم را به آن بدهد متوجه می‌شود):
- برای هر ثانیه‌ای که این اتفاق می‌افتد یک هشدار `OVERLOAD` در گزارش می‌نویسد و در پایان ضبط یک خلاصه.
- با **On overload: Stop recording**، پس از ۳ ثانیهٔ پیاپی بار اضافه ضبط را تمام می‌کند و خطا نشان می‌دهد.

اگر هشدارهای بار اضافه می‌بینید، به انکودر prores_aw یا یک پروفایل سبک‌تر بروید.

## محدودیت‌های شناخته‌شده

- ورودی باید P216 باشد (برای انکودرهای HEVC، P010). OBS 33 قالب خروجی ۱۰ بیتی 4:2:2 دیگری ارائه نمی‌کند.
- فریم‌هایی که پس از فشردن Stop کپچر شده‌اند اما هنوز در حال انکودند دور ریخته می‌شوند. این رفتار مورد انتظار است؛ ضبط در نقطهٔ توقف پایان می‌یابد.
- MOV هیبریدی آزمایش نشده است.
- Windows. Linux ساخته می‌شود و اجرا می‌شود (روی Ubuntu و CachyOS آزموده شده؛ `docs/HANDOFF.md`، «Build and test on Linux» و «Linux results» را ببینید).

## ساخت

دستورالعمل‌های ساخت (زنجیرهٔ ابزار و مراحل) فقط در [README انگلیسی](../../README.md#build) آمده است.

## مجوز

GPL-2.0-or-later (به `LICENSE` نگاه کنید)، همان‌طور که برای افزونه‌های OBS لازم است. کد شخص ثالث:
- FFmpeg (LGPL)، همان‌طور که همراه OBS عرضه می‌شود.
- crate مربوط به FilmCraft ProRes (MIT OR Apache-2.0، با مجوز MIT استفاده شده).
- AW+ (`src/prores-aw.c`): انتقالی از انکودر prores_aw در FFmpeg، DCT پیشرو و جدول‌های ProRes (LGPL 2.1 یا بالاتر؛ Anatoliy Wasserman، Konstantin Shishkov، Independent JPEG Group)، با مجوز GPL استفاده شده.
- `oxideav-prores` و `oxideav-core` (MIT)، انکودر ProRes در OxideAV.
