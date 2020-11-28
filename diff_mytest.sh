make clean
make semant
cp semant ./test
./test/semant ./test/test.seal > ./test/test_0.seal.out
./test/semant1 ./test/test.seal > ./test/test_1.seal.out
echo "--------Test and diff -----------------------------"
diff ./test/test_0.seal.out ./test/test_1.seal.out