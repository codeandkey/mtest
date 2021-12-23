example: example.c mtest.c
	g++ -Wall -g -pthread $^ -o $@

clean:
	rm -f example
