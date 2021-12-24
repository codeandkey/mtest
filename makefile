CXX = g++

example: example.cpp mtest.cpp
	$(CXX) -Wall -g -pthread $^ -o $@

clean:
	rm -f example
