// General C/C++ Headers
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <filesystem>
#include <thread>
#include <chrono>

// Socket Programming Headers
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_BUF_SIZE 1024
#define DATA_SIZE 100 // read 100 bytes at a time (atmost)
#define MAX_BACKLOG 3

struct Client {
	int conn_sockfd;
	struct sockaddr_in client_addr;
};

// Global Variables
int client_id, _PORT, unique_id; 
int server_sockfd;

void handle_server(int, int);
void listen_for_clients();
void handle_client(Client);

int main(int argc, char** argv) {
	std::ifstream config_file(argv[1]);
	std::filesystem::path directory_path(argv[2]);

#ifdef DEBUG
		std::cout << "=======================================\n";
		std::cout << "List of entries in the directory: " << std::endl;
#endif
	for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(directory_path)) {
		std::cout << dir_entry.path().string() << std::endl;
	}
#ifdef DEBUG
		std::cout << "=======================================\n";
#endif
	
	int num_immediate_neigh;

	config_file >> client_id >> _PORT >> unique_id >> num_immediate_neigh;

#ifdef DEBUG
	std::cout << "Client ID: " << client_id << std::endl;
	std::cout << "Port: " << _PORT << std::endl;
	std::cout << "Private ID: " << unique_id << std::endl;
	std::cout << "Number of Immediate Neighbors: " << num_immediate_neigh << std::endl;
#endif

	std::thread(listen_for_clients).detach();
	
	for (int _ = 0; _ < num_immediate_neigh; ++_) {
		int id, port;
		config_file >> id >> port;

		std::thread server_thread(handle_server, id, port);
		server_thread.detach();


#ifdef DEBUG
		std::cout << "=======================================\n";
		std::cout << "Neighbor ID: " << id << std::endl;
		std::cout << "Port: " << port << std::endl;
		std::cout << "Thread Spawned and Detached\n";
#endif
	}

	while (1) { }
}

// This handles the effective server for this client
// 1. First obtain the Unique ID of the server
void handle_server(int id, int PORT) {
#ifdef DEBUG
	std::cout << "Entered the function" << std::endl;
#endif
	// Connect to server
	int client_sockfd = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	std::memset((void*)&server_addr, 0, sizeof(struct sockaddr_in));

	server_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port = htons(PORT);

	if (connect(client_sockfd, (const struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "unable to connect to %d on port %d\n", id, PORT);
		close(client_sockfd);
		return;
	}

	char recv_buffer[MAX_BUF_SIZE] = { };
	recv(client_sockfd, (void*)recv_buffer, DATA_SIZE, 0);
	printf("Connected to %d with unique-ID %s on port %d\n", id, recv_buffer, PORT);
}

void listen_for_clients() {
	server_sockfd = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	std::memset((void*)&server_addr, 0, sizeof(struct sockaddr_in));

	server_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port = htons(_PORT);

	if (bind(server_sockfd, (const struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
		std::cerr << "Unable to bind on port " << _PORT << std::endl;
		exit(1);
	}

	listen(server_sockfd, MAX_BACKLOG);

	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(struct sockaddr);
		int conn_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_addr_len);

		if (conn_sockfd == -1) {
			std::cout << "There was some error in accepting connection from client" << std::endl;
			exit(1);
		}

		Client client = {conn_sockfd, client_addr};
		std::thread(handle_client, client).detach();
	}
}

void handle_client(Client client) {
	// first send the unique-ID
}
