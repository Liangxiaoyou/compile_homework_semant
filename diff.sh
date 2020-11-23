make clean
make semant
cp semant ./test
./test/semant ./test/test5.seal > ./test/test5.seal.out
diff ./test/test5.seal.out ./test-answer/test5.seal.out