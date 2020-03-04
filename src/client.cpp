#include <vector>
#include "../include/TinyCRClient.h"
#include <thread>
#include <stdlib.h>
#include "../include/ClientSocket.h"
#include "../include/SocketException.h"
#define COMMAND_PORT 60000

TinyCRClient<uint64_t, uint32_t>client("localhost");

void runClientThread()
{
	client.startClient();
}

int main(int argc, char **argv)
{

	std::thread clientThread (runClientThread);


	while(true)
	{

		//implement something to take commands and stuff
		std::regex rgx("(show) ([0-9]+)");
        ServerSocket server(COMMAND_PORT);
        std::cout << "Listening For Commands on Port: " << COMMAND_PORT << "\n";

		sleep(10); //sleep for 5 seconds
		uint32_t num = 1;
		bool v = client.queryCertificate(num);
		if(v)
		{
			std::cout <<  "1 valid"<< "\n";
		}
		else
		{
			std::cout <<  "1 not valid"<< "\n";
		}

		v = client.queryCertificate(100500);
		if(v)
		{
			std::cout <<  "100500 valid"<< "\n";
		}
		else
		{
			std::cout <<  "100500 not valid"<< "\n";
		}
	}

	clientThread.join();
}

