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
    <td align="center"><b>Français</b></td>
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

# StudioEncoder : des encodeurs adaptés au montage pour OBS Studio

> Cette traduction a été écrite par une IA et n'a pas été relue par des locuteurs natifs. Les noms des menus, des réglages et des encodeurs d'OBS Studio sont donnés en anglais, tels qu'OBS les affiche. Le texte de référence est le [README en anglais](../../README.md).

Un plugin OBS Studio pour Windows qui ajoute à OBS Studio 33 et suivants des encodeurs all-intra 10 bits, pour des enregistrements 4K en MOV qui se montent sans accroc.

- **AW+** est un encodeur CPU compatible ProRes, avec une couleur 10 bits 4:2:2. C'est une version améliorée de l'encodeur ProRes de FFmpeg (`prores_aw`) qui corrige une erreur d'arrondi rendant l'image floue en mode ProRes Proxy. Il est aussi optimisé pour la vitesse : il utilise environ moitié moins de CPU que l'encodeur de FFmpeg.
- **Encodeurs GPU en 10 bits 4:2:2 :** H.264 pour les cartes NVIDIA de la série RTX 50, et HEVC via Intel QuickSync (testé sur un CPU Intel Panther Lake).
- **Encodeurs GPU en 10 bits 4:2:0 :** HEVC all-intra pour les GPU AMD et pour les cartes NVIDIA antérieures à la série RTX 50. OBS dispose déjà d'encodeurs HEVC matériels pour ces GPU, mais StudioEncoder les rend plus simples à utiliser, avec des préréglages clairs au lieu de réglages obscurs. Les GPU AMD ne peuvent pas encoder en 4:2:2 ; ces encodeurs restent donc en 4:2:0.

<p align="center"><img src="../images/settings-window.png" alt="La fenêtre de réglages de StudioEncoder (Tools > StudioEncoder) dans OBS : une liste d'encodeurs avec des cases à cocher et des groupes repliables 10-bit 4:2:0 et Experimental" width="420"></p>

Depuis OBS 33.0, la voie Custom Output (FFmpeg) comporte un bug qui provoque de temps en temps l'erreur « non monotonically increasing dts » et peut empêcher un nouvel enregistrement de démarrer. StudioEncoder évite cette voie : ses encodeurs fonctionnent en mode d'enregistrement **Standard**, et tournent donc sans problème.

> **Pas Apple ProRes.** Ce plugin est non officiel et n'est ni affilié à Apple ni approuvé par Apple. Apple ProRes est un codec sous licence et certifié. Les encodeurs présentés ici sont des implémentations indépendantes du format publiquement documenté (SMPTE RDD 36) : `prores_aw` de FFmpeg (et l'encodeur AW+ qui en dérive), l'encodeur Rust FilmCraft et l'encodeur Rust OxideAV. Leur sortie se lit dans les logiciels de montage courants, comme DaVinci Resolve, mais elle n'est pas certifiée par Apple.

## Matériel

C'est la carte de capture qui décide si vous enregistrez réellement en 10 bits 4:2:2, car elle doit fournir du 10 bits 4:2:2 au PC.

**Cartes.** Le moyen le moins cher de capturer de la 4K 10 bits 4:2:2 sur un PC est une carte Blackmagic Design Intensity ou DeckLink, ou une carte Magewell Pro Capture. Les cartes AJA et Datapath le font aussi, mais à un prix bien plus élevé.

**P210 dans OBS.** OBS ne peut pas capturer le format de pixel P210 (10 bits 4:2:2) avec sa propre source Video Capture Device, qui s'arrête au 10 bits 4:2:0. Les cartes Blackmagic (v210) fonctionnent déjà dans OBS grâce à sa source DeckLink. StudioEncoder ajoute la prise en charge des cartes Magewell et de toute autre carte qui sort du P210, ce qu'OBS n'a jamais eu. Utilisez la source **SE: 10-bit 4:2:2 capture (P210)**.

**Cartes PCIe.** La carte a besoin d'un emplacement câblé en **x4** pour fonctionner à pleine capacité. Ne la laissez pas partager ses lignes avec un connecteur M.2 ou des ports SATA, ce qui peut ramener l'emplacement à x2 ou x1. Consultez le manuel de votre carte mère.

**Cartes USB.** La 4K30 en 10 bits 4:2:2 demande environ 5,3 Gbit/s en v210 compacté, ou 8,0 Gbit/s en P210. Une liaison USB 3.0 à 5 Gbit/s n'en transporte en pratique que 3,5 à 4, elle ne suffit donc pas. Il faut un vrai port USB 3.2 Gen 2 à 10 Gbit/s, un câble adapté et une carte qui sort du 10 bits 4:2:2, et la 4K30 en P210 y tiendra tout juste. La 4K60 ne passe sur aucune liaison USB. La plupart des cartes de capture USB n'offrent que du 4:2:2 8 bits ou du 4:2:0 10 bits ; vérifiez donc les formats de sortie d'une carte avant de l'acheter.

## Encodeurs

| Encodeur dans OBS | Implémentation | Remarques |
|---|---|---|
| SE: ProRes (AW+, fast) | AW+ : `prores_aw` de FFmpeg intégré au plugin, avec un arrondi corrigé | **Recommandé.** Léger pour le CPU : sur les CPU avec AVX2, sa part est d'environ la moitié de celle du prores_aw de FFmpeg, et il revient au C pur (toujours plus rapide que FFmpeg) sur les CPU anciens ou d'entrée de gamme. Proxy garde les détails fins ; le prores_aw de FFmpeg rend Proxy flou. |
| ProRes-compatible (Rust / FilmCraft) (experimental) | Encodeur Rust [FilmCraft](https://github.com/storytold/filmcraft) | Expérimental. Forte utilisation du CPU en 4K. |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | Expérimental. Environ 2,5 fois le CPU de prores_aw en 4K (30 % contre 12 % d'un portable à 22 threads) et des fichiers environ un tiers plus gros au profil 422 sur des images de caméra en direct, sans avantage de qualité par bit. Code très jeune (0.1). |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | Fichiers plus petits sur le GPU. Nécessite une carte de la **série RTX 50**. Voir ci-dessous. |
| HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC) | FFmpeg `hevc_nvenc` | Pour les cartes NVIDIA **antérieures à la série RTX 50** (pas de H.264 10 bits sur celles-ci) : **4:2:0** 10 bits, chaque image est une image clé, un réglage de débit. Voir ci-dessous. |
| HEVC 10-bit Intra (AMD) | FFmpeg `hevc_amf` | **4:2:0** 10 bits, chaque image est une image clé, sur un GPU Radeon, avec quatre préréglages de qualité. Voir ci-dessous. |

Profils : Proxy (`apco`), LT (`apcs`), 422 (`apcn`), HQ (`apch`).

### Choisir les encodeurs affichés (Tools > StudioEncoder)

La liste d'encodeurs d'OBS reste courte par défaut. **Tools > StudioEncoder** ouvre une fenêtre qui liste tous les encodeurs de ce plugin en trois sections : **Recommended (10-bit 4:2:2)** (AW+ et l'encodeur H.264 RTX 50), **10-bit 4:2:0** (les encodeurs HEVC NVIDIA et AMD) et **Experimental** (FilmCraft et OxideAV, désactivés par défaut). Les deux dernières sont des groupes repliables, fermés au départ ; cliquez sur la petite flèche pour en ouvrir un. Cochez ceux que vous voulez et appuyez sur OK ; le changement prend effet après le redémarrage d'OBS.

<p align="center"><img src="../images/settings-window.png" alt="La fenêtre de réglages de StudioEncoder (Tools > StudioEncoder) dans OBS : une liste d'encodeurs avec des cases à cocher et des groupes repliables 10-bit 4:2:0 et Experimental" width="420"></p>

Un encodeur désactivé est seulement masqué dans la liste déroulante. Un profil qui l'utilise déjà continue de fonctionner. Tous les encodeurs de ce plugin commencent par « SE: » dans la liste d'encodeurs d'OBS, pour être faciles à repérer.


### Pourquoi AW+ (prores_aw corrigé)

Le prores_aw de FFmpeg divise chaque coefficient de détail par son pas de quantification et **tronque vers zéro**, si bien que tout ce qui est plus petit qu'un pas entier est perdu. En Proxy, où les pas sont grossiers, cela supprime la texture fine : visages et tissus paraissent baveux. L'encodeur d'Apple utilise la même matrice Proxy et le même quantificateur, mais conserve ce détail. AW+ est prores_aw porté dans le plugin avec un arrondi au lieu de la troncature (décalage par profil : Proxy 4/8, LT 3/8, 422 3/8, HQ 2/8 d'un pas, choisis pour la meilleure qualité par bit).

Mesuré sur un clip de caméra 4K (PSNR, plus c'est haut mieux c'est) :

| Profil | FFmpeg prores_aw | AW+ | Apple ProRes |
|---|---|---|---|
| Proxy | 74 Mbit/s, 54.0 dB | **94 Mbit/s, 56.2 dB** | 96 Mbit/s, 56.5 dB |
| LT | 170 Mbit/s, 59.7 dB | 198 Mbit/s, 61.6 dB | 235 Mbit/s, 62.2 dB |
| HQ | 492 Mbit/s, 71.1 dB | 560 Mbit/s, 73.5 dB | 520 Mbit/s, 72.2 dB |

Avec l'arrondi désactivé, AW+ produit des fichiers identiques octet pour octet à ceux du prores_aw de FFmpeg (vérifié sur les quatre profils).
Les encodeurs GPU vérifient votre carte graphique au démarrage d'OBS. Si la carte ne peut pas en faire tourner un (une carte NVIDIA antérieure à la série RTX 50 pour l'encodeur H.264, aucune carte NVIDIA avec HEVC 10 bits pour l'encodeur HEVC NVIDIA, aucune Radeon pour l'encodeur AMD), cet encodeur est masqué de la liste et affiché en grisé dans cette fenêtre.

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

Une alternative à ProRes qui économise de la place, encodée sur le GPU. Chaque image est une image clé (all-intra), profil High 4:2:2, 10 bits, débit constant : il se monte donc comme du ProRes, mais en plus petit, au prix d'une légère perte de qualité.

- **Nécessite un GPU NVIDIA de la série RTX 50** (Blackwell) et un pilote récent. Les cartes NVIDIA plus anciennes ne peuvent encoder ni du H.264 10 bits ni du 4:2:2 ; sur celles-ci l'enregistrement ne démarre pas et OBS affiche une erreur. Utilisez-y ProRes (prores_aw).
- **Bitrate** : 100 Mbit/s (petit, environ la qualité de ProRes LT), **200 (par défaut**, meilleur que ProRes LT, environ la moitié de la taille de ProRes 422), 300 (élevé). Au-delà de 400, cela n'apporte rien.
- **Speed** : Quality (p5, par défaut) ou Fast (p1, qualité légèrement inférieure).
- Les images vont au GPU en P216 sans conversion. Au plus 2 images sont en cours simultanément, car les cartes NVIDIA grand public limitent le nombre de sessions d'encodeur.
- Lors d'un test FFmpeg sur une RTX 5080 (images de caméra 4K 29,97), cela tournait à environ 77 fps en p5 et 101 fps en p1.

### HEVC 10-bit 4:2:0 Intra (NVIDIA NVENC)

Pour les cartes NVIDIA **antérieures à la série RTX 50** (la GTX série 10 et plus récentes devraient fonctionner ; testé sur une RTX 4070 Laptop). Ces cartes n'ont ni mode H.264 10 bits ni 4:2:2 ; c'est donc du HEVC (Main 10) en **4:2:0 10 bits** : le détail de couleur vaut la moitié de ce que garde ProRes 422, mais chaque image est une image clé, si bien qu'on le coupe et le parcourt comme du ProRes, avec des fichiers plus petits. Sur une carte RTX 50, vous avez aussi l'encodeur H.264 4:2:2 ci-dessus, qui conserve le détail de couleur.

- **Settings > Advanced > Color Format: P010** (pas P216), Rec. 709, Limited. L'image va au GPU telle qu'OBS la produit, sans conversion.
- **Bitrate** (30 à 400 Mbit/s, 150 par défaut) et **Speed** (Quality p5, ou Fast p1). Lors d'un test sur un clip de caméra 4K (qualité de luma par rapport à la source) : 100 Mbit/s a mesuré 57,6 dB, mieux que ProRes Proxy (53,7 dB à 95) ; 150 a mesuré 59,4 dB, à peu près comme AW+ LT (59,6 dB à 200) avec 70 % de ses bits ; 200 a mesuré 60,8 dB. Le préréglage Fast est à peine moins bon en qualité et à peu près aussi rapide sur ce GPU.
- Les enregistrements sont en HEVC Main 10 dans du MOV avec l'étiquette `hvc1` (toutes les images sont des images clés, décodées sans erreur dans FFmpeg). Pas encore essayé dans Resolve ou Premiere.
- Au plus 2 images sont encodées en même temps, car les cartes NVIDIA grand public limitent le nombre de sessions d'encodeur. Environ 15 ms par image 4K sur la RTX 4070 Laptop ; OBS a utilisé environ 7 % du CPU (dont environ 5 % pour la scène elle-même).

### HEVC 10-bit 4:2:0 Intra (AMD)

Pour les utilisateurs de Radeon. L'encodeur vidéo d'AMD ne sait pas du tout faire du 4:2:2 (et son encodeur H.264 ne sait pas faire du 10 bits) ; c'est donc du **4:2:0 10 bits** : le détail de couleur vaut la moitié de ce que garde ProRes 422. Chaque image est une image clé, si bien que chaque image peut être coupée et parcourue sans décoder ses voisines, comme avec ProRes, avec des fichiers bien plus petits (High fait environ un tiers de ProRes 422 sur le clip de test). Le « AMD HW H.265 (HEVC) » d'OBS peut aussi enregistrer en 10 bits, mais il demande un GOP normal et un réglage du débit ; cet encodeur remplace cela par un seul choix, **Quality**.

- **Settings > Advanced > Color Format: P010** (pas P216), Rec. 709, Limited.
- **Quality** est constante, la taille du fichier dépend donc de l'image. Tailles typiques pour des images de caméra 4K 29,97 :

| Préréglage | Taille | PSNR de luma sur un clip de caméra 4K |
|---|---|---|
| Small | environ 30 Mbit/s | 52,4 dB |
| Standard | environ 70 Mbit/s | 55,3 dB (ProRes LT : 56,2 dB à 169 Mbit/s) |
| **High** (par défaut) | environ 120 Mbit/s | 56,7 dB |
| Maximum | environ 170 Mbit/s | 57,8 dB (le plafond de l'encodeur) |

  Les valeurs de PSNR comparent avec le clip de caméra converti en 4:2:0, elles sont donc équitables pour cet encodeur ; ProRes conserve une couleur 4:2:2 que celui-ci ne peut pas. Des images bruitées donnent des fichiers plus gros au même préréglage.
- Mesuré sur une Radeon RX 9070 XT : environ 3,8 ms par image 4K, aucune image répétée en 4K 29,97. L'encodeur AMD renvoie chaque paquet avec une image de retard ; la dernière image avant l'appui sur Stop est donc perdue.
- Les enregistrements sont en HEVC Main 10 dans du MOV (toutes les images sont des images clés, horodatages réguliers, décodés sans erreur dans FFmpeg). Pas encore essayé dans Resolve ou Premiere.

## Capture 10 bits 4:2:2 (P210), Windows

La source Video Capture Device d'OBS ne peut pas faire entrer de 10 bits 4:2:2 : sa capture DirectShow s'arrête à P010 (10 bits 4:2:0). Certaines cartes de capture, comme Magewell Pro Capture, offrent du 10 bits 4:2:2 en P210. StudioEncoder ajoute une source pour elles : **Sources > + > SE: 10-bit 4:2:2 capture (P210)**.

- Choisissez la carte sous Device, ou laissez « First device that offers P210 ».
- Réglez Resolution et Frame rate pour correspondre au signal HDMI de la caméra (par défaut 3840x2160, 29.97).
- Réglez **Settings > Video > Advanced > Color Format sur P216** pour que les 10 bits arrivent à l'encodeur. Avec P216, OBS mélange la scène en virgule flottante 16 bits.
- Ajoutez le son de la carte avec une source Audio Input Capture séparée.

Les images sont converties de P210 en I210 pour OBS : un décalage sans perte et une séparation des plans, environ 2,4 ms par image 4K sur un Ryzen 9 9950X3D. Testé avec une Magewell Pro Capture HDMI 4K en 3840x2160 29.97 : aucune image répétée, et les échantillons portent de vraies valeurs 10 bits. `tools/p210-probe.cpp` est un petit programme en ligne de commande qui vérifie si la sortie P210 d'une carte contient de vraies données 10 bits.

## Configuration requise

- Windows 10/11 x64
- OBS Studio 33.0.x (compilé avec 33.0.0 ; testé sur 33.0.1)

## Installation

Fermez OBS, puis copiez le dossier `obs-studioencoder` de l'archive zip de la version :

- **OBS installé :** dans `C:\ProgramData\obs-studio\plugins\obs-studioencoder\`. `ProgramData` est masqué ; saisissez le chemin dans l'Explorateur de fichiers.
- **OBS portable :** dans `<OBS folder>\plugins\obs-studioencoder\`, à côté de `bin`, `data` et `config`.

Le dossier doit contenir `obs-studioencoder.dll` et `data\locale\`. Dans OBS, Tools → Plugin Manager doit lister « StudioEncoder », et Tools → StudioEncoder ouvre ses réglages.

N'utilisez pas les anciens dossiers `obs-plugins\64bit` ou `data\obs-plugins`. OBS 33 considère ces emplacements comme obsolètes.

## Réglages

1. Settings → Advanced : **Color Format P216** (**P010** pour les deux encodeurs HEVC), Color Space Rec. 709, Color Range Limited.

   <img src="../images/obs-advanced-colour-format.png" alt="OBS Settings, onglet Advanced : Colour Format P216 (16-bit, 4:2:2, 2 planes), Rec. 709, Limited, mis en évidence" width="560">

2. Settings → Video : réglez le canevas (Base) et la résolution de sortie (Output) sur la taille de votre enregistrement et la fréquence d'images sur celle de votre source. La capture d'écran montre 3840x2160 à 29.97.

   <img src="../images/obs-video.png" alt="OBS Settings, onglet Video : résolution Base et Output 3840x2160, 29.97 fps" width="560">

3. Settings → Output : Output Mode **Advanced**, onglet Recording, Type **Standard**, Recording Format **MOV**.
4. Video Encoder : l'un des encodeurs ci-dessus (il commence par « SE: »). La ligne sous Encoder Settings, « Required video settings », rappelle le Color Format, le Color Space et le Color Range dont cet encodeur a besoin.

   <img src="../images/obs-output-encoder.png" alt="OBS Settings, onglet Output, Recording : type Standard, QuickTime (.mov), Video Encoder SE: ProRes (AW+, fast) mis en évidence" width="560">

   Dans ses réglages :
   - **Profile** (encodeurs ProRes) : Proxy, LT, 422 ou HQ. **Bitrate** et **Speed** (encodeur NVENC) : voir ci-dessus.
   - **Frames in flight** : 0 (automatique, recommandé). Les images sont encodées en parallèle, une par cœur de CPU, selon les besoins.
   - **Automatic sizing** (seulement avec Frames in flight à 0) : *Grow when frames are late* (par défaut) ou *Size from measured frame cost* (expérimental). Le second démarre avec 4 workers, chronomètre une image, puis fixe en une fois le nombre de workers nécessaire. Cela utilise moins de mémoire pour les encodeurs légers et monte plus vite en charge pour les lourds. La charge CPU est la même dans les deux cas.
   - **On overload** : *Warn in the log* ou *Stop recording*.
   - **Log verbosity** : Info affiche les temps et le réglage automatique.

## Surcharge

Si un encodeur ne suit pas, OBS répète l'image précédente pour conserver la ligne de temps, et l'enregistrement semble figé à ces endroits. Le plugin compte ces images répétées pour chaque enregistrement (il remarque quand OBS lui donne deux fois de suite la même image) :
- Il écrit un avertissement `OVERLOAD` dans le journal pour chaque seconde où cela se produit, et un résumé à la fin de l'enregistrement.
- Avec **On overload: Stop recording**, il met fin à l'enregistrement après 3 secondes de surcharge consécutives et affiche une erreur.

Si vous voyez des avertissements de surcharge, passez à l'encodeur prores_aw ou à un profil plus léger.

## Limites connues

- L'entrée doit être en P216 (P010 pour les encodeurs HEVC). OBS 33 n'offre aucun autre format de sortie 10 bits 4:2:2.
- Les images capturées après l'appui sur Stop, mais encore en cours d'encodage, sont écartées. C'est attendu ; l'enregistrement s'arrête au point d'arrêt.
- Le MOV hybride n'est pas testé.
- Windows. Linux compile et fonctionne (testé sur Ubuntu et CachyOS ; voir `docs/HANDOFF.md`, « Build and test on Linux » et « Linux results »).

## Compilation

Les instructions de compilation (chaîne d'outils et étapes) ne figurent que dans le [README en anglais](../../README.md#build).

## Licence

GPL-2.0-or-later (voir `LICENSE`), comme l'exigent les plugins OBS. Code tiers :
- FFmpeg (LGPL), tel qu'il est livré avec OBS.
- Crate ProRes de FilmCraft (MIT OR Apache-2.0, utilisé sous MIT).
- AW+ (`src/prores-aw.c`) : un portage de l'encodeur prores_aw de FFmpeg, de la DCT directe et des tables ProRes (LGPL 2.1 ou ultérieure ; Anatoliy Wasserman, Konstantin Shishkov, Independent JPEG Group), utilisé sous la GPL.
- `oxideav-prores` et `oxideav-core` (MIT), l'encodeur ProRes OxideAV.
