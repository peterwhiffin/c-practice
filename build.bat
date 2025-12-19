pushd build\win
clang -g -o practice.exe ../../src/main.c -I../../../SDL/include -I../../../glad/include -L../../lib/win -lSDL3.lib
popd
