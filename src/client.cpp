#include <vector>
#include "../include/TinyCRClient.h"
#include <thread>
#include <stdlib.h>

TinyCRClient<uint32_t, uint32_t>client("localhost");

void runClientThread()
{
	client.startClient();
}

int main(int argc, char **argv)
{

	std::thread clientThread (runClientThread);

    // try to query 1 out of the client to test if it works.
	std::cout << "timeout\n";
	sleep(5); //sleep for 5 seconds
	std::cout << "timeout end\n";

	uint32_t num = 1;
	uint32_t v = client.queryCertificate(num);
	std::cout << v << "\n";


	clientThread.join();
}

