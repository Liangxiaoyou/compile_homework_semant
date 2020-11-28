make clean
make semant
cp semant ./test
./test/semant ./test/test.seal > ./test/test.seal.out
echo "--------Test and diff -----------------------------"
diff ./test/test.seal.out ./test-answer/test.seal.out