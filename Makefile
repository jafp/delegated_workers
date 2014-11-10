
all:
	g++ test.cc -pthread -std=c++11 -Wall -Wextra -pedantic -o test

clean:
	rm test