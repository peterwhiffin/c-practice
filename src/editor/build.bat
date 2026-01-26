@echo OFF
pushd ..\..\build\win
clang++ -gcodeview -O0 -w -c ../../src/editor/editor.cpp -I../../../freetype/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include -o editor.obj
clang++ -gcodeview -shared editor.obj -L../../lib/win -lSDL3.lib -o editor.dll
popd
