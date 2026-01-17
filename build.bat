@echo off

if not exist "out" mkdir "out"

cl /std:c++20 ^
   /EHsc ^
   /DNOMINMAX ^
   /DWIN64 ^
   /I vendor\stb ^
   /I "C:\Program Files\msvc\VC\Tools\MSVC\14.50.35717\include" ^
   /I "C:\Program Files\msvc\VC\Tools\MSVC\14.50.35717\include\wrl" ^
   /I "C:\Program Files\msvc\Windows Kits\10\Include\10.0.26100.0\ucrt" ^
   /I "C:\Program Files\msvc\Windows Kits\10\Include\10.0.26100.0\um" ^
   /I "C:\Program Files\msvc\Windows Kits\10\Include\10.0.26100.0\shared" ^
   /I "C:\Program Files\msvc\Windows Kits\10\Include\10.0.26100.0\winrt" ^
   src\main.cpp ^
   lib\padInput.cpp ^
   /Fe:out\main.exe ^
   /link ^
   /LIBPATH:"C:\Program Files\msvc\VC\Tools\MSVC\14.50.35717\lib\x64" ^
   /LIBPATH:"C:\Program Files\msvc\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64" ^
   /LIBPATH:"C:\Program Files\msvc\Windows Kits\10\Lib\10.0.26100.0\um\x64" ^
   user32.lib ^
   gdi32.lib ^
   d2d1.lib ^
   dwrite.lib ^
   dxgi.lib ^
   ole32.lib
