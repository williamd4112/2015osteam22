valgrind --leak-check=full ./nachos -f
valgrind --leak-check=full ./nachos -mkdir /a
valgrind --leak-check=full ./nachos -mkdir /a/b
valgrind --leak-check=full ./nachos -cp ../test/num_100.txt /a/b/bf1
valgrind --leak-check=full ./nachos -cp ../test/num_100.txt /a/b/bf2
valgrind --leak-check=full ./nachos -cp ../test/num_100.txt /a/b/bf3
valgrind --leak-check=full ./nachos -lr /
valgrind --leak-check=full ./nachos -rr /a/b
valgrind --leak-check=full ./nachos -lr /

