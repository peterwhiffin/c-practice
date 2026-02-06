pushd build/lin
clang -g -rdynamic -o practice -DPETE_EDITOR -DSTB_IMAGE_IMPLEMENTATION ../../src/main.c -I../../../cglm/include -I../../../freetype/include -I../../../SDL/include -I../../../glad/include -I../JoltPhysics-5.3.0 -L../../lib/lin -lJolt -ldl -lSDL3 -lm
# clang -O3 -msse2 -o practice -DSTB_IMAGE_IMPLEMENTATION ../../src/main.c -I../../../SDL/include -I../../../glad/include -L../lin -ldl -lSDL3 -lm
# clang -S -o practice.s ../../src/main.c -I../../../SDL/include -I../../../glad/include -ldl -lSDL3
popd
