#!/bin/sh

i=1

valgrind --leak-check=full ./nachos -f
valgrind --leak-check=full ./nachos -mkdir /a
valgrind --leak-check=full ./nachos -mkdir /b
valgrind --leak-check=full ./nachos -cp ../test/num_1000.txt /b/bf
while [ $i -le 64 ] 
do
    echo "Copy file to NachOS: "$i
    valgrind --leak-check=full ./nachos -cp ../test/num_100.txt /a/f$i
    i=`expr $i + 1`
done
valgrind --leak-check=full ./nachos -lr /
valgrind --leak-check=full ./nachos -rr /a
valgrind --leak-check=full ./nachos -lr /
