// General C/C++ Headers
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

// Socket Programming Headers
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <asm-generic/socket.h>

#define MAX_BUF_SIZE 1024
#define MAX_BACKLOG 3

std::atomic<int> count{0};
std::map<int, int> neighbors;
// TODO: This
std::mutex ostream_mux;

std::map<int, int> unique_id_to_port;

std::vector<std::string> file_list;
std::vector<std::string> required_files;

std::mutex map_mux;
std::map<std::string, std::vector<int>> m; 
std::map<std::string, std::vector<int>> m2;

std::string file_list_string;
std::string depth2info;

struct Client {
	int conn_sockfd;
	struct sockaddr_in client_addr;
};

void act_as_server();
void act_as_client_depth_1();
void act_as_client_depth_2();
void handle_client(Client);
void tokenize(std::string&, const char, std::vector<std::string>&);

int client_id, _PORT, unique_id;
int num_immediate_neigh;

int main(int argc, char** argv) {
	std::ifstream config_file(argv[1]);
	std::filesystem::path directory_path(argv[2]);

#ifdef DEBUG
		std::cout << "=======================================\n";
		std::cout << "List of entries in the directory: " << std::endl;
#endif
	for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(directory_path)) {
		file_list.push_back(dir_entry.path().filename().string());
	}
#ifdef DEBUG
		std::cout << "=======================================\n";
#endif
	for (auto s : file_list) 
		file_list_string += (s + " ");
#ifdef DEBUG
	std::cout << "=======================================\n";
	std::cout << "File List String: " << file_list_string << std::endl;
	std::cout << "=======================================\n";
#endif
	
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
#ifdef DEBUG
		std::cout << "=======================================\n";
		std::printf("%d --> %d\n", id, port);
		std::cout << "=======================================\n";
#endif
		neighbors.insert(std::make_pair(id, port));
	}

	int _t;
	config_file >> _t;
	while (_t--) {
		std::string _s;
		config_file >> _s;
		required_files.push_back(_s);
	}
#ifdef DEBUG
	std::cout << "=======================================\n";
	std::cout << "Required files: ";
	for (auto x: required_files)
		std::cout << x << " ";
	std::cout << "\n";
	std::cout << "=======================================\n";
#endif 
///////////////////////////////////////////////////////////////////////////////////////
////////////////////////PREPROCESSING DONE/////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

	std::thread server_thread(act_as_server);
	std::thread client_thread_depth_1(act_as_client_depth_1);

	client_thread_depth_1.join();
#ifdef DEBUG
	std::cout << "=======================================\n";
	std::cout << "Printing the Map\n";
	std::cout << m.size() << std::endl;
	for (auto elem: m) {
		std::cout << elem.first << ": ";
		for (auto _id : elem.second)
			std::cout << _id << " ";
		std::cout << "\n";
	}
	std::cout << "=======================================\n";
#endif

	std::thread client_thread_depth_2(act_as_client_depth_2);
	client_thread_depth_2.join();

	server_thread.join();

	for (auto elem: m)
		std::sort(elem.second.begin(), elem.second.end());
	for (auto elem: m2)
		std::sort(elem.second.begin(), elem.second.end());
	std::sort(required_files.begin(), required_files.end());

	for (auto x: required_files) {
#ifdef DEBUG
		std::cout << "Searching for file: " << x << ".\n";
#endif
		if (std::find(file_list.begin(), file_list.end(), x) != file_list.end()) 
			std::printf("Found %s at 0 with MD5 0 at depth 0\n", x.c_str());
		else if (m.find(x) != m.end())
			std::printf("Found %s at %d with MD5 0 at depth 1\n", x.c_str(), m[x].front());
		else if (m2.find(x) != m2.end())
			std::printf("Found %s at %d with MD5 0 at depth 2\n", x.c_str(), m2[x].front());
	}
}

void act_as_server() {
	int server_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	int _val = 1;
	setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &_val, sizeof(int));

	struct sockaddr_in server_addr;
	std::memset((void*)&server_addr, 0, sizeof(struct sockaddr_in));

	server_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port = htons(_PORT);

	if (bind(server_sockfd, (const struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
		std::cerr << "Line " << __LINE__ << ": Unable to bind on port " << _PORT << std::endl;
		exit(1);
	}

	listen(server_sockfd, MAX_BACKLOG);

	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(struct sockaddr);

		fd_set readfs;
		struct timeval tv;
		tv.tv_sec = 7;
		tv.tv_usec = 0;
		FD_ZERO(&readfs);
		FD_SET(server_sockfd, &readfs);

		if (select(server_sockfd + 1, &readfs, NULL, NULL, &tv) == 0) return; // this implements a timeout of 7 seconds

		int conn_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
		if (conn_sockfd == -1) {
			std::cout << "There was some error in accepting connection from client" << std::endl;
			exit(1);
		}

		char recv_buffer[MAX_BUF_SIZE] = { };
		char send_buffer[MAX_BUF_SIZE] = { };

		// First find out whether client is in phase 1 or phase 2
		recv(conn_sockfd, (void*)recv_buffer, 2, 0);

		if (recv_buffer[0] == '1') {
#ifdef DEBUG
			std::printf("Sending client on port %d depth 1 information\n", ntohs(client_addr.sin_port));
#endif
			// Task 1. Send Unique ID padded to 10 bytes
			std::stringstream ss;
			ss << unique_id;
			std::string unique_id_str = ss.str();
			std::strcpy(send_buffer, unique_id_str.c_str());
			// Send the unique ID to client
			send(conn_sockfd, send_buffer, 10, 0);

			// Task 2. Send the list of files to client
			std::strcpy(send_buffer, file_list_string.c_str());
			send(conn_sockfd, send_buffer, strlen(send_buffer), 0);
		}
		else if (recv_buffer[0] == '2') {
#ifdef DEBUG
			std::printf("Receiving depth 2 information from client on port %d\n", htons(client_addr.sin_port));
#endif
			recv(conn_sockfd, (void*)recv_buffer, 1000, 0);
			// parse the received message
			std::string flattened_map(recv_buffer);
			std::vector<std::string> _temp;
			tokenize(flattened_map, '\n', _temp);
#ifdef DEBUG
			std::cout << "=======================================\n";
			printf("Tokenized string from user: \n");
			for (auto s : _temp) std::cout << s << "\n";
			std::cout << "=======================================\n";
#endif
			for (auto s: _temp) {
				std::vector<std::string> temp;
				tokenize(s, ' ', temp);
				std::string filename = temp[0];
				temp.erase(temp.begin());
				if (m.find(filename) == m.end()) {
					std::vector<int> v;
					m2[filename] = v;
				}
				for (int i = 0; i < temp.size(); i += 2) {
					int _uid = std::stoi(temp[i]);
					int _port = std::stoi(temp[i + 1]);
					if (unique_id_to_port.find(_uid) == unique_id_to_port.end()) unique_id_to_port.insert(std::make_pair(_uid, _port));
					m2[filename].push_back(_uid);
				}
			}
			close(conn_sockfd);
		}
	}
}

void act_as_client_depth_1() {
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
		char send_buffer[MAX_BUF_SIZE] { };
		char recv_buffer[MAX_BUF_SIZE] { };

		// First, send 1 to show that we want depth 1 info
		strcpy(send_buffer, "1");
		send(client_sockfd, (void*)send_buffer, 2, 0);

		// Task 1. Receive Unique ID
		// The unique id would be padded to 10 bytes
		recv(client_sockfd, (void*)recv_buffer, 10, 0);

		int server_unique_id = std::stoi(recv_buffer);
		unique_id_to_port.insert(std::make_pair(server_unique_id, port));
#ifdef DEBUG
		printf("Connected to %d with unique-ID (after typecasting) %d on port %d\n", id, server_unique_id, port);
#endif

		recv(client_sockfd, (void*)recv_buffer, 1000, 0);

		int i = 0, j = 0;
#ifdef DEBUG
		std::cout << "Received from " << port << ": " << recv_buffer << "\n";
#endif

		while (i <= j && j < strlen(recv_buffer)) {
			if (recv_buffer[j] == ' ' || recv_buffer[j] == 0) {
				std::string temp(recv_buffer + i, recv_buffer + j);
#ifdef DEBUG
				std::cout << "." << temp << ".\n";
#endif
				map_mux.lock();
				if (m.find(temp) != m.end())
					m[temp].push_back(server_unique_id);
				else {
					std::vector<int> v;
					v.push_back(server_unique_id);
					m[temp] = v;
				}
				map_mux.unlock();
				i = ++j;
			} else j++;
		}
	}
#ifdef DEBUG
	std::cout << "All depth 1 information has been received\n";
#endif 
}

void act_as_client_depth_2() {
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
			std::cout << "LINE " << __LINE__ << ": Server on port " << port << " is not active\n";
#endif
			sleep(1);
		}

		char send_buffer[MAX_BUF_SIZE] = { };
		char recv_buffer[MAX_BUF_SIZE] = { };

		std::strcpy(send_buffer, "2");
		send(client_sockfd, (void*)send_buffer, 2, 0);

		// make a string of depth 1 map
		std::string flattened_map;
		std::stringstream ss;
		for (auto elem: m) {
			ss << elem.first << " ";
			for (int uid : elem.second) {
#ifdef DEBUG
				std::printf("%d --> %d\n", uid, unique_id_to_port[uid]);
#endif
				ss << uid << " " << unique_id_to_port[uid] << " ";
			}
			ss << "\n";
		}
		flattened_map = ss.str();
#ifdef DEBUG
		std::cout << "=======================================\n";
		std::cout << "Flattened Map:\n";
		std::cout << flattened_map << std::endl;
		std::cout << "=======================================\n";
#endif 
		send(client_sockfd, flattened_map.c_str(), 1000, 0);
		close(client_sockfd);
	}
}

void tokenize(std::string& str, const char delim, std::vector<std::string>& out) {
	std::stringstream ss(str);
	std::string s;

	while (std::getline(ss, s, delim)) out.push_back(s);
}
