Loader example (English)
========================

Contents
--------
  loader.txt                 configuration of the loader (concatenated to the module)
  payload\                   files that go into the external archive
    app.sfx.config.txt       actions after extraction (InstallPath, RunProgram)
    hello.txt                sample payload file
    data\settings.ini        sample nested file
  build_loader.bat           builds payload.7z and loader.exe
  loader.exe, payload.7z     demo build (created by build_loader.bat)

Quick start
-----------
1. Put your files into payload\ (subfolders are kept).
2. Edit payload\app.sfx.config.txt if you need another install path or command.
3. Edit loader.txt: Title, GUIMode and Password (the archive is encrypted with it).
4. Run build_loader.bat (double click or from a console).
5. Run:  loader.exe payload.7z

The installer does not set InstallPath: the module extracts into its own
UNIQUE temp folder and runs RunProgram from that folder. To use a fixed
folder instead, set InstallPath in app.sfx.config.txt.

Notes
-----
- 7-Zip must be installed (or set SZ_7ZIP to the full path of 7z.exe).
- Keep the modules (..\7zsd_*.sfx) in the set root; this example uses
  ..\7zsd_LZMA2.sfx. To switch to x64, edit MODULE in build_loader.bat
  to ..\7zsd_LZMA2_x64.sfx and rebuild.
- The archive is LZMA2 with encrypted names (-mhe=on). If you use the
  Deflate or PPMd modules, build the archive with -mhc=off.
- Password rules: ASCII letters/digits; avoid " and !; double backslashes.
- Alternative author mode: instead of Password="..." set
  SfxAuthor="Some text" in loader.txt. The module derives a code
  (XXXXX-XXXXX-XXXXX-XXXXX-XXXXX) and uses it as the archive password.
  In this case encrypt the archive manually with that code
  (build_loader.bat reads only Password).
