SRC_FILES:=$(wildcard src/*.c)

main: $(SRC_FILES)
	gcc -o main.exe $(SRC_FILES) -lsndfile-1 -lfftw3
