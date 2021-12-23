example: example.cpp mtest.cpp
	g++ -Wall -g -pthread $^ -o $@

clean:
	rm -f example
