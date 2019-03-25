@echo off

set CompilerFlags=/D_CRT_SECURE_NO_WARNINGS /Oi /Gm- /MT /Zi /GR- /EHa- /FC /W4 /WX /wd4201 /wd4505 /wd4100 /wd4189 /Fm /nologo
set LinkerFlags=/link /opt:ref
set Imports=user32.lib Gdi32.lib winmm.lib

pushd ..\build
cl %CompilerFlags% ..\code\main.c %LinkerFlags% %Imports%
REM cl %CompilerFlags% ..\code\win32_handmade.cpp %LinkerFlags% /subsystem:windows,5.1 %Imports%

popd