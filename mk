# used for level 1 requirements
# executes mkdisk which creates a new disk image
# then compiles, and executes, passing "mydisk" as argv[1]
./mkdisk

rm a.out 2> /dev/null

gcc main.c util.c

./a.out mydisk


