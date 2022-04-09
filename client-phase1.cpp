// General C/C++ Headers
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>

// Socket Programming Headers
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_BUF_SIZE 1024
#define DATA_SIZE 100 // read 100 bytes at a time (atmost)
#define MAX_BACKLOG 3

std::atomic<int> count{0};

struct Client {
	int conn_sockfd;
	struct sockaddr_in client_addr;
};

void act_as_server();
void act_as_client(int, int);
void handle_client(Client);

int client_id, _PORT, unique_id;

int main(int argc, char** argv) {
	std::ifstream config_file(argv[1]);
	std::filesystem::path directory_path(argv[2]);

#ifdef DEBUG
		std::cout << "=======================================\n";
		std::cout << "List of entries in the directory: " << std::endl;
#endif
	for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(directory_path)) {
		std::cout << dir_entry.path().filename().string() << std::endl;
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

	std::thread(act_as_server).detach();

	for (int _t = 0; _t < num_immediate_neigh; ++_t) {
		int id, port;
		config_file >> id >> port;
#ifdef DEBUG
		printf("Spawning a thread to connect to %d on port %d\n", id, port);
#endif
		std::thread(act_as_client, id, port).detach();
	}

	while (count < 2 * num_immediate_neigh);
}

void act_as_server() {
	int server_sockfd = socket(PF_INET, SOCK_STREAM, 0);

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

		Client client = { conn_sockfd, client_addr };
		std::thread(handle_client, client).detach();
	}
}

void act_as_client(int id, int port) {
	int client_sockfd = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	std::memset((void*)&server_addr, 0, sizeof(struct sockaddr_in));

	server_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port = htons(port);

	while (connect(client_sockfd, (const struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
#ifdef DEBUG
		std::cout << "Server on port " << port << " is not active\n";
#endif
		sleep(1);
	}

	char recv_buffer[MAX_BUF_SIZE] { };

	// Task 1. Receive Unique ID
	// The unique id would be padded to 10 bytes
	recv(client_sockfd, (void*)recv_buffer, 10, 0);
	printf("Connected to %d with unique-ID %s on port %d\n", id, recv_buffer, port);

	count++;
}

void handle_client(Client client) {
	char send_buffer[MAX_BUF_SIZE] = { };
	// Task 1. Send Unique ID padded to 10 bytes
	std::stringstream ss;
	ss << unique_id;
	std::string unique_id_str = ss.str();
	
	std::strcpy(send_buffer, unique_id_str.c_str());

	send(client.conn_sockfd, send_buffer, 10, 0);

	count++;
}
