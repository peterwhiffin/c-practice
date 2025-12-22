pushd build/lin
clang -g -fPIC -c ../../src/game.c -I../../../cglm/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include -o game.o
clang -shared -lSDL3 -lm -o libgamelib.so game.o
popd
