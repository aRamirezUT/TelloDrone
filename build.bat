@echo off

rem This will use VS2015 for compiler
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

cl /I "C:\Users\Shado\OneDrive\Documents\opencv\build\include" /nologo /W3 /O2 /fp:fast /Fedemo.exe gui.cpp user32.lib /link /incremental:no /LIBPATH:"C:\Users\Shado\OneDrive\Documents\opencv\build\x64\vc16\lib" opencv_world470.lib
pause