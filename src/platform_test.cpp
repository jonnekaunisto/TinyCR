#include "platform/DASS_Tracker.h"
#include "platform/DASS_Verifier.h"

/*
 * Runs the server with appropriate input paremeters.
 */
int main()
{
    int positive_keysize = 100000;
	int negative_keysize = 1000000;

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
    
    DASS_Tracker<uint64_t, uint32_t> dassTracker;
    dassTracker.init(positive_keys.size(), negative_keys.size());
    dassTracker.batch_insert(positive_keys, negative_keys);

    DASS_Verifier<uint64_t, uint32_t> dassVerifier;
    dassVerifier.install(dassTracker);


    return 0;
}