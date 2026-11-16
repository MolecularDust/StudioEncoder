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
    <td align="center"><b>日本語</b></td>
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

# StudioEncoder: OBS Studio 向けの編集しやすいエンコーダー

> この翻訳は AI が作成したもので、ネイティブの話者による確認は行われていません。OBS Studio のメニュー、設定、エンコーダーの名前は、OBS の表示どおり英語のままにしています。基準となるのは[英語版 README](../../README.md) です。

OBS Studio 33 以降に、10 ビットのオールイントラ（all-intra）エンコーダーを追加する Windows 用の OBS Studio プラグインです。編集がスムーズな 4K の MOV 録画に使えます。

- **AW+** は ProRes 互換の CPU エンコーダーで、10 ビット 4:2:2 のカラーに対応します。FFmpeg の ProRes エンコーダー（`prores_aw`）の改良版で、ProRes Proxy モードで映像がぼやける丸めのバグを修正しています。速度面の最適化もされており、FFmpeg のエンコーダーのおよそ半分の CPU で動作します。
- **10 ビット 4:2:2 対応の GPU エンコーダー:** NVIDIA RTX 50 シリーズ向けの H.264 と、Intel QuickSync 経由の HEVC（Intel Panther Lake CPU で動作確認）。
- **10 ビット 4:2:0 対応の GPU エンコーダー:** AMD GPU と RTX 50 シリーズより古い NVIDIA カード向けのオールイントラ HEVC。OBS にはこれらの GPU 向けのハードウェア HEVC エンコーダーが既にありますが、StudioEncoder は難解な設定の代わりに簡単なプリセットを用意して使いやすくしています。AMD GPU は 4:2:2 をエンコードできないため、これらは 4:2:0 のままです。

OBS 33.0 の時点では、Custom Output (FFmpeg) の経路にバグがあり、ときどき「non monotonically increasing dts」エラーが発生して、新しい録画が始められなくなることがあります。StudioEncoder はこの経路を使いません。エンコーダーは **Standard** 録画モードで動作するため、スムーズに動きます。

> **Apple ProRes ではありません。** このプラグインは非公式で、Apple とは無関係であり、Apple の承認も受けていません。Apple ProRes はライセンスされた認定コーデックです。ここにあるエンコーダーは、公開されている仕様（SMPTE RDD 36）の独立した実装です。FFmpeg の `prores_aw`（およびそれをもとにした AW+ エンコーダー）、FilmCraft の Rust エンコーダー、OxideAV の Rust エンコーダーです。出力は DaVinci Resolve などの一般的な編集ソフトで再生できますが、Apple の認定は受けていません。

## ハードウェア

実際に 10 ビット 4:2:2 で録画できるかどうかはキャプチャーカードで決まります。カードが PC に 10 ビット 4:2:2 を渡す必要があるからです。

**カード。** PC で 4K 10 ビット 4:2:2 をキャプチャーするいちばん安い方法は、Blackmagic Design の Intensity か DeckLink カード、または Magewell の Pro Capture カードです。AJA や Datapath のカードでも可能ですが、価格はずっと高くなります。

**OBS での P210。** OBS は標準の Video Capture Device ソースでは P210 ピクセルフォーマット（10 ビット 4:2:2）をキャプチャーできず、10 ビット 4:2:0 までです。Blackmagic のカード（v210）は DeckLink ソース経由で OBS で既に使えます。StudioEncoder は Magewell のカードや、P210 を出力するその他のカードへの対応を追加します。これは OBS にはなかったものです。**SE: 10-bit 4:2:2 capture (P210)** ソースを使ってください。

**PCIe カード。** フル性能で動かすには、**x4 レーン**で配線されたスロットが必要です。M.2 ソケットや SATA ポートとレーンを共有させないでください。スロットが x2 や x1 に落ちることがあります。マザーボードのマニュアルを確認してください。

**USB カード。** 4K30 の 10 ビット 4:2:2 には、パックされた v210 で約 5.3 Gbit/s、P210 で 8.0 Gbit/s が必要です。5 Gbit/s の USB 3.0 は実際には 3.5〜4 Gbit/s 程度しか流せないため、対応できません。本物の 10 Gbit/s の USB 3.2 Gen 2 ポート、適切なケーブル、10 ビット 4:2:2 を出力するカードが必要で、4K30 の P210 でようやく収まる程度です。4K60 はどの USB 接続にも収まりません。多くの USB キャプチャーカードは 8 ビット 4:2:2 か 10 ビット 4:2:0 しか出力できないため、購入前にカードの出力フォーマットを確認してください。

## エンコーダー

| OBS 上のエンコーダー | 実装 | 備考 |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: プラグインに組み込んだ FFmpeg の `prores_aw`（丸めを修正） | **推奨。** CPU に軽い: AVX2 対応の CPU では FFmpeg 自身の prores_aw のおよそ半分の負荷で、古い CPU や低価格の CPU ではプレーンな C にフォールバックします（それでも FFmpeg より高速）。Proxy でも細部を保ちますが、FFmpeg の prores_aw では Proxy がぼやけます。 |
| ProRes-compatible (Rust / FilmCraft) (experimental) | [FilmCraft](https://github.com/storytold/filmcraft) の Rust エンコーダー | 実験的。4K では CPU を大きく使います。 |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) の `oxideav-prores` | 実験的。4K で prores_aw のおよそ 2.5 倍の CPU（22 スレッドのノート PC で 30% 対 12%）を使い、実写映像のプロファイル 422 ではファイルが約 3 分の 1 大きく、ビットあたりの画質の利点はありません。コードはとても新しい（0.1）。 |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | GPU で小さいファイルにできます。**RTX 50 シリーズ**のカードが必要です。後述。 |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | **RTX 50 シリーズより古い** NVIDIA カード向け（10 ビット H.264 が使えません）: 10 ビット **4:2:0**、全フレームがキーフレーム、ビットレート設定あり。後述。 |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | Radeon GPU で 10 ビット **4:2:0**、全フレームがキーフレーム、4 つの画質プリセット付き。後述。 |

プロファイル: Proxy（`apco`）、LT（`apcs`）、422（`apcn`）、HQ（`apch`）。

### 表示するエンコーダーの選択（Tools > StudioEncoder）

OBS のエンコーダー一覧は、既定では短くなっています。**Tools > StudioEncoder** を開くと、このプラグインのすべてのエンコーダーを 3 つのセクションに分けて表示するウィンドウが開きます。**Recommended (10-bit 4:2:2)**（AW+ と RTX 50 の H.264 エンコーダー）、**10-bit 4:2:0**（NVIDIA と AMD の HEVC エンコーダー）、**Experimental**（FilmCraft と OxideAV。既定ではオフ）です。後ろの 2 つは折りたたみ可能なグループで、最初は閉じています。小さな矢印をクリックすると開きます。使いたいものにチェックを入れて OK を押してください。変更は OBS を再起動すると反映されます。

<p align="center"><img src="../images/settings-window.png" alt="OBS の StudioEncoder 設定ウィンドウ（Tools > StudioEncoder）: チェックボックス付きのエンコーダー一覧と、折りたたみ可能な 10-bit 4:2:0 および Experimental グループ" width="420"></p>

オフにしたエンコーダーは、ドロップダウンから隠れるだけです。すでにそれを使っているプロファイルは動き続けます。このプラグインのエンコーダーはすべて OBS のエンコーダー一覧で「SE:」から始まるので、見つけやすくなっています。


### AW+（修正した prores_aw）を使う理由

FFmpeg の prores_aw は、各ディテール係数を量子化ステップで割って**ゼロ方向に切り捨て**ます。そのため 1 ステップに満たないものはすべて失われます。ステップが粗い Proxy では、これが細かなテクスチャを消してしまい、顔や布地がのっぺりして見えます。Apple のエンコーダーは同じ Proxy のマトリクスと量子化器を使いながら、この細部を保っています。AW+ は prores_aw をプラグインに移植し、切り捨ての代わりに丸めを行います（プロファイルごとのオフセットは Proxy 4/8、LT 3/8、422 3/8、HQ 2/8 ステップで、ビットあたりの画質が最も良くなるように選んでいます）。

4K のカメラ映像で測定（PSNR、高いほど良い）:

| プロファイル | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s、54.0 dB | **94 Mbit/s、56.2 dB** | 96 Mbit/s、56.5 dB |
| LT | 170 Mbit/s、59.7 dB | 198 Mbit/s、61.6 dB | 235 Mbit/s、62.2 dB |
| HQ | 492 Mbit/s、71.1 dB | 560 Mbit/s、73.5 dB | 520 Mbit/s、72.2 dB |

丸めをオフにすると、AW+ は FFmpeg の prores_aw とバイト単位で同一のファイルを出力します（4 つのプロファイルすべてで確認済み）。
GPU エンコーダーは、OBS の起動時にグラフィックスカードを調べます。カードがいずれかを実行できない場合（H.264 エンコーダーに RTX 50 シリーズより古い NVIDIA カード、NVIDIA の HEVC エンコーダーに 10 ビット HEVC 対応の NVIDIA カードがない、AMD のエンコーダーに Radeon がない）、そのエンコーダーは一覧から隠れ、このウィンドウではグレー表示になります。

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

GPU でエンコードする、容量を節約できる ProRes の代替です。全フレームがキーフレーム（all-intra）で、High 4:2:2 プロファイル、10 ビット、固定ビットレートです。ProRes と同じように編集でき、画質をわずかに犠牲にして、より小さいファイルになります。

- **NVIDIA RTX 50 シリーズの GPU**（Blackwell）と最新のドライバーが必要です。それより古い NVIDIA カードは 10 ビット H.264 も 4:2:2 もエンコードできず、録画が始まらずに OBS がエラーを表示します。その場合は ProRes（prores_aw）を使ってください。
- **Bitrate**: 100 Mbit/s（小さめ、ProRes LT 相当の画質）、**200（既定**、ProRes LT より良く、ProRes 422 の約半分のサイズ）、300（高）。400 を超えても得るものはありません。
- **Speed**: Quality（p5、既定）または Fast（p1、画質はやや低め）。
- フレームは変換なしで P216 のまま GPU に送られます。処理中のフレームは最大 2 つです。一般向けの NVIDIA カードはエンコーダーセッションの数を制限しているためです。
- RTX 5080 での FFmpeg のテスト（4K 29.97 のカメラ映像）では、p5 で約 77 fps、p1 で約 101 fps で動きました。

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

**RTX 50 シリーズより古い** NVIDIA カード向けです（GTX 10 シリーズ以降で動くはずです。RTX 4070 Laptop で確認）。これらのカードには 10 ビット H.264 モードも 4:2:2 もないため、これは **10 ビット 4:2:0** の HEVC（Main 10）です。色情報の細かさは ProRes 422 が保つものの半分ですが、全フレームがキーフレームなので、ProRes と同じようにカットやスクラブができ、ファイルはより小さくなります。RTX 50 のカードでは、色情報を保つ上記の H.264 4:2:2 エンコーダーも使えます。

- **Settings > Advanced > Color Format: P010**（P216 ではありません）、Rec. 709、Limited。映像は OBS が生成したままの形で、変換なしに GPU へ渡されます。
- **Bitrate**（30〜400 Mbit/s、既定 150）と **Speed**（Quality p5 または Fast p1）。4K のカメラ映像 1 本でのテスト（ソースに対する輝度の画質）: 100 Mbit/s は 57.6 dB で、ProRes Proxy（95 Mbit/s で 53.7 dB）より良好。150 は 59.4 dB で、AW+ LT（200 Mbit/s で 59.6 dB）とほぼ同じ画質をそのビット数の 70% で実現。200 は 60.8 dB でした。Fast プリセットは画質がわずかに低いだけで、その GPU では速度もほぼ同じです。
- 録画は `hvc1` タグ付きの MOV 内の HEVC Main 10 です（全フレームがキーフレーム、FFmpeg でエラーなくデコード可能）。Resolve や Premiere ではまだ試していません。
- 同時にエンコードされるフレームは最大 2 つです。一般向けの NVIDIA カードはエンコーダーセッションの数を制限しているためです。RTX 4070 Laptop で 4K フレームあたり約 15 ms。OBS は CPU の約 7% を使いました（そのうち約 5% はシーン自体の分です）。

### HEVC 10-bit 4:2:0 Intra (AMD)

Radeon ユーザー向けです。AMD のビデオエンコーダーは 4:2:2 を一切扱えず（H.264 エンコーダーも 10 ビットを扱えません）、これは **10 ビット 4:2:0** です。色情報の細かさは ProRes 422 が保つものの半分です。全フレームがキーフレームなので、ProRes と同様に隣のフレームをデコードせずに任意のフレームでカットやスクラブでき、ファイルはずっと小さくなります（テストクリップでは High が ProRes 422 の約 3 分の 1）。OBS 標準の「AMD HW H.265 (HEVC)」も 10 ビット録画ができますが、通常の GOP とビットレートの調整が必要です。このエンコーダーはそれを **Quality** の 1 つの選択に置き換えます。

- **Settings > Advanced > Color Format: P010**（P216 ではありません）、Rec. 709、Limited。
- **Quality** は固定なので、ファイルサイズは映像によって変わります。4K 29.97 のカメラ映像での一般的なサイズ:

| プリセット | サイズ | 4K カメラクリップでの輝度 PSNR |
|---|---|---|
| Small | 約 30 Mbit/s | 52.4 dB |
| Standard | 約 70 Mbit/s | 55.3 dB（ProRes LT: 169 Mbit/s で 56.2 dB） |
| **High**（既定） | 約 120 Mbit/s | 56.7 dB |
| Maximum | 約 170 Mbit/s | 57.8 dB（エンコーダーの上限） |

  PSNR は 4:2:0 に変換したカメラクリップと比較した値なので、このエンコーダーにとって公平です。ProRes はこのエンコーダーにできない 4:2:2 の色を保ちます。ノイズの多い映像は、同じプリセットでもファイルが大きくなります。
- Radeon RX 9070 XT での測定: 4K フレームあたり約 3.8 ms、4K 29.97 で繰り返しフレームなし。AMD のエンコーダーは各パケットを 1 フレーム遅れて返すため、Stop を押す直前の最後のフレームは破棄されます。
- 録画は MOV 内の HEVC Main 10 です（全フレームがキーフレーム、タイムスタンプは安定、FFmpeg でエラーなくデコード可能）。Resolve や Premiere ではまだ試していません。

## 10 ビット 4:2:2 キャプチャー（P210）、Windows

OBS 標準の Video Capture Device ソースでは 10 ビット 4:2:2 を取り込めません。DirectShow のキャプチャーは P010（10 ビット 4:2:0）までです。Magewell Pro Capture のように、10 ビット 4:2:2 を P210 で提供するキャプチャーカードもあります。StudioEncoder はそのためのソースを追加します: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**。

- Device でカードを選ぶか、「First device that offers P210」のままにします。
- Resolution と Frame rate をカメラの HDMI 信号に合わせます（既定は 3840x2160、29.97）。
- 10 ビットがエンコーダーに届くよう、**Settings > Video > Advanced > Color Format を P216** にします。P216 では、OBS はシーンを 16 ビット浮動小数点で合成します。
- カードの音声は、別の Audio Input Capture ソースで追加します。

フレームは OBS 向けに P210 から I210 へ変換されます。可逆のシフトとプレーン分離で、Ryzen 9 9950X3D で 4K フレームあたり約 2.4 ms です。Magewell Pro Capture HDMI 4K を 3840x2160 29.97 で使ってテストしました。繰り返しフレームはなく、サンプルは本物の 10 ビット値を持っています。`tools/p210-probe.cpp` は、カードの P210 出力に本物の 10 ビットのデータが入っているかを調べる小さなコマンドラインプログラムです。

## 動作環境

- Windows 10/11 x64
- OBS Studio 33.0.x（33.0.0 でビルド、33.0.1 で動作確認）

## インストール

OBS を閉じてから、リリースの zip にある `obs-studioencoder` フォルダーをコピーします:

- **インストール版の OBS:** `C:\ProgramData\obs-studio\plugins\obs-studioencoder\` へ。`ProgramData` は隠しフォルダーなので、エクスプローラーにパスを入力してください。
- **ポータブル版の OBS:** `<OBS folder>\plugins\obs-studioencoder\` へ。`bin`、`data`、`config` と同じ階層です。

フォルダーには `obs-studioencoder.dll` と `data\locale\` が含まれている必要があります。OBS では、Tools → Plugin Manager に「StudioEncoder」が表示され、Tools → StudioEncoder で設定が開きます。

古い `obs-plugins\64bit` や `data\obs-plugins` フォルダーは使わないでください。OBS 33 はこれらの場所をレガシーとして扱います。

## 設定

1. Settings → Advanced: **Color Format P216**（2 つの HEVC エンコーダーでは **P010**）、Color Space Rec. 709、Color Range Limited。

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings の Advanced タブ: Colour Format P216 (16-bit, 4:2:2, 2 planes)、Rec. 709、Limited が強調表示されている" width="560">

2. Settings → Video: キャンバス（Base）と出力解像度（Output）を録画サイズに、フレームレートをソースに合わせます。スクリーンショットは 29.97 の 3840x2160 です。

   <img src="../images/obs-video.png" alt="OBS Settings の Video タブ: Base と Output の解像度 3840x2160、29.97 fps" width="560">

3. Settings → Output: Output Mode は **Advanced**、Recording タブ、Type は **Standard**、Recording Format は **MOV**。
4. Video Encoder: 上記のエンコーダーのいずれか（「SE:」で始まります）。Encoder Settings の下にある「Required video settings」の行には、このエンコーダーに必要な Color Format、Color Space、Color Range が表示されます。

   <img src="../images/obs-output-encoder.png" alt="OBS Settings の Output タブ、Recording: Standard タイプ、QuickTime (.mov)、Video Encoder の SE: ProRes (AW+, fast) が強調表示されている" width="560">

   エンコーダーの設定:
   - **Profile**（ProRes エンコーダー）: Proxy、LT、422、HQ。**Bitrate** と **Speed**（NVENC エンコーダー）: 前述のとおり。
   - **Frames in flight**: 0（自動、推奨）。フレームは必要に応じて、CPU コアごとに 1 つずつ並列でエンコードされます。
   - **Automatic sizing**（Frames in flight が 0 のときのみ）: *Grow when frames are late*（既定）または *Size from measured frame cost*（実験的）。後者は 4 ワーカーで開始し、1 フレームの時間を測って、必要なワーカー数を一度に決めます。軽いエンコーダーではメモリ使用量が減り、重いエンコーダーでは立ち上がりが速くなります。CPU 負荷はどちらでも同じです。
   - **On overload**: *Warn in the log* または *Stop recording*。
   - **Log verbosity**: Info ではタイミングと自動調整が表示されます。

## 過負荷

エンコーダーが追いつけないと、OBS はタイムラインを保つために直前のフレームを繰り返し、その部分の録画が止まって見えます。プラグインは録画ごとにこの繰り返しフレームを数えます（OBS から同じフレームが 2 回続けて渡されたことを検出します）:
- 発生した秒ごとに `OVERLOAD` 警告をログに書き、録画の終了時にまとめを出力します。
- **On overload: Stop recording** を選ぶと、過負荷の秒が 3 秒続いた時点で録画を終了し、エラーを表示します。

過負荷の警告が出る場合は、prores_aw エンコーダーか、より軽いプロファイルに切り替えてください。

## 既知の制限

- 入力は P216 である必要があります（HEVC エンコーダーでは P010）。OBS 33 には他の 10 ビット 4:2:2 出力フォーマットがありません。
- Stop を押した後にキャプチャーされ、まだエンコード中のフレームは破棄されます。これは想定どおりの動作で、録画は停止した時点で終わります。
- ハイブリッド MOV は未検証です。
- Windows。Linux ではビルドして動作します（Ubuntu と CachyOS で確認。`docs/HANDOFF.md` の「Build and test on Linux」と「Linux results」を参照）。

## ビルド

ビルドの手順（ツールチェーンと各ステップ）は[英語版 README](../../README.md#build)にのみ記載されています。

## ライセンス

OBS プラグインに求められるとおり、GPL-2.0-or-later です（`LICENSE` を参照）。サードパーティのコード:
- FFmpeg（LGPL）。OBS に同梱されているもの。
- FilmCraft ProRes クレート（MIT OR Apache-2.0、MIT で使用）。
- AW+（`src/prores-aw.c`）: FFmpeg の prores_aw エンコーダー、順方向 DCT、ProRes テーブルの移植（LGPL 2.1 以降。Anatoliy Wasserman、Konstantin Shishkov、Independent JPEG Group）。GPL のもとで使用。
- `oxideav-prores` と `oxideav-core`（MIT）、OxideAV の ProRes エンコーダー。
