@echo OFF
pushd ..\..\build\win
clang -gcodeview -O0 -w -c ../../src/renderer/renderer.c -I../../../cglm/include -I../../../freetype/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include -o renderer.obj
clang -gcodeview -shared renderer.obj -L../../lib/win -lfreetype.lib -lSDL3.lib -o renderer.dll
popd

REM pushd ..\..\build\win
REM
REM clang -gcodeview -O0 -fno-omit-frame-pointer -fexceptions -c ../../src/renderer/renderer.c ^
REM     -I../../../cglm/include -I../../../freetype/include -I../../../glad/include ^
REM     -I../../../cglm/include -I../../../SDL/include ^
REM     -o renderer.obj
REM
REM clang -shared renderer.obj ^
REM     -L../../lib/win -lfreetype.lib -lSDL3.lib ^
REM     -Xlinker /DEBUG ^
REM     -o renderer.dll
REM
REM popd
