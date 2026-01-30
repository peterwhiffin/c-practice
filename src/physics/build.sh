pushd ../../build/lin
clang++ -g -fdeclspec -fPIC -march=x86-64-v3 -c -DJPH_PROFILE_ENABLED -DJPH_OBJECT_STREAM -DJPH_USE_AVX2 -DJPH_USE_AVX -DJPH_USE_SSE4_1 -DJPH_USE_SSE4_2 -DJPH_USE_LZCNT -DJPH_USE_TZCNT -DJPH_USE_F16C -DJPH_USE_FMADD ../../src/physics/physics.cpp -I../../../glad/include -I../../../freetype/include -I../../../cglm/include -I../../../JoltPhysics-5.3.0 -o physics.o
clang++ -shared physics.o -L../../lib/lin -lJolt -lSDL3 -lm -o libphysicslib.so 
popd
