@echo OFF
pushd build\win
REM clang -gcodeview -O0 -w -MTd -Wl,/nodefaultlib:libcmt -Wl,/defaultlib:libcmtd -o practice.exe ../../src/main.c -I../../../SDL/include -I../../../glad/include -I../../../cglm/include -I../../../freetype/include -L../../lib/win -lSDL3.lib -lfreetype.lib
clang -gcodeview -O0 -w -o practice.exe ../../src/main.c -I../../../SDL/include -I../../../glad/include -I../../../cglm/include -I../../../freetype/include -L../../lib/win -lSDL3.lib -lfreetype.lib
REM clang -gcodeview -O0 -w -o practice.exe ../../src/main.c -I../../../SDL/include -I../../../glad/include -I../../../cglm/include -I../../../freetype/include -L../../lib/win -lSDL3.lib -lfreetype.lib

REM cl /Felearnopengl /EHsc /Zi /DEBUG msvcrt.lib glfw3.lib user32.lib gdi32.lib shell32.lib ../src/*.cpp ../src/*.c -I../include 
REM /link /libpath:../lib/ assimp-vc143-mt.lib /NODEFAULTLIB:libcmt

REM clang-cl /Fepractice /EHsc /Zi /DEBUG msvcrt.lib user32.lib gdi32.lib shell32.lib ../../src/main.c -I../../../SDL/include -I../../../glad/include -I../../../cglm/include -I../../../freetype/include /link /libpath:../../lib/win SDL3.lib freetype.lib /NODEFAULTLIB:libcmt

popd
