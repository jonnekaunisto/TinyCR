#include <vector>
#include "TinyCR/TinyCRServer.h"
#include <thread>
#include <stdlib.h>

/*
 * Function for running the server in a thread
 */
void runServerThread(TinyCRServer<uint64_t, uint32_t> *server)
{
	server->startServer();
}

/*
 * Runs the server with appropriate input paremeters.
 */
int main(int argc, char *argv[])
{
	int positive_keysize = 100000;
	int negative_keysize = 1000000;
	if (argc == 3)
	{
		positive_keysize = atoi(argv[1]);
		negative_keysize = atoi(argv[2]);
	}

    // Some place holder keys
    vector <uint64_t> positive_keys;
	vector <uint64_t> negative_keys;
    int i = 0;
	while (i < positive_keysize) {
		positive_keys.push_back(i);
		i ++;
	}
	i = 0;
	while (i < negative_keysize) {
		negative_keys.push_back(i+positive_keysize);
		i ++;
	}
	TinyCRServer<uint64_t, uint32_t>server(30000, positive_keys, negative_keys);
	std::cout << "Server Initialized" << std::endl;
	std::thread serverThread (runServerThread, &server);

	serverThread.join();
}