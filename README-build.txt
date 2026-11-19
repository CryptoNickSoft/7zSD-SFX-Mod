Сборка 7zSFX (7-Zip SFX Modified Module) 1.8.0 build 4000
Автор модификации: Олег Н. Щербаков (mulder), 7zsfx.info
Дата сборки: 2026-11-19

ИСХОДНИКИ
  Архив: 7zsd_src_170_3900.7z
  Откуда: https://github.com/OlegScherbakov/7zSFX (папка Source)
  Состав: 7-Zip 9.22 (CPP, C, Asm) + исходники модифицированного SFX-модуля (7zSfxMod)

ИНСТРУМЕНТЫ
  Microsoft Visual Studio 2022 Build Tools (MSVC 14.44, MASM ml.exe, Windows SDK 10.0.26100)
  nmake + cl + link + rc + ml, целевые платформы x86 и x64

ПРАВКИ, НЕОБХОДИМЫЕ ДЛЯ СБОРКИ СОВРЕМЕННЫМ MSVC
  1. sources/7zSfxMod.rc подключает "afxres.h" (заголовок MFC). В Build Tools MFC нет,
     поэтому в INCLUDE добавлена папка shim с минимальным afxres.h
     (windows.h + #define IDC_STATIC -1). Сами исходники модуля не изменялись.
  2. В makefile из LFLAGS убрана опция -opt:NOWIN98: линковщик VS 2022 её не принимает
     (LNK1117). На код и поведение модуля это не влияет.

СБОРКА
  build_port.cmd <x86|x64> <All|Base|LZMA|LZMA2|Deflate|PPMd>
  (build.cmd — алиас, форвардит на build_port.cmd)
  Внутри: vcvarsall x86/x64, затем
    nmake SFX_CRYPTO=1 SFX_VOLUMES=1 _SFX_MAINTAINER=1 CPU=<x86|x64> \
          MODULE_SUBNAME=<...> COMPRESS_... build
  Набор флагов совпадает с официальным make_bin.cmd / Make7zsd.cmd.
  Готовые модули: port_2606\output\*.sfx, копия — dist\.

ПОЛУЧЕННЫЕ МОДУЛИ (12 файлов, несжатые)
  7zsd.sfx, 7zsd_x64.sfx                 LZMA (базовый модуль)
  7zsd_LZMA.sfx, 7zsd_LZMA_x64.sfx       LZMA (имя как в официальном наборе)
  7zsd_LZMA2.sfx, 7zsd_LZMA2_x64.sfx     LZMA/LZMA2
  7zsd_Deflate.sfx, ..._x64.sfx          Copy/Deflate
  7zsd_PPMd.sfx, ..._x64.sfx             Copy/PPMd
  7zsd_All.sfx, ..._x64.sfx              Copy/BCJ/BCJ2/Delta/LZMA/LZMA2/Deflate/PPMd
  Ресурс версии во всех файлах: 1.8.0.4000.

ПРОВЕРКА (smoke.ps1)
  Для каждого из 12 модулей собран SFX-архив с соответствующим методом сжатия,
  конфиг: InstallPath на отдельную папку + RunProgram="cmd.exe /c copy /y hello.txt copied.txt".
  Результат: у всех модулей код возврата 0, распакованы оба файла полезной нагрузки
  и создан copied.txt (т.е. работает и распаковка, и запуск программы).
  Разрядность PE-заголовков соответствует имени (_x64 — AMD64, остальные — i386).

ОСОБЕННОСТЬ
  Модули Deflate и PPMd не содержат декодера LZMA, поэтому архив для них нужно
  создавать без сжатия заголовка: 7z a -m0=deflate -mhc=off (аналогично для PPMd).
  С заголовком, сжатым LZMA по умолчанию, и наш, и официальный модуль завершаются
  ошибкой — это не дефект сборки.

ФАЙЛЫ СБОРКИ
  port_2606\7zSfxMod\   активные исходники (7-Zip + 7zSfxMod)
  shim\afxres.h    заглушка заголовка MFC для rc.exe
  build_port.cmd   сборка одного модуля (build.cmd — его алиас)
  smoke.ps1        сквозной тест всех модулей
  dist\                 готовые модули (сборка без изменений исходников)
  7zsd_170_3900.7z      архив с готовыми модулями

ПАТЧ "PASSWORD" (см. README-password.md)
  В исходники добавлен параметр конфигурации Password="..." — пароль для
  зашифрованного архива, который модуль применяет сам и не спрашивает у
  пользователя. Затронуты два файла: sources/7zSfxMod.h (CFG_PASSWORD) и
  sources/main.cpp (SetConfigVariables). Ключ -p по-прежнему имеет приоритет.
  Патченные модули: dist_password\ и 7zsd_180_4000_password.7z;
  тест — smoke_password.ps1 (все 12 модулей проходят).

ПАТЧ "LOADER" (внешний архив, см. README-loader.md)
  Добавлено: параметр Archive (необязательный путь к внешнему архиву) и
  прием пути из командной строки — loader.exe archive.7z или перетаскивание.
  Переданный аргумент вырезается из строки, чтобы не попасть в RunProgram.
  Затронуты: 7zSfxMod.h (CFG_ARCHIVE), archive.h/archive.cpp (Reopen),
  main.cpp (разбор argv, внешний режим, пропуск повторного чтения оверлея).
  Комплект из 12 модулей: dist_loader\ и 7zsd_180_4000_loader.7z;
  тест — smoke_loader_all.ps1 (все 12 проходят, LOADER SET: ALL PASSED).

ПАРАМЕТР "SfxAuthor" (код автора)
  Логика взята из официальных собранных модулей (в них есть алфавит
  "123456789ABCDEFGHJKMNPQRSTUVWXYZ" и таблица констант SHA-256).
  Алгоритм:
    1) SHA-256 от строки SfxAuthor в UTF-16LE (длина*2 байта);
    2) key[i] = digest[i] XOR digest[i+16], i = 0..15;
    3) для позиции i (0..24): bit = i*5; value = (key[bit/8] >> (bit%8)) & 31;
    4) символ = alphabet[value]; дефис перед каждой группой из 5.
  Особенность: 5 бит берутся из ОДНОГО байта, при сдвиге >3 старшие биты
  теряются — это не обычная упаковка Base32, поэтому стандартные схемы
  не совпадали.
  Проверено на всех парах (gora-sah, Homer, Homeq, Homer1, Kalina,
  Vochik-ghg) — совпадение полное (автономный тест и модуль).
  Реализация: sources/SfxAuthor.cpp. Ключ -sfxauthor удалён — его нет
  и в оригинальном модуле; код нигде не отображается, только
  подставляется как пароль.

  КОД КАК ПАРОЛЬ АРХИВА (как в официальном модуле)
  Если задан SfxAuthor и не задан Password, модуль подставляет код
  (XXXXX-XXXXX-XXXXX-XXXXX-XXXXX) в качестве пароля архива — диалог
  не показывается. Так официальный 7zsd открывает архив, зашифрованный
  паролем-кодом; проверено на 7zsd_LZMA2.sfx из репозитория:
  архив с паролем "<removed>" и SfxAuthor="Homer"
  распаковывается молча (и с -mhe=on, и без). Без SfxAuthor — запрос
  пароля, как раньше.
  Наша приоритетность: -p / Password= -> код SfxAuthor -> диалог.
  В официальном 1.7.3900 параметра Password нет, поэтому там код
  используется всегда, когда задан SfxAuthor.
  Затронуты: archive.h/archive.cpp (CPassword::SetAuthorCode,
  GetTextPassword) и main.cpp (SetConfigVariables).
  Проверено: classic и loader-режим — exit 0; при заданном Password
  архив под ним открывается, архив под кодом — нет (приоритет Password).






