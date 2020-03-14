#include <vector>
#include "../include/TinyCRServer.h"
#include <thread>
#include <stdlib.h>
#define POSITIVE_KEYSIZE 100000
#define NEGATIVE_KEYSIZE 1000000


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
int main()
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

	serverThread.join();
	std::cout << "server joined\n";
}