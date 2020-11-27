make clean
make semant
cp semant ./test
./test/semant ./test/test2.seal > ./test/test2.seal.out
echo "--------Test and diff -----------------------------"
diff ./test/test2.seal.out ./test-answer/test2.seal.out