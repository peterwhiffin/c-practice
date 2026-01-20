pushd ../../build/lin
clang++ -g -fdeclspec -fPIC -c ../../src/editor/editor.cpp -I../../../cglm/include -I../../../freetype/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include -o editor.o
clang++ -shared editor.o -L../../lib/lin -lSDL3 -lm -o libeditorlib.so 
popd
