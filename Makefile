CFLAGS = -Wall -g

all: server subscriber

server: server.c

subscriber: subscriber.c

run_server:
	./server ${IP_SERVER} ${PORT}

run_subscriber:
	./server ${IP_SERVER} ${PORT}

clean:
	rm -rf server subscriber
