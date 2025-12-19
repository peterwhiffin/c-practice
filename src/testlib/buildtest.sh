pushd ../../build/lin
clang -fPIC -c ../../src/testlib/test.c -o test.o
clang -shared -o libtestlib.so test.o
popd
