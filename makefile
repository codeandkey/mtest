example: example.cpp mtest.cpp
	g++ -std=c++11 -Wall -pthread -g $^ -o $@

example.exe: example.cpp mtest.cpp
	x86_64-w64-mingw32-g++ -Wall -g -pthread $^ -o $@

clean:
	rm -f example example.exe
