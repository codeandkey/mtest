example: example.cpp mtest.cpp
	g++ $^ -o $@

clean:
	rm -f example
