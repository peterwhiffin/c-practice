pushd ../../build/lin

# clang++ -g -fdeclspec -fPIC -c -DJPH_OBJECT_STREAM -DJPH_USE_AVX2 -DJPH_USE_AVX -DJPH_USE_SSE4_1 -DJPH_USE_SSE4_2 -DJPH_USE_LZCNT -DJPH_USE_TZCNT -DJPH_USE_F16C -DJPH_USE_FMADD ../../src/physics/physics.cpp -I../../../glad/include -I../../../freetype/include -I../../../cglm/include -I../../../JoltPhysics-5.3.0 -o physics.o
# clang++ -g -fdeclspec -fPIC -march=x86-64-v3 -std=c++17 -c -DJPH_DEBUG_RENDERER -DJPH_OBJECT_STREAM -DJPH_USE_AVX2 -DJPH_USE_AVX -DJPH_USE_SSE4_1 -DJPH_USE_SSE4_2 -DJPH_USE_LZCNT -DJPH_USE_TZCNT -DJPH_USE_F16C -DJPH_USE_FMADD -DJPH_PROFILE_ENABLED ../../src/editor/editor.cpp -I../../../cglm/include -I../../../freetype/include -I../../../glad/include -I../../../SDL/include -I../../../JoltPhysics-5.3.0 -o editor.o
clang++ -g -fdeclspec -fPIC -c -DJPH_DEBUG_RENDERER -DJPH_OBJECT_STREAM -DJPH_USE_AVX2 -DJPH_USE_AVX -DJPH_USE_SSE4_1 -DJPH_USE_SSE4_2 -DJPH_USE_LZCNT -DJPH_USE_TZCNT -DJPH_USE_F16C -DJPH_USE_FMADD -DJPH_PROFILE_ENABLED ../../src/editor/editor.cpp -I../../../cglm/include -I../../../freetype/include -I../../../glad/include -I../../../SDL/include -I../../../JoltPhysics-5.3.0 -o editor.o
# clang++ -g -fdeclspec -fPIC -c ../../src/editor/editor.cpp -I../../../cglm/include -I../../../freetype/include -I../../../glad/include -I../../../SDL/include -o editor.o
clang++ -shared editor.o -L../../lib/lin -lSDL3 -lm -o libeditorlib.so 
popd
