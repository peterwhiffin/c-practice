@echo OFF
pushd ..\..\build\win
REM clang -gcodeview -O0 -w -c ../../src/game/game.c -I../../../cglm/include -I../../../freetype/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include -o game.obj
REM clang -gcodeview -shared game.obj -L../../lib/win -lSDL3.lib -o game.dll

clang -gcodeview -O0 -fno-omit-frame-pointer -fexceptions -c ../../src/game/game.c ^
    -I../../../cglm/include -I../../../freetype/include -I../../../glad/include ^
    -I../../../cglm/include -I../../../SDL/include ^
    -o game.obj

clang -shared game.obj ^
    -L../../lib/win -lSDL3.lib ^
    -Xlinker /DEBUG ^
    -o game.dll

popd
