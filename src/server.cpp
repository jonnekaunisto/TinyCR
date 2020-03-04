#include <vector>
#include "../include/TinyCRServer.h"
#include <thread>
#include <stdlib.h>
#define POSITIVE_KEYSIZE 100000
#define NEGATIVE_KEYSIZE 1000000



void runServerThread(TinyCRServer<uint64_t, uint32_t> *server)
{
	server->startServer();
}

int main(int argc, char **argv)
{

    // Some place holder keys
    vector <uint64_t> positive_keys;
	vector <uint64_t> negative_keys;
    int i = 0;
	while (i < POSITIVE_KEYSIZE) {
		positive_keys.push_back(i);
		i ++;
	}
	i = 0;
	while (i < NEGATIVE_KEYSIZE) {
		negative_keys.push_back(i+POSITIVE_KEYSIZE);
		i ++;
	}
	TinyCRServer<uint64_t, uint32_t>server(30000, positive_keys, negative_keys);
	std::thread serverThread (runServerThread, &server);
	/*
	std::cout << "timeout\n";
	sleep(10); //sleep for 15 seconds
	std::cout << "timeout end\n";

	server.revokeCertificate(1);
	*/
	serverThread.join();
	std::cout << "server joined\n";
}