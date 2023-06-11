.PHONY:run-client

run-client:
	./client LAPTOP-CNLTBJLF 12343

.PHONY: make-client

make-client:
	g++ chat_client.cpp -o client -pthread

.PHONY: make-server

make-server: 
	g++ chat_server.cpp -o server -pthread

.PHONY: run-server 

run-server:
	./server 12343

clean:
	rm server
	rm client
