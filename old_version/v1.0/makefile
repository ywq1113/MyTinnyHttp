main: main.o http_conn.o
	g++ -g -std=c++11 -o main main.o http_conn.o -lpthread

main.o: main.cpp
	g++ -g -std=c++11 -c main.cpp -lpthread

http_conn.o: http_conn.cpp
	g++ -g -std=c++11 -c http_conn.cpp -lpthread

clean:
	rm -f main main.o http_conn.o
