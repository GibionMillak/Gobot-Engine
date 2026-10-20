# Built-in fonts

`SCsub` embeds the font files in the family subfolders using
`editor_fonts_builders.py`. The build generates `editor_fonts.gen.h` here;
do not edit it manually. `editor_fonts.cpp` configures these fonts for the editor.

- `baloo_2/*.ttf`:
  * Upstream: https://github.com/EkType/Baloo2
  * License: OFL-1.1
- `droid_sans/*.woff2`:
  * Upstream: https://android.googlesource.com/platform/frameworks/base/+/master/data/fonts/
  * Version: ? (pre-2014 commit when DroidSansJapanese.ttf was obsoleted)
  * License: Apache 2.0
- `inter/*.woff2`:
  * Upstream: https://github.com/rsms/inter
  * Version: v4.1 (e3a3d4c57d5ecc01453a575621882a384c1995a3, 2024)
  * License: OFL-1.1
- `jetbrains_mono/regular.woff2`:
  * Upstream: https://github.com/JetBrains/JetBrainsMono
  * Version: 2.304 (cd5227bd1f61dff3bbd6c814ceaf7ffd95e947d9, 2023)
  * License: OFL-1.1
- `noto_sans/bengali_*.woff2`:
  * Upstream: https://github.com/notofonts/bengali
  * Version: 3.011 (85d80394cbbbb798ca0a41c983902e6cf77be3a3, 2026)
  * License: OFL-1.1
- `noto_sans/devanagari_*.woff2`:
  * Upstream: https://github.com/notofonts/devanagari
  * Version: 2.006 (bb8d2566a1708ef2dcc6396ee2eb261a18967f76, 2024)
  * License: OFL-1.1
- `noto_sans/georgian_*.woff2`:
  * Upstream: https://github.com/notofonts/georgian
  * Version: 2.005 (c02e5483c2dd63c5cf223845010ebd6e6dc56aec, 2024)
  * License: OFL-1.1
- `noto_sans/hebrew_*.woff2`:
  * Upstream: https://github.com/notofonts/hebrew
  * Version: 3.001 (caa7ab0614fb5b37cc003d9bf3d7d3e765331110, 2024)
  * License: OFL-1.1
- `noto_sans/malayalam_ui_*.woff2`:
  * Upstream: https://github.com/notofonts/malayalam
  * Version: 2.104 (0fd65e553a6af3dc1c09ed39dfe8933e01c17b32, 2023)
  * License: OFL-1.1
- `noto_sans/oriya_*.woff2`:
  * Upstream: https://github.com/notofonts/oriya
  * Version: 2.006 (97abab82ec512f8a4a98c389352f194a03385ce2, 2024)
  * License: OFL-1.1
- `noto_sans/sinhala_*.woff2`:
  * Upstream: https://github.com/notofonts/sinhala
  * Version: 3.000 (032355e96de5bac83fd996535af3d13b1fbfeccf, 2025)
  * License: OFL-1.1
- `noto_sans/tamil_ui_*.woff2`:
  * Upstream: https://github.com/notofonts/tamil
  * Version: 2.004 (f34a08d1ae3fa810581f63410296d971bdcd62dc, 2023)
  * License: OFL-1.1
- `noto_sans/telugu_ui_*.woff2`:
  * Upstream: https://github.com/notofonts/telugu
  * Version: 2.005 (e97c3409a8347d68cccd06a82a68b418c315ee0c, 2023)
  * License: OFL-1.1
- `noto_sans/thai_*.woff2`:
  * Upstream: https://github.com/notofonts/thai
  * Version: 2.002 (f8b482c158650260bba5d5edba9da3e8bb7185b4, 2023)
  * License: OFL-1.1
- `open_sans/semi_bold.woff2`:
  * Upstream: https://github.com/googlefonts/opensans
  * Version: git (bd7e37632246368c60fdcbd374dbf9bad11969b6, 2023)
  * License: OFL-1.1
- `vazirmatn/*.woff2`:
  * Upstream: https://github.com/rastikerdar/vazirmatn
  * Version: 33.003 (83629f877e8f084cc07b47030b5d3a0ff06c76ec, 2022)
  * License: OFL-1.1

The WOFF2 fonts are converted from the unhinted `.ttf` sources using the
`https://github.com/google/woff2` tool.

Use UI font variant if available, because it has tight vertical metrics and good
for UI.
