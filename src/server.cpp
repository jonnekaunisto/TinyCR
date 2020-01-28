#include "../include/TinyCRServer.h"
#include <vector>
#define POSITIVE_KEYSIZE 100000
#define NEGATIVE_KEYSIZE 1000000


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

	TinyCRServer server(30000, positive_keys, negative_keys);