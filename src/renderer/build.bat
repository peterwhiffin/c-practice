pushd ..\..\build\win
clang -g -c ../../src/renderer/renderer.c -I../../../cglm/include -I../../../freetype/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include -o renderer.obj
clang -shared renderer.obj -L../../lib/win -lSDL3.lib -o renderer.dll
popd
