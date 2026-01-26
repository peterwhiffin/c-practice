@echo OFF
pushd ..\..\build\win
clang++ -gcodeview -w -O0 -c ../../src/physics/physics.cpp -I../../../SDL/include -I../../../glad/include -I../../../freetype/include -I../../../cglm/include -I../../../JoltPhysics-5.3.0/Jolt -o physics.obj
clang++ -gcodeview -shared physics.obj -L../../lib/win -lJolt.lib -lSDL3.lib -o physics.dll 
popd
