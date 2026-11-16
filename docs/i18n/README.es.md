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
    <td align="center"><b>Español</b></td>
    <td align="center"><a href="README.te.md">తెలుగు</a></td>
    <td align="center"><a href="README.th.md">ไทย</a></td>
    <td align="center"><a href="README.tr.md">Türkçe</a></td>
    <td align="center"><a href="README.vi.md">Tiếng Việt</a></td>
  </tr>
</table>

</details>

</div>
<!-- languages:end -->

# StudioEncoder: codificadores pensados para editar en OBS Studio

> Esta traducción la ha escrito una IA y no la han revisado hablantes nativos. Los nombres de menús, ajustes y codificadores de OBS Studio se dan en inglés, tal como los muestra OBS. El texto de referencia es el [README en inglés](../../README.md).

Un plugin de OBS Studio para Windows que añade a OBS Studio 33 y posteriores codificadores all-intra de 10 bits, para grabaciones 4K en MOV que se editan con fluidez.

- **AW+** es un codificador por CPU compatible con ProRes, con color 10 bits 4:2:2. Es una versión mejorada del codificador ProRes de FFmpeg (`prores_aw`) y corrige un error de redondeo que desenfocaba la imagen en el modo ProRes Proxy. Además está optimizado para la velocidad, así que usa aproximadamente la mitad de CPU que el codificador de FFmpeg.
- **Codificadores GPU con 10 bits 4:2:2:** H.264 para tarjetas NVIDIA de la serie RTX 50, y HEVC mediante Intel QuickSync (probado en una CPU Intel Panther Lake).
- **Codificadores GPU con 10 bits 4:2:0:** HEVC all-intra para GPU de AMD y para tarjetas NVIDIA anteriores a la serie RTX 50. OBS ya tiene codificadores HEVC por hardware para estas GPU, pero StudioEncoder los hace más fáciles de usar, con ajustes preestablecidos sencillos en lugar de opciones arcanas. Las GPU de AMD no pueden codificar 4:2:2, así que estos se quedan en 4:2:0.

Desde OBS 33.0, la vía Custom Output (FFmpeg) tiene un error que de vez en cuando provoca el error «non monotonically increasing dts» y puede impedir que arranque una grabación nueva. StudioEncoder evita esta vía: sus codificadores funcionan en el modo de grabación **Standard**, así que funcionan sin problemas.

> **No es Apple ProRes.** Este plugin no es oficial y no está afiliado a Apple ni respaldado por Apple. Apple ProRes es un códec con licencia y certificado. Los codificadores de aquí son implementaciones independientes del formato documentado públicamente (SMPTE RDD 36): `prores_aw` de FFmpeg (y el codificador AW+ construido a partir de él), el codificador Rust FilmCraft y el codificador Rust OxideAV. Su salida se reproduce en editores habituales, como DaVinci Resolve, pero no está certificada por Apple.

## Hardware

La capturadora decide si realmente grabas en 10 bits 4:2:2, porque tiene que entregar 10 bits 4:2:2 al PC.

**Tarjetas.** La forma más barata de capturar 4K 10 bits 4:2:2 en un PC es una tarjeta Blackmagic Design Intensity o DeckLink, o una tarjeta Magewell Pro Capture. Las tarjetas AJA y Datapath también lo hacen, pero a un precio mucho más alto.

**P210 en OBS.** OBS no puede capturar el formato de píxel P210 (10 bits 4:2:2) en su propia fuente Video Capture Device, que se queda en 10 bits 4:2:0. Las tarjetas Blackmagic (v210) ya funcionan en OBS mediante su fuente DeckLink. StudioEncoder añade compatibilidad con las tarjetas Magewell y con cualquier otra tarjeta que dé P210, algo que OBS nunca tuvo. Usa la fuente **SE: 10-bit 4:2:2 capture (P210)**.

**Tarjetas PCIe.** La tarjeta necesita una ranura cableada con **4 carriles (x4)** para funcionar a plena capacidad. No dejes que comparta carriles con un zócalo M.2 o puertos SATA, lo que puede reducir la ranura a x2 o x1. Consulta el manual de tu placa base.

**Tarjetas USB.** 4K30 en 10 bits 4:2:2 necesita unos 5,3 Gbit/s como v210 empaquetado, u 8,0 Gbit/s como P210. Un enlace USB 3.0 de 5 Gbit/s transporta en la práctica solo unos 3,5 a 4 Gbit/s, así que no puede con esto. Necesitas un puerto USB 3.2 Gen 2 de verdad, de 10 Gbit/s, un cable adecuado y una tarjeta que dé 10 bits 4:2:2, y 4K30 P210 apenas cabrá. 4K60 no cabe en ningún enlace USB. La mayoría de las capturadoras USB solo ofrecen 4:2:2 de 8 bits o 4:2:0 de 10 bits, así que comprueba los formatos de salida de una tarjeta antes de comprarla.

## Codificadores

| Codificador en OBS | Implementación | Notas |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+: `prores_aw` de FFmpeg integrado en el plugin, con el redondeo corregido | **Recomendado.** Ligero para la CPU: en CPU con AVX2 su parte es aproximadamente la mitad que la del prores_aw de FFmpeg, y recurre a C puro (aun así más rápido que FFmpeg) en CPU antiguas o modestas. Proxy conserva el detalle fino; el prores_aw de FFmpeg desenfoca Proxy. |
| ProRes-compatible (Rust / FilmCraft) (experimental) | Codificador Rust [FilmCraft](https://github.com/storytold/filmcraft) | Experimental. Mucho uso de CPU en 4K. |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | Experimental. Unas 2,5 veces la CPU de prores_aw en 4K (30 % frente a 12 % de un portátil de 22 hilos) y archivos aproximadamente un tercio más grandes en el perfil 422 con material de cámara en directo, sin ventaja de calidad por bit. Código muy joven (0.1). |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | Archivos más pequeños en la GPU. Necesita una tarjeta de la **serie RTX 50**. Véase más abajo. |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | Para tarjetas NVIDIA **anteriores a la serie RTX 50** (allí no hay H.264 de 10 bits): **4:2:0** de 10 bits, cada fotograma es un fotograma clave, un ajuste de tasa de bits. Véase más abajo. |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | **4:2:0** de 10 bits, cada fotograma es un fotograma clave, en una GPU Radeon, con cuatro ajustes de calidad. Véase más abajo. |

Perfiles: Proxy (`apco`), LT (`apcs`), 422 (`apcn`), HQ (`apch`).

### Elegir qué codificadores aparecen (Tools > StudioEncoder)

La lista de codificadores de OBS es corta por defecto. **Tools > StudioEncoder** abre una ventana que enumera todos los codificadores de este plugin en tres secciones: **Recommended (10-bit 4:2:2)** (AW+ y el codificador H.264 para RTX 50), **10-bit 4:2:0** (los codificadores HEVC de NVIDIA y AMD) y **Experimental** (FilmCraft y OxideAV, desactivados por defecto). Los dos últimos son grupos plegables que empiezan cerrados; haz clic en la flechita para abrir uno. Marca los que quieras y pulsa OK; el cambio surte efecto al reiniciar OBS.

<p align="center"><img src="../images/settings-window.png" alt="La ventana de ajustes de StudioEncoder (Tools > StudioEncoder) en OBS: una lista de codificadores con casillas de verificación y grupos plegables 10-bit 4:2:0 y Experimental" width="420"></p>

Un codificador desactivado solo se oculta del desplegable. Un perfil que ya lo usa sigue funcionando. Todos los codificadores de este plugin empiezan por «SE:» en la lista de codificadores de OBS, para reconocerlos fácilmente.


### Por qué AW+ (prores_aw corregido)

El prores_aw de FFmpeg divide cada coeficiente de detalle por su paso de cuantificación y **trunca hacia cero**, de modo que todo lo menor que un paso completo se descarta. En Proxy, donde los pasos son gruesos, eso elimina la textura fina: las caras y las telas se ven emborronadas. El codificador de Apple usa la misma matriz Proxy y el mismo cuantificador, pero conserva ese detalle. AW+ es prores_aw portado al plugin con redondeo en lugar de truncamiento (desplazamiento por perfil: Proxy 4/8, LT 3/8, 422 3/8, HQ 2/8 de un paso, elegidos para obtener la mejor calidad por bit).

Medido en un clip de cámara 4K (PSNR, más alto es mejor):

| Perfil | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s, 54.0 dB | **94 Mbit/s, 56.2 dB** | 96 Mbit/s, 56.5 dB |
| LT | 170 Mbit/s, 59.7 dB | 198 Mbit/s, 61.6 dB | 235 Mbit/s, 62.2 dB |
| HQ | 492 Mbit/s, 71.1 dB | 560 Mbit/s, 73.5 dB | 520 Mbit/s, 72.2 dB |

Con el redondeo desactivado, AW+ produce archivos idénticos byte a byte a los del prores_aw de FFmpeg (comprobado en los cuatro perfiles).
Los codificadores GPU comprueban tu tarjeta gráfica al iniciar OBS. Si la tarjeta no puede ejecutar uno (una tarjeta NVIDIA anterior a la serie RTX 50 para el codificador H.264, ninguna tarjeta NVIDIA con HEVC de 10 bits para el codificador HEVC de NVIDIA, ninguna Radeon para el de AMD), ese codificador se oculta de la lista y se muestra atenuado en esta ventana.

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

Una alternativa a ProRes que ahorra espacio, codificada en la GPU. Cada fotograma es un fotograma clave (all-intra), perfil High 4:2:2, 10 bits, tasa de bits constante, así que se edita como ProRes pero es más pequeño a cambio de una pequeña pérdida de calidad.

- **Necesita una GPU NVIDIA de la serie RTX 50** (Blackwell) y un controlador actual. Las tarjetas NVIDIA más antiguas no pueden codificar H.264 de 10 bits ni 4:2:2; en ellas la grabación no arranca y OBS muestra un error. Usa ahí ProRes (prores_aw).
- **Bitrate**: 100 Mbit/s (pequeño, calidad parecida a ProRes LT), **200 (por defecto**, mejor que ProRes LT, aproximadamente la mitad del tamaño de ProRes 422), 300 (alto). Por encima de 400 no aporta nada.
- **Speed**: Quality (p5, por defecto) o Fast (p1, calidad algo menor).
- Los fotogramas llegan a la GPU como P216 sin conversión. Como máximo hay 2 fotogramas en curso, porque las tarjetas NVIDIA de consumo limitan el número de sesiones de codificador.
- En una prueba de FFmpeg en una RTX 5080 (material de cámara 4K 29,97) funcionó a unos 77 fps en p5 y 101 fps en p1.

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

Para tarjetas NVIDIA **anteriores a la serie RTX 50** (la serie GTX 10 y posteriores deberían funcionar; probado en una RTX 4070 Laptop). Esas tarjetas no tienen modo H.264 de 10 bits ni 4:2:2, así que esto es HEVC (Main 10) en **4:2:0 de 10 bits**: el detalle de color es la mitad de lo que conserva ProRes 422, pero cada fotograma es un fotograma clave, así que se corta y se recorre como ProRes, en archivos más pequeños. En una tarjeta RTX 50 también tienes el codificador H.264 4:2:2 de arriba, que conserva el detalle de color.

- **Settings > Advanced > Color Format: P010** (no P216), Rec. 709, Limited. La imagen llega a la GPU tal como la produce OBS, sin conversión.
- **Bitrate** (de 30 a 400 Mbit/s, 150 por defecto) y **Speed** (Quality p5, o Fast p1). En una prueba con un clip de cámara 4K (calidad de luma frente al original): 100 Mbit/s midió 57,6 dB, mejor que ProRes Proxy (53,7 dB a 95); 150 midió 59,4 dB, aproximadamente lo mismo que AW+ LT (59,6 dB a 200) con el 70 % de sus bits; 200 midió 60,8 dB. El ajuste Fast es solo ligeramente peor en calidad y casi igual de rápido en esa GPU.
- Las grabaciones son HEVC Main 10 en MOV con la etiqueta `hvc1` (todos los fotogramas son clave, se decodifican sin errores en FFmpeg). Aún no probado en Resolve ni Premiere.
- Se codifican como máximo 2 fotogramas a la vez, porque las tarjetas NVIDIA de consumo limitan el número de sesiones de codificador. Unos 15 ms por fotograma 4K en la RTX 4070 Laptop; OBS usó cerca del 7 % de la CPU (cerca del 5 % de eso es la propia escena).

### HEVC 10-bit 4:2:0 Intra (AMD)

Para usuarios de Radeon. El codificador de vídeo de AMD no puede hacer 4:2:2 en absoluto (y su codificador H.264 no puede hacer 10 bits), así que esto es **4:2:0 de 10 bits**: el detalle de color es la mitad de lo que conserva ProRes 422. Cada fotograma es un fotograma clave, así que cada fotograma se puede cortar y recorrer sin decodificar los vecinos, como con ProRes, en archivos mucho más pequeños (High es aproximadamente un tercio de ProRes 422 en el clip de prueba). El «AMD HW H.265 (HEVC)» propio de OBS también puede grabar en 10 bits, pero necesita un GOP normal y ajustar la tasa de bits; este codificador lo sustituye por una única opción, **Quality**.

- **Settings > Advanced > Color Format: P010** (no P216), Rec. 709, Limited.
- **Quality** es constante, así que el tamaño del archivo depende de la imagen. Tamaños típicos para material de cámara 4K 29,97:

| Ajuste | Tamaño | PSNR de luma en un clip de cámara 4K |
|---|---|---|
| Small | unos 30 Mbit/s | 52,4 dB |
| Standard | unos 70 Mbit/s | 55,3 dB (ProRes LT: 56,2 dB a 169 Mbit/s) |
| **High** (por defecto) | unos 120 Mbit/s | 56,7 dB |
| Maximum | unos 170 Mbit/s | 57,8 dB (el techo del codificador) |

  Las cifras de PSNR comparan con el clip de cámara convertido a 4:2:0, así que son justas para este codificador; ProRes conserva un color 4:2:2 que este no puede. El material con ruido genera archivos más grandes con el mismo ajuste.
- Medido en una Radeon RX 9070 XT: unos 3,8 ms por fotograma 4K, sin fotogramas repetidos en 4K 29,97. El codificador de AMD devuelve cada paquete con un fotograma de retraso, así que se descarta el último fotograma antes de pulsar Stop.
- Las grabaciones son HEVC Main 10 en MOV (todos los fotogramas son clave, marcas de tiempo estables, se decodifican sin errores en FFmpeg). Aún no probado en Resolve ni Premiere.

## Captura 10 bits 4:2:2 (P210), Windows

La propia fuente Video Capture Device de OBS no puede traer 10 bits 4:2:2: su captura DirectShow se queda en P010 (10 bits 4:2:0). Algunas capturadoras, como Magewell Pro Capture, ofrecen 10 bits 4:2:2 como P210. StudioEncoder añade una fuente para ellas: **Sources > + > SE: 10-bit 4:2:2 capture (P210)**.

- Elige la tarjeta en Device, o déjalo en «First device that offers P210».
- Ajusta Resolution y Frame rate para que coincidan con la señal HDMI de la cámara (por defecto 3840x2160, 29.97).
- Pon **Settings > Video > Advanced > Color Format en P216** para que los 10 bits lleguen al codificador. Con P216, OBS mezcla la escena en coma flotante de 16 bits.
- Añade el audio de la tarjeta con una fuente Audio Input Capture aparte.

Los fotogramas se convierten de P210 a I210 para OBS: un desplazamiento sin pérdida y una separación de planos, unos 2,4 ms por fotograma 4K en un Ryzen 9 9950X3D. Probado con una Magewell Pro Capture HDMI 4K a 3840x2160 29.97: sin fotogramas repetidos, y las muestras llevan valores reales de 10 bits. `tools/p210-probe.cpp` es un pequeño programa de línea de comandos que comprueba si la salida P210 de una tarjeta contiene datos reales de 10 bits.

## Requisitos

- Windows 10/11 x64
- OBS Studio 33.0.x (compilado con 33.0.0; probado en 33.0.1)

## Instalación

Cierra OBS y copia la carpeta `obs-studioencoder` del zip de la versión:

- **OBS instalado:** en `C:\ProgramData\obs-studio\plugins\obs-studioencoder\`. `ProgramData` está oculta; escribe la ruta en el Explorador de archivos.
- **OBS portable:** en `<OBS folder>\plugins\obs-studioencoder\`, junto a `bin`, `data` y `config`.

La carpeta debe contener `obs-studioencoder.dll` y `data\locale\`. En OBS, Tools → Plugin Manager debería listar «StudioEncoder», y Tools → StudioEncoder abre sus ajustes.

No uses las antiguas carpetas `obs-plugins\64bit` ni `data\obs-plugins`. OBS 33 marca esas ubicaciones como heredadas.

## Ajustes

1. Settings → Advanced: **Color Format P216** (**P010** para los dos codificadores HEVC), Color Space Rec. 709, Color Range Limited.

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings, pestaña Advanced: Colour Format P216 (16-bit, 4:2:2, 2 planes), Rec. 709, Limited, resaltado" width="560">

2. Settings → Video: ajusta el lienzo (Base) y la resolución de salida (Output) al tamaño de tu grabación y la velocidad de fotogramas a la de tu fuente. La captura de pantalla muestra 3840x2160 a 29.97.

   <img src="../images/obs-video.png" alt="OBS Settings, pestaña Video: resolución Base y Output 3840x2160, 29.97 fps" width="560">

3. Settings → Output: Output Mode **Advanced**, pestaña Recording, Type **Standard**, Recording Format **MOV**.
4. Video Encoder: uno de los codificadores anteriores (empieza por «SE:»). La línea bajo Encoder Settings, «Required video settings», repite el Color Format, el Color Space y el Color Range que necesita este codificador.

   <img src="../images/obs-output-encoder.png" alt="OBS Settings, pestaña Output, Recording: tipo Standard, QuickTime (.mov), Video Encoder SE: ProRes (AW+, fast) resaltado" width="560">

   En sus ajustes:
   - **Profile** (codificadores ProRes): Proxy, LT, 422 o HQ. **Bitrate** y **Speed** (codificador NVENC): véase más arriba.
   - **Frames in flight**: 0 (automático, recomendado). Los fotogramas se codifican en paralelo, uno por núcleo de CPU, según haga falta.
   - **Automatic sizing** (solo con Frames in flight en 0): *Grow when frames are late* (por defecto) o *Size from measured frame cost* (experimental). El segundo empieza con 4 workers, mide un fotograma y luego fija el número de workers necesarios de una vez. Eso usa menos memoria con codificadores ligeros y se acelera antes con los pesados. La carga de CPU es la misma en ambos casos.
   - **On overload**: *Warn in the log* o *Stop recording*.
   - **Log verbosity**: Info muestra tiempos y el ajuste automático.

## Sobrecarga

Si un codificador no puede seguir el ritmo, OBS repite el fotograma anterior para mantener la línea de tiempo, y la grabación se ve congelada en esos puntos. El plugin cuenta esos fotogramas repetidos en cada grabación (nota cuando OBS le entrega el mismo fotograma dos veces seguidas):
- Escribe un aviso `OVERLOAD` en el registro por cada segundo en que ocurre, y un resumen cuando termina la grabación.
- Con **On overload: Stop recording**, termina la grabación tras 3 segundos de sobrecarga seguidos y muestra un error.

Si ves avisos de sobrecarga, cambia al codificador prores_aw o a un perfil más ligero.

## Límites conocidos

- La entrada debe ser P216 (P010 para los codificadores HEVC). OBS 33 no ofrece otro formato de salida 10 bits 4:2:2.
- Los fotogramas capturados después de pulsar Stop, pero que aún se están codificando, se descartan. Es lo esperado; la grabación termina en el punto de parada.
- MOV híbrido sin probar.
- Windows. Linux compila y funciona (probado en Ubuntu y CachyOS; véase `docs/HANDOFF.md`, «Build and test on Linux» y «Linux results»).

## Compilación

Las instrucciones de compilación (cadena de herramientas y pasos) solo están en el [README en inglés](../../README.md#build).

## Licencia

GPL-2.0-or-later (véase `LICENSE`), como exigen los plugins de OBS. Código de terceros:
- FFmpeg (LGPL), tal como se distribuye con OBS.
- Crate ProRes de FilmCraft (MIT OR Apache-2.0, usado bajo MIT).
- AW+ (`src/prores-aw.c`): un port del codificador prores_aw de FFmpeg, de la DCT directa y de las tablas de ProRes (LGPL 2.1 o posterior; Anatoliy Wasserman, Konstantin Shishkov, Independent JPEG Group), usado bajo la GPL.
- `oxideav-prores` y `oxideav-core` (MIT), el codificador ProRes de OxideAV.
