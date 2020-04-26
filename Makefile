# CC=g++ -std=c++17 -Wall -Wextra -fsanitize=address -D_GLIBCXX_DEBUG -g
CC=g++ -std=c++17 -Wall -Wextra -Ofast -DNDEBUG

main: main.cpp
	$(CC) -o $@ $^

clean:
	rm -rf main

install: main
	mkdir -p /usr/local/bin/
	cp main /usr/local/bin/trash

unisntall:
	rm -rf /usr/local/bin/trash
