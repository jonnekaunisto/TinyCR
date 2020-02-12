#include <vector>
#include "../include/TinyCRClient.h"

int main(int argc, char **argv)
{
	TinyCRClient<uint32_t, uint32_t>client("localhost");
	client.startClient();
    // try to query 1 out of the client to test if it works.
}