# 7zSFX - modified module (1.8.0.4000)

Модифицированный 7-Zip SFX-модуль (`7zSfxMod`), собранный на базе
7-Zip 26.06, с доработками исходников.

## Что это

SFX-модуль для 7-Zip, который позволяет создавать самораспаковывающиеся
архивы с дополнительными возможностями:

- параметр `Password="..."` — пароль архива берётся из конфига,
  диалог ввода не показывается;
- параметр `SfxAuthor="..."` — код автора (как в официальном модуле);
- параметр `Archive="..."` — внешний архив, лоадер-режим
  (`loader.exe payload.7z`), без склеивания архива с exe;
- конфигурация установки внутри архива (`*.sfx.config*`);
- **уникальная временная папка распаковки** `%TEMP%\<pid>.<seq>`
  (base36 от PID и счётчика) — две одновременные копии не конфликтуют,
  и очистка одной не задевает файлы другой;
- **защита от принудительного удаления занятых файлов** при очистке
  (`SfxIsFileBusyForDelete`) — файл, открытый другим процессом, не сносится;
- запасная папка `%LOCALAPPDATA%\7zSFX_Temp`, если `%TEMP%` недоступна.

Модули собраны со статическим CRT и не требуют Visual C++ Redistributable.

## Структура

```
7zSfxMod/           исходники модуля
  sources/          исходники 7zSfxMod (с правками)
  configs/          примеры конфигураций
  build/            makefile + сборочные .cmd
7-Zip/              SDK 7-Zip 26.06 (зависимость сборки)
shim/               заглушка afxres.h (MFC-заголовок отсутствует в VS Build Tools)
test/               тесты (test_all.ps1, test_features.ps1)
build.cmd           сборка модуля (без жёстких путей)
README-*.md/.txt    документация доработок
LICENSE.md          лицензия
```

## Сборка

Требования: Visual Studio 2022 Build Tools (MSVC), целевые x86/x64.

```
build.cmd x86 LZMA
build.cmd x64 All
(модули: All|Base|LZMA|LZMA2|Deflate|PPMd)
```

Готовые модули появляются в `7zSfxMod/build` (путь вывода задаётся makefile).

Подробнее о сборке и правках — в `README-build.txt`, `README-loader.md`,
`README-password.md`.

## Тесты

```
test\test_all.ps1        проверка всех модулей (распаковка + запуск)
test\test_features.ps1   SfxAuthor / Archive
```

## Лицензия

См. `LICENSE.md`. Код ядра 7-Zip распространяется в основном под
GNU LGPL 2.1+; файлы RAR- и часть BSD-файлов имеют отдельные условия
(см. `7-Zip/DOC/License.txt`). Модификация `7zSfxMod` — производная
доработка авторского модуля Oleg Scherbakov, распространяется на тех же
условиях GNU LGPL, если в исходнике явно не указано иное.

---

# English

## Overview

A modified 7-Zip SFX module (`7zSfxMod`) built on top of 7-Zip 26.06,
with source-level improvements.

## Features

An SFX module for 7-Zip that lets you build self-extracting archives
with extra capabilities:

- `Password="..."` — archive password taken from the config, no prompt;
- `SfxAuthor="..."` — author code (same as the official module);
- `Archive="..."` — external archive, loader mode (`loader.exe payload.7z`),
  no need to concatenate the archive into the exe;
- installation config inside the archive (`*.sfx.config*`);
- **unique extraction temp folder** `%TEMP%\<pid>.<seq>`
  (base36 of PID and a counter) — two simultaneous copies never collide,
  and cleanup of one never touches the other's files;
- **protection against force-deleting files in use** during cleanup
  (`SfxIsFileBusyForDelete`) — a file opened by another process is not removed;
- fallback folder `%LOCALAPPDATA%\7zSFX_Temp` when `%TEMP%` is unavailable.

Modules are built with a static CRT and do not require Visual C++ Redistributable.

## Layout

```
7zSfxMod/           module sources
  sources/          7zSfxMod sources (with modifications)
  configs/          example configurations
  build/            makefile + build .cmd scripts
7-Zip/              SDK 7-Zip 26.06 (build dependency)
shim/               afxres.h stub (MFC header absent from VS Build Tools)
test/               tests (test_all.ps1, test_features.ps1)
build.cmd           module build script (no hard-coded paths)
README-*.md/.txt    documentation of the changes
LICENSE.md          license
```

## Building

Requirements: Visual Studio 2022 Build Tools (MSVC), target x86/x64.

```
build.cmd x86 LZMA
build.cmd x64 All
(modules: All|Base|LZMA|LZMA2|Deflate|PPMd)
```

The resulting modules are placed in `7zSfxMod/build` (the output path is set
by the makefile).

See `README-build.txt`, `README-loader.md` and `README-password.md` for
build details and the changes made.

## Tests

```
test\test_all.ps1        checks every module (extraction + run)
test\test_features.ps1   SfxAuthor / Archive
```

## License

See `LICENSE.md`. The 7-Zip core is distributed mostly under
GNU LGPL 2.1+; RAR and some BSD files have separate terms
(see `7-Zip/DOC/License.txt`). The `7zSfxMod` modification is a derived
work of the author's module by Oleg Scherbakov, distributed under the same
GNU LGPL terms unless a source file states otherwise.