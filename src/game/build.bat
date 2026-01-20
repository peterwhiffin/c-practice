pushd ..\..\build\win
clang -g -c ../../src/game/game.c -I../../../cglm/include -I../../../freetype/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include -o game.obj
clang -shared game.obj -L../../lib/win -lSDL3.lib -o game.dll
popd
