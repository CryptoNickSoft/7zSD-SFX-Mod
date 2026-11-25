7zSFX loader modules 1.8.0.4000 (English set)
============================================

Contents
--------
  7zsd.sfx              x86  Copy/BCJ/BCJ2/Delta/LZMA
  7zsd_x64.sfx          x64  same
  7zsd_LZMA.sfx         x86  LZMA
  7zsd_LZMA_x64.sfx     x64  LZMA
  7zsd_LZMA2.sfx        x86  LZMA/LZMA2
  7zsd_LZMA2_x64.sfx    x64  LZMA/LZMA2
  7zsd_Deflate.sfx      x86  Copy/Deflate
  7zsd_Deflate_x64.sfx  x64  Copy/Deflate
  7zsd_PPMd.sfx         x86  Copy/PPMd
  7zsd_PPMd_x64.sfx     x64  Copy/PPMd
  7zsd_All.sfx          x86  all methods
  7zsd_All_x64.sfx      x64  all methods

  example\              ready-to-build sample (see example\README.txt)
  7zSD_EN_plus.chm      English help (HTML Help, includes loader mode)
  README.txt            this file

Features
--------
1. External archive. The module no longer has to contain the archive:

       loader.exe payload.7z
       loader.exe "D:\distr\app v2.7z"
       (or drag and drop the archive onto loader.exe)

   The argument is removed from the command line, so it is not forwarded
   to the program started after extraction.

2. Password from the loader config:

       Password="ChangeMe123"

   The archive is opened without a prompt. The command line switch -p
   still overrides everything.

3. Install actions inside the archive. Put a file named *.sfx.config or
   *.sfx.config.* into the archive, for example app.sfx.config.txt:

       ;!@Install@!UTF-8!
       Title="My application"
       GUIMode="2"
       InstallPath="%PROGRAMFILES%\\MyApp"
       RunProgram="setup.exe /S"
       ;!@InstallEnd@!

   The file is read from the archive and is not extracted to disk.

  Single instance
  ---------------
    SingleInstance="1"  a second start exits at once (exit code 0) without
                        extracting or deleting anything while a copy works
                        with the same InstallPath folder
    SingleInstance="2"  a second start activates the window of the first
                        copy and exits
    SingleInstance="0"  previous behaviour (default)

    %SfxVarPID% (or %%PID%%) in InstallPath, RunProgram, Delete and other
    expanded parameters is the current process id, so each copy can extract
    into its own folder.

    The extraction folder is deleted with a limited number of attempts
    (about one second at most): free files are removed, a locked file and
    the folder remain; there is no one-minute wait any more.

Fixes in this build (concurrency of two copies)
----------------------------------------------
- The extraction temp folder is now UNIQUE: its name is
  <PID in base36>.<sequence in base36> - short, no leading zeros,
  no extra suffixes. Two simultaneously running copies of the SFX
  no longer work into the same folder, and one copy''s cleanup can no
  longer delete files the other copy is still using.
- When InstallPath is not set the module still uses a temp folder,
  but the folder name is unique per run.
- File deletion no longer force-deletes files held by another process:
  SfxIsFileBusyForDelete skips such files instead of breaking the
  running program.
- If %TEMP% is unavailable, a fallback folder %LOCALAPPDATA%\7zSFX_Temp
  is used; the fallback folder is auto-removed once it becomes empty.

How it works
------------
  loader.exe = 7zsd_LZMA2.sfx + loader.txt   (auth parameters only)
  payload.7z = files + app.sfx.config.txt    (install scenario)

The loader reads its overlay config (e.g. Password), opens the
archive given in the command line, decrypts it, reads the compressed
config from inside the archive, applies InstallPath/RunProgram and
extracts the files.

Priority of password sources:
   -p command line switch  ->  Password= in config  ->  password prompt.

Requirements
------------
- Windows; 7-Zip installed for building archives (or set SZ_7ZIP).
- Modules are unsigned; sign them before public distribution if needed.

Building archives
-----------------
   7z a -t7z -m0=lzma2 -mx=9 -mhe=on -p"ChangeMe123" payload.7z .\files\*

- The compression method must be supported by the chosen module.
- -mhe=on encrypts file names as well (optional).
- For 7zsd_Deflate.sfx and 7zsd_PPMd.sfx add -mhc=off: these modules have
  no LZMA decoder and cannot read a compressed archive header.

Security notes
--------------
- The external payload.7z can be replaced. Verify a hash or signature
  before extraction if that matters.
- Password= is stored in plain text in loader.exe (-sfxconfig shows it).

Building the modules from sources
---------------------------------
    build_port.cmd x86|x64 Base|LZMA|LZMA2|Deflate|PPMd|All

Sources: 7zsfx\port_2606\7zSfxMod\sources (main.cpp,
archive.cpp, archive.h, 7zSfxMod.h). Original unmodified sources:
7zsd_src_170_3900.7z.

Version: 1.8.0.4000 (module resource), 7-Zip 26.06 core, static CRT.
