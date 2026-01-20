pushd ..\..\build\win
clang++ -g -c ../../src/editor/editor.cpp -I../../../freetype/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include -o editor.obj
clang++ -shared editor.obj -L../../lib/win -lSDL3.lib -o editor.dll
popd
