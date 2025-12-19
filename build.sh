pushd build/lin
clang -g -o practice -DSTB_IMAGE_IMPLEMENTATION ../../src/main.c -I../../../SDL/include -I../../../glad/include -L../lin -ldl -lSDL3 -lm
# clang -S -o practice.s ../../src/main.c -I../../../SDL/include -I../../../glad/include -ldl -lSDL3
popd
