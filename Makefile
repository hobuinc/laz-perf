CC=clang
CXX=clang++

CXX_FLAGS=-std=c++11 -stdlib=libc++ -Wall -pedantic -Wextra

export CC
export CXX
export CXX_FLAGS

all:
	make -C tlaz

clean:
	make -C tlaz clean
