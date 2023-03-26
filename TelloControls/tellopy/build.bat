call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

rem This compiles telloc on windows.
rem run this script from the developer command prompt.
rem read the README.

rem :: set environment variables ::
set ffmpeg_include_dir="C:\Program Files\FFmpeg\include"
set ffmpeg_lib_dir="C:\Program Files\FFmpeg\lib"
set ffmpeg_dll_dir="C:\Program Files\FFmpeg\bin"
set build_python="true"
set python_dir="%userprofile%\AppData\Local\Programs\Python\Python311"

rem :: compile telloc ::
set SOURCES=telloc\video.c telloc\telloc_windows.c
set avcodec=%ffmpeg_lib_dir%\avcodec.lib
set avformat=%ffmpeg_lib_dir%\avformat.lib
set avutil=%ffmpeg_lib_dir%\avutil.lib
set swscale=%ffmpeg_lib_dir%\swscale.lib
cl /c /MT /Itelloc\ /I%ffmpeg_include_dir% %SOURCES% 
lib /OUT:telloc.lib /MACHINE:X64  video.obj telloc_windows.obj %avcodec% %avformat% %avutil% %swscale% ws2_32.lib
pause
rem :: compile test program ::
cl /c telloc/main_windows.c /Itelloc 
link main_windows.obj /out:test.exe /LIBPATH:"%CD%" telloc.lib user32.lib gdi32.lib

copy %ffmpeg_dll_dir%\avcodec*.dll .
copy %ffmpeg_dll_dir%\avformat*.dll .
copy %ffmpeg_dll_dir%\avutil*.dll .
copy %ffmpeg_dll_dir%\swscale*.dll .
copy %ffmpeg_dll_dir%\swresample*.dll .

pause
exit
IF NOT %build_python% == "false" (
    rem build the python library
    cl /c /MT /Itelloc /I%python_dir%\include tellopy.c
    link /dll /MACHINE:X64 /OUT:tellopy\libtellopy.pyd /LIBPATH:%python_dir%\libs tellopy.obj telloc.lib python3.lib
)

rem del *.obj
