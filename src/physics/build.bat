REM @echo OFF
REM pushd ..\..\build\win
REM clang++ -std=c++20 -gcodeview -w -O0 -c -DDEBUG -D_MBCS -DWIN32 -D_WINDOWS -D_HAS_EXCEPTIONS=0 -D_DEBUG -DJPH_FLOATING_POINT_EXCEPTIONS_ENABLED -DJPH_DEBUG_RENDERER -DJPH_PROFILE_ENABLED -DJPH_OBJECT_STREAM -DJPH_USE_AVX2 -DJPH_USE_AVX -DJPH_USE_SSE4_1 -DJPH_USE_SSE4_2 -DJPH_USE_LZCNT -DJPH_USE_TZCNT -DJPH_USE_F16C -DJPH_USE_FMADD ../../src/physics/physics.cpp -I../../../SDL/include -I../../../glad/include -I../../../freetype/include -I../../../cglm/include -I../../../JoltPhysics-5.3.0 -o physics.obj
REM clang++ -std=c++20 -gcodeview -shared physics.obj -L../../lib/win -lJolt.lib -MTd -o physics.dll 
REM popd
@echo OFF
pushd ..\..\build\win
REM cl /Fepractice /EHsc /Zi /DEBUG /MTd /std:c++17 /Zc:inline /fp:fast /D _MBCS /D WIN32 /D _WINDOWS /D _HAS_EXCEPTIONS=0 /D _DEBUG /D JPH_FLOATING_POINT_EXCEPTIONS_ENABLED /D JPH_DEBUG_RENDERER /D JPH_PROFILE_ENABLED /D JPH_OBJECT_STREAM /D JPH_USE_AVX2 /D JPH_USE_AVX /D JPH_USE_SSE4_1 /D JPH_USE_SSE4_2 /D JPH_USE_LZCNT /D JPH_USE_TZCNT /D JPH_USE_F16C /D JPH_USE_FMADD libcmtd.lib glfw3.lib user32.lib gdi32.lib shell32.lib ../../src/main.cpp ../../src/utils/*.c ../../src/utils/soloud/*.cpp ../../src/utils/soloud/*.c -I../../include -I../../../JoltPhysics-5.3.0 -I../../../soloud20200207/include -I../../../imgui-docking /link /libpath:../../lib assimp-vc143-mt.lib Jolt.lib /NODEFAULTLIB:libcmt

cl /LD /Fephysics.dll /EHsc /Zi /DEBUG /MTd /std:c++17 /Zc:inline /fp:fast /D _MBCS /D WIN32 /D _WINDOWS /D _HAS_EXCEPTIONS=0 /D _DEBUG /D JPH_FLOATING_POINT_EXCEPTIONS_ENABLED /D JPH_DEBUG_RENDERER /D JPH_PROFILE_ENABLED /D JPH_OBJECT_STREAM /D JPH_USE_AVX2 /D JPH_USE_AVX /D JPH_USE_SSE4_1 /D JPH_USE_SSE4_2 /D JPH_USE_LZCNT /D JPH_USE_TZCNT /D JPH_USE_F16C /D JPH_USE_FMADD ../../src/physics/physics.cpp -I../../../JoltPhysics-5.3.0 -I../../../SDL/include -I../../../glad/include -I../../../freetype/include -I../../../cglm/include /link /libpath:../../lib/win Jolt.lib libcmtd.lib user32.lib gdi32.lib shell32.lib /NODEFAULTLIB:libcmt


popd
