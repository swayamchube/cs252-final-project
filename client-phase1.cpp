// General C/C++ Headers
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
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
#include <asm-generic/socket.h>

#define MAX_BUF_SIZE 1024
#define DATA_SIZE 100 // read 100 bytes at a time (atmost)
#define MAX_BACKLOG 3

std::atomic<int> count{0};

std::map<int, int> neighbors;
std::vector<std::string> file_list;

struct Client {
	int conn_sockfd;
	struct sockaddr_in client_addr;
};

void act_as_server();
void act_as_client();
void handle_client(Client);

int client_id, _PORT, unique_id;
int num_immediate_neigh;

int main(int argc, char** argv) {
	std::ifstream config_file(argv[1]);
	std::filesystem::path directory_path(argv[2]);

	for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(directory_path)) {
		if (dir_entry.path().filename().string() != "Downloaded")
			file_list.push_back(dir_entry.path().filename().string());
	}
	std::sort(file_list.begin(), file_list.end());
	
	for (auto s: file_list) std::cout << s << "\n";

	config_file >> client_id >> _PORT >> unique_id >> num_immediate_neigh;

#ifdef DEBUG
	std::cout << "Client ID: " << client_id << std::endl;
	std::cout << "Port: " << _PORT << std::endl;
	std::cout << "Private ID: " << unique_id << std::endl;
	std::cout << "Number of Immediate Neighbors: " << num_immediate_neigh << std::endl;
#endif

	for (int _t = 0; _t < num_immediate_neigh; ++_t) {
		int id, port;
		config_file >> id >> port;
		neighbors.insert(std::make_pair(id, port));
	}

	std::thread server_thread(act_as_server);
	std::thread client_thread(act_as_client);

	server_thread.join();
	client_thread.join();
}

void act_as_server() {
	// Just setting up socket stuff
	int server_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	int _val = 1;
	setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEPORT, &_val, sizeof(int));

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
	// done setting up socket stuff
	int number_of_clients_connected = 0;

	while (number_of_clients_connected < num_immediate_neigh) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(struct sockaddr);
		int conn_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_addr_len);

		if (conn_sockfd == -1) {
			std::cout << "There was some error in accepting connection from client" << std::endl;
			exit(1);
		}

		Client client = { conn_sockfd, client_addr };
		char send_buffer[MAX_BUF_SIZE] = { };
		// Task 1. Send Unique ID padded to 10 bytes
		std::stringstream ss;
		ss << unique_id;
		std::string unique_id_str = ss.str();
		std::strcpy(send_buffer, unique_id_str.c_str());
		send(client.conn_sockfd, send_buffer, 10, 0);
		number_of_clients_connected++;
	}
}

void act_as_client() {
	for (std::pair<int, int> elem: neighbors) {
		int client_sockfd = socket(PF_INET, SOCK_STREAM, 0);
		int id = elem.first, port = elem.second;

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
	}
	count++;
}
