pushd ../../build/lin
clang -g -fdeclspec -fPIC -c ../../src/game/game.c -I../../../cglm/include -I../../../freetype/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include -o game.o
clang -shared game.o -L../../lib/lin -lfreetype -lSDL3 -lm -o libgamelib.so
popd
