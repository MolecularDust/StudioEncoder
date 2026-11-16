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
    <td align="center"><b>Italiano</b></td>
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

# StudioEncoder: encoder pensati per il montaggio in OBS Studio

> Questa traduzione è stata scritta da un'IA e non è stata rivista da madrelingua. I nomi di menu, impostazioni ed encoder di OBS Studio sono riportati in inglese, come li mostra OBS. Fa fede il [README in inglese](../../README.md).

Un plugin di OBS Studio per Windows che aggiunge a OBS Studio 33 e successivi encoder all-intra a 10 bit, per registrazioni 4K in MOV che si montano senza intoppi.

- **AW+** è un encoder CPU compatibile con ProRes, con colore a 10 bit 4:2:2. È una versione migliorata dell'encoder ProRes di FFmpeg (`prores_aw`) e corregge un errore di arrotondamento che rendeva sfocate le immagini in modalità ProRes Proxy. È anche ottimizzato per la velocità, quindi usa circa metà della CPU dell'encoder di FFmpeg.
- **Encoder GPU con 10 bit 4:2:2:** H.264 per le schede NVIDIA serie RTX 50 e HEVC tramite Intel QuickSync (provato su una CPU Intel Panther Lake).
- **Encoder GPU con 10 bit 4:2:0:** HEVC all-intra per le GPU AMD e per le schede NVIDIA precedenti alla serie RTX 50. OBS ha già encoder HEVC hardware per queste GPU, ma StudioEncoder li rende più facili da usare, con preset semplici al posto di impostazioni arcane. Le GPU AMD non possono codificare in 4:2:2, quindi questi restano in 4:2:0.

Da OBS 33.0, il percorso Custom Output (FFmpeg) ha un bug che ogni tanto provoca l'errore «non monotonically increasing dts» e può impedire l'avvio di una nuova registrazione. StudioEncoder evita questo percorso: i suoi encoder funzionano in modalità di registrazione **Standard**, quindi girano senza problemi.

> **Non è Apple ProRes.** Questo plugin non è ufficiale e non è affiliato né approvato da Apple. Apple ProRes è un codec con licenza e certificato. Gli encoder qui presenti sono implementazioni indipendenti del formato documentato pubblicamente (SMPTE RDD 36): `prores_aw` di FFmpeg (e l'encoder AW+ costruito a partire da esso), l'encoder Rust FilmCraft e l'encoder Rust OxideAV. Il loro output si riproduce nei comuni programmi di montaggio, come DaVinci Resolve, ma non è certificato da Apple.

## Hardware

La scheda di acquisizione decide se registri davvero in 10 bit 4:2:2, perché deve fornire 10 bit 4:2:2 al PC.

**Schede.** Il modo meno costoso per acquisire 4K 10 bit 4:2:2 su un PC è una scheda Blackmagic Design Intensity o DeckLink, oppure una scheda Magewell Pro Capture. Anche le schede AJA e Datapath lo fanno, ma a un prezzo molto più alto.

**P210 in OBS.** OBS non può acquisire il formato di pixel P210 (10 bit 4:2:2) con la sua sorgente Video Capture Device, che si ferma ai 10 bit 4:2:0. Le schede Blackmagic (v210) funzionano già in OBS tramite la sorgente DeckLink. StudioEncoder aggiunge il supporto per le schede Magewell e per qualsiasi altra scheda che emetta P210, cosa che OBS non ha mai avuto. Usa la sorgente **SE: 10-bit 4:2:2 capture (P210)**.

**Schede PCIe.** La scheda ha bisogno di uno slot cablato a **x4 linee** per funzionare a piena capacità. Non lasciare che condivida le linee con uno zoccolo M.2 o con porte SATA, il che può ridurre lo slot a x2 o x1. Controlla il manuale della scheda madre.

**Schede USB.** 4K30 a 10 bit 4:2:2 richiede circa 5,3 Gbit/s come v210 impacchettato, o 8,0 Gbit/s come P210. Un collegamento USB 3.0 da 5 Gbit/s ne trasporta in pratica solo 3,5-4, quindi non ce la fa. Serve una vera porta USB 3.2 Gen 2 da 10 Gbit/s, un cavo adatto e una scheda che emetta 10 bit 4:2:2, e il 4K30 P210 ci starà appena. Il 4K60 non entra in nessun collegamento USB. La maggior parte delle schede di acquisizione USB offre solo 4:2:2 a 8 bit o 4:2:0 a 10 bit, quindi controlla i formati di uscita di una scheda prima di comprarla.

## Encoder

| Encoder in OBS | Implementazione | Note |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: `prores_aw` di FFmpeg integrato nel plugin, con arrotondamento corretto | **Consigliato.** Leggero per la CPU: sulle CPU con AVX2 la sua quota è circa la metà di quella del prores_aw di FFmpeg, e ripiega su C puro (comunque più veloce di FFmpeg) sulle CPU vecchie o di fascia bassa. Proxy mantiene i dettagli fini; il prores_aw di FFmpeg rende Proxy sfocato. |
| ProRes-compatible (Rust / FilmCraft) (experimental) | Encoder Rust [FilmCraft](https://github.com/storytold/filmcraft) | Sperimentale. Forte uso della CPU in 4K. |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | Sperimentale. Circa 2,5 volte la CPU di prores_aw in 4K (30% contro 12% di un portatile a 22 thread) e file più grandi di circa un terzo al profilo 422 su riprese dal vivo, senza vantaggi di qualità per bit. Codice molto giovane (0.1). |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | File più piccoli sulla GPU. Richiede una scheda della **serie RTX 50**. Vedi sotto. |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | Per le schede NVIDIA **precedenti alla serie RTX 50** (lì non c'è H.264 a 10 bit): **4:2:0** a 10 bit, ogni fotogramma è un keyframe, un'impostazione di bitrate. Vedi sotto. |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | **4:2:0** a 10 bit, ogni fotogramma è un keyframe, su una GPU Radeon, con quattro preset di qualità. Vedi sotto. |

Profili: Proxy (`apco`), LT (`apcs`), 422 (`apcn`), HQ (`apch`).

### Scegliere quali encoder mostrare (Tools > StudioEncoder)

L'elenco degli encoder di OBS resta breve per impostazione predefinita. **Tools > StudioEncoder** apre una finestra che elenca ogni encoder di questo plugin in tre sezioni: **Recommended (10-bit 4:2:2)** (AW+ e l'encoder H.264 per RTX 50), **10-bit 4:2:0** (gli encoder HEVC NVIDIA e AMD) ed **Experimental** (FilmCraft e OxideAV, disattivati per impostazione predefinita). Le ultime due sono gruppi comprimibili che partono chiusi; fai clic sulla piccola freccia per aprirne uno. Spunta quelli che vuoi e premi OK; la modifica ha effetto dopo il riavvio di OBS.

<p align="center"><img src="../images/settings-window.png" alt="La finestra delle impostazioni di StudioEncoder (Tools > StudioEncoder) in OBS: un elenco di encoder con caselle di controllo e gruppi comprimibili 10-bit 4:2:0 ed Experimental" width="420"></p>

Un encoder disattivato viene solo nascosto dal menu a discesa. Un profilo che lo usa già continua a funzionare. Tutti gli encoder di questo plugin iniziano con «SE:» nell'elenco degli encoder di OBS, così sono facili da riconoscere.


### Perché AW+ (prores_aw corretto)

Il prores_aw di FFmpeg divide ogni coefficiente di dettaglio per il suo passo di quantizzazione e **tronca verso zero**, quindi tutto ciò che è più piccolo di un passo intero va perso. In Proxy, dove i passi sono grossolani, questo elimina la grana fine: volti e tessuti sembrano impastati. L'encoder di Apple usa la stessa matrice Proxy e lo stesso quantizzatore, ma conserva quel dettaglio. AW+ è prores_aw portato nel plugin con arrotondamento al posto del troncamento (offset per profilo: Proxy 4/8, LT 3/8, 422 3/8, HQ 2/8 di un passo, scelti per la migliore qualità per bit).

Misurato su un clip di camera 4K (PSNR, più alto è meglio):

| Profilo | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s, 54.0 dB | **94 Mbit/s, 56.2 dB** | 96 Mbit/s, 56.5 dB |
| LT | 170 Mbit/s, 59.7 dB | 198 Mbit/s, 61.6 dB | 235 Mbit/s, 62.2 dB |
| HQ | 492 Mbit/s, 71.1 dB | 560 Mbit/s, 73.5 dB | 520 Mbit/s, 72.2 dB |

Con l'arrotondamento disattivato, AW+ produce file identici byte per byte a quelli del prores_aw di FFmpeg (verificato su tutti e quattro i profili).
Gli encoder GPU controllano la tua scheda grafica all'avvio di OBS. Se la scheda non può eseguirne uno (una scheda NVIDIA precedente alla serie RTX 50 per l'encoder H.264, nessuna scheda NVIDIA con HEVC a 10 bit per l'encoder HEVC NVIDIA, nessuna Radeon per quello AMD), quell'encoder viene nascosto dall'elenco e mostrato in grigio in questa finestra.

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

Un'alternativa a ProRes che fa risparmiare spazio, codificata sulla GPU. Ogni fotogramma è un keyframe (all-intra), profilo High 4:2:2, 10 bit, bitrate costante, quindi si monta come ProRes ma è più piccolo, con una piccola perdita di qualità.

- **Richiede una GPU NVIDIA serie RTX 50** (Blackwell) e un driver aggiornato. Le schede NVIDIA più vecchie non possono codificare H.264 a 10 bit né 4:2:2; su di esse la registrazione non parte e OBS mostra un errore. Lì usa ProRes (prores_aw).
- **Bitrate**: 100 Mbit/s (piccolo, qualità circa ProRes LT), **200 (predefinito**, meglio di ProRes LT, circa metà delle dimensioni di ProRes 422), 300 (alto). Oltre 400 non aggiunge nulla.
- **Speed**: Quality (p5, predefinito) o Fast (p1, qualità leggermente inferiore).
- I fotogrammi vanno alla GPU come P216 senza conversione. Al massimo 2 fotogrammi sono in corso contemporaneamente, perché le schede NVIDIA consumer limitano il numero di sessioni di encoder.
- In un test FFmpeg su una RTX 5080 (riprese da camera 4K 29,97) girava a circa 77 fps con p5 e 101 fps con p1.

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

Per le schede NVIDIA **precedenti alla serie RTX 50** (GTX serie 10 e successive dovrebbero funzionare; provato su una RTX 4070 Laptop). Quelle schede non hanno una modalità H.264 a 10 bit né il 4:2:2, quindi questo è HEVC (Main 10) a **10 bit 4:2:0**: il dettaglio di colore è la metà di quello che mantiene ProRes 422, ma ogni fotogramma è un keyframe, quindi si taglia e si scorre come ProRes, con file più piccoli. Su una scheda RTX 50 hai anche l'encoder H.264 4:2:2 qui sopra, che mantiene il dettaglio di colore.

- **Settings > Advanced > Color Format: P010** (non P216), Rec. 709, Limited. L'immagine va alla GPU come la produce OBS, senza conversione.
- **Bitrate** (da 30 a 400 Mbit/s, predefinito 150) e **Speed** (Quality p5, oppure Fast p1). In un test su un clip di camera 4K (qualità di luma rispetto all'originale): 100 Mbit/s ha misurato 57,6 dB, meglio di ProRes Proxy (53,7 dB a 95); 150 ha misurato 59,4 dB, circa come AW+ LT (59,6 dB a 200) con il 70% dei suoi bit; 200 ha misurato 60,8 dB. Il preset Fast è solo leggermente inferiore in qualità e circa altrettanto veloce su quella GPU.
- Le registrazioni sono HEVC Main 10 in MOV con il tag `hvc1` (tutti i fotogrammi sono keyframe, si decodificano senza errori in FFmpeg). Non ancora provato in Resolve o Premiere.
- Al massimo 2 fotogrammi vengono codificati contemporaneamente, perché le schede NVIDIA consumer limitano il numero di sessioni di encoder. Circa 15 ms per fotogramma 4K sulla RTX 4070 Laptop; OBS ha usato circa il 7% della CPU (circa il 5% di questo è la scena stessa).

### HEVC 10-bit 4:2:0 Intra (AMD)

Per chi usa Radeon. L'encoder video di AMD non sa fare 4:2:2 (e il suo encoder H.264 non sa fare 10 bit), quindi questo è **4:2:0 a 10 bit**: il dettaglio di colore è la metà di quello che mantiene ProRes 422. Ogni fotogramma è un keyframe, quindi ogni fotogramma si può tagliare e scorrere senza decodificare i vicini, come con ProRes, con file molto più piccoli (High è circa un terzo di ProRes 422 sul clip di prova). Anche l'«AMD HW H.265 (HEVC)» di OBS può registrare a 10 bit, ma richiede un GOP normale e la messa a punto del bitrate; questo encoder lo sostituisce con un'unica scelta, **Quality**.

- **Settings > Advanced > Color Format: P010** (non P216), Rec. 709, Limited.
- **Quality** è costante, quindi la dimensione del file dipende dall'immagine. Dimensioni tipiche per riprese da camera 4K 29,97:

| Preset | Dimensione | PSNR di luma su un clip di camera 4K |
|---|---|---|
| Small | circa 30 Mbit/s | 52,4 dB |
| Standard | circa 70 Mbit/s | 55,3 dB (ProRes LT: 56,2 dB a 169 Mbit/s) |
| **High** (predefinito) | circa 120 Mbit/s | 56,7 dB |
| Maximum | circa 170 Mbit/s | 57,8 dB (il limite dell'encoder) |

  I valori di PSNR confrontano con il clip di camera convertito in 4:2:0, quindi sono equi per questo encoder; ProRes mantiene un colore 4:2:2 che questo non può. Riprese rumorose producono file più grandi con lo stesso preset.
- Misurato su una Radeon RX 9070 XT: circa 3,8 ms per fotogramma 4K, nessun fotogramma ripetuto a 4K 29,97. L'encoder AMD restituisce ogni pacchetto con un fotogramma di ritardo, quindi l'ultimo fotogramma prima di premere Stop viene scartato.
- Le registrazioni sono HEVC Main 10 in MOV (tutti i fotogrammi sono keyframe, timestamp regolari, si decodificano senza errori in FFmpeg). Non ancora provato in Resolve o Premiere.

## Acquisizione 10 bit 4:2:2 (P210), Windows

La sorgente Video Capture Device di OBS non può portare dentro 10 bit 4:2:2: la sua acquisizione DirectShow si ferma a P010 (10 bit 4:2:0). Alcune schede di acquisizione, come Magewell Pro Capture, offrono 10 bit 4:2:2 come P210. StudioEncoder aggiunge una sorgente per loro: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**.

- Scegli la scheda in Device, oppure lascia «First device that offers P210».
- Imposta Resolution e Frame rate in modo che corrispondano al segnale HDMI della camera (predefinito 3840x2160, 29.97).
- Imposta **Settings > Video > Advanced > Color Format su P216** perché i 10 bit arrivino all'encoder. Con P216, OBS mixa la scena in virgola mobile a 16 bit.
- Aggiungi l'audio della scheda con una sorgente Audio Input Capture separata.

I fotogrammi vengono convertiti da P210 a I210 per OBS: uno spostamento senza perdita e una separazione dei piani, circa 2,4 ms per fotogramma 4K su un Ryzen 9 9950X3D. Provato con una Magewell Pro Capture HDMI 4K a 3840x2160 29.97: nessun fotogramma ripetuto, e i campioni portano veri valori a 10 bit. `tools/p210-probe.cpp` è un piccolo programma da riga di comando che verifica se l'uscita P210 di una scheda contiene veri dati a 10 bit.

## Requisiti

- Windows 10/11 x64
- OBS Studio 33.0.x (compilato con 33.0.0; provato su 33.0.1)

## Installazione

Chiudi OBS, poi copia la cartella `obs-studioencoder` dallo zip della release:

- **OBS installato:** in `C:\ProgramData\obs-studio\plugins\obs-studioencoder\`. `ProgramData` è nascosta; digita il percorso in Esplora file.
- **OBS portatile:** in `<OBS folder>\plugins\obs-studioencoder\`, accanto a `bin`, `data` e `config`.

La cartella deve contenere `obs-studioencoder.dll` e `data\locale\`. In OBS, Tools → Plugin Manager dovrebbe elencare «StudioEncoder», e Tools → StudioEncoder ne apre le impostazioni.

Non usare le vecchie cartelle `obs-plugins\64bit` o `data\obs-plugins`. OBS 33 segnala quelle posizioni come obsolete.

## Impostazioni

1. Settings → Advanced: **Color Format P216** (**P010** per i due encoder HEVC), Color Space Rec. 709, Color Range Limited.

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings, scheda Advanced: Colour Format P216 (16-bit, 4:2:2, 2 planes), Rec. 709, Limited, evidenziato" width="560">

2. Settings → Video: imposta la tela (Base) e la risoluzione di uscita (Output) sulla dimensione della tua registrazione e la frequenza fotogrammi su quella della tua sorgente. Lo screenshot mostra 3840x2160 a 29.97.

   <img src="../images/obs-video.png" alt="OBS Settings, scheda Video: risoluzione Base e Output 3840x2160, 29.97 fps" width="560">

3. Settings → Output: Output Mode **Advanced**, scheda Recording, Type **Standard**, Recording Format **MOV**.
4. Video Encoder: uno degli encoder sopra (inizia con «SE:»). La riga sotto Encoder Settings, «Required video settings», ripete il Color Format, il Color Space e il Color Range di cui questo encoder ha bisogno.

   <img src="../images/obs-output-encoder.png" alt="OBS Settings, scheda Output, Recording: tipo Standard, QuickTime (.mov), Video Encoder SE: ProRes (AW+, fast) evidenziato" width="560">

   Nelle sue impostazioni:
   - **Profile** (encoder ProRes): Proxy, LT, 422 o HQ. **Bitrate** e **Speed** (encoder NVENC): vedi sopra.
   - **Frames in flight**: 0 (automatico, consigliato). I fotogrammi sono codificati in parallelo, uno per core della CPU, secondo necessità.
   - **Automatic sizing** (solo con Frames in flight a 0): *Grow when frames are late* (predefinito) o *Size from measured frame cost* (sperimentale). Il secondo parte con 4 worker, cronometra un fotogramma, poi imposta in un colpo solo il numero di worker necessari. Usa meno memoria con gli encoder leggeri e sale più in fretta con quelli pesanti. Il carico della CPU è lo stesso in entrambi i casi.
   - **On overload**: *Warn in the log* o *Stop recording*.
   - **Log verbosity**: Info mostra i tempi e la regolazione automatica.

## Sovraccarico

Se un encoder non riesce a stare al passo, OBS ripete il fotogramma precedente per mantenere la timeline, e la registrazione appare bloccata in quei punti. Il plugin conta quei fotogrammi ripetuti per ogni registrazione (si accorge quando OBS gli passa lo stesso fotogramma due volte di seguito):
- Scrive un avviso `OVERLOAD` nel log per ogni secondo in cui succede, e un riepilogo alla fine della registrazione.
- Con **On overload: Stop recording**, termina la registrazione dopo 3 secondi di sovraccarico consecutivi e mostra un errore.

Se vedi avvisi di sovraccarico, passa all'encoder prores_aw o a un profilo più leggero.

## Limiti noti

- L'ingresso deve essere P216 (P010 per gli encoder HEVC). OBS 33 non offre altri formati di uscita 10 bit 4:2:2.
- I fotogrammi acquisiti dopo aver premuto Stop, ma ancora in codifica, vengono scartati. È previsto; la registrazione termina al punto di arresto.
- MOV ibrido non testato.
- Windows. Linux compila e funziona (provato su Ubuntu e CachyOS; vedi `docs/HANDOFF.md`, «Build and test on Linux» e «Linux results»).

## Compilazione

Le istruzioni di compilazione (toolchain e passaggi) sono solo nel [README in inglese](../../README.md#build).

## Licenza

GPL-2.0-or-later (vedi `LICENSE`), come richiesto per i plugin di OBS. Codice di terze parti:
- FFmpeg (LGPL), come distribuito con OBS.
- Crate ProRes di FilmCraft (MIT OR Apache-2.0, usato con licenza MIT).
- AW+ (`src/prores-aw.c`): un port dell'encoder prores_aw di FFmpeg, della DCT diretta e delle tabelle ProRes (LGPL 2.1 o successiva; Anatoliy Wasserman, Konstantin Shishkov, Independent JPEG Group), usato con licenza GPL.
- `oxideav-prores` e `oxideav-core` (MIT), l'encoder ProRes di OxideAV.
