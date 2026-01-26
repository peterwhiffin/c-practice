pushd ../../build/lin
clang++ -g -fdeclspec -fPIC -c ../../src/physics/physics.cpp -I../../../glad/include -I../../../freetype/include -I../../../cglm/include -I../../../JoltPhysics-5.3.0/Jolt -o physics.o
clang++ -shared physics.o -L../../lib/lin -lJolt -lSDL3 -lm -o libphysicslib.so 
popd
