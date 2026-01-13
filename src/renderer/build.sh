pushd ../../build/lin
clang -g -fPIC -c ../../src/renderer/renderer.c -I../../../cglm/include -I../../../freetype/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include -o renderer.o
clang -shared renderer.o -L../../lib/lin -lfreetype -lSDL3 -lm -o librendererlib.so 
popd
