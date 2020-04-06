#include "platform/DASS_Tracker.h"
#include "platform/DASS_Verifier.h"

/*
 * Runs the server with appropriate input paremeters.
 */
int main()
{
	int total_init_certs = 1000000;
    int positive_keysize = total_init_certs * 0.01;
	int negative_keysize = total_init_certs * 0.99;
	int total_insert_certs = 1000;

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

	int insert_pos_high = total_init_certs + 1 + 0.99 * total_insert_certs;
	int insert_pos_low = total_init_certs + 1;
	std::cout << "inserting: " << insert_pos_low << ":" << insert_pos_high << std::endl;
	for(uint64_t i = insert_pos_low; i < insert_pos_high; i++)
	{
		
		vector<uint32_t> flipped_indexes = dassTracker.insert(std::pair<uint64_t, uint32_t>(i, 1));
		if(flipped_indexes.size() == 1 && flipped_indexes[0] == 0xffffffff)
        {
            /*rebuild*/
            dassTracker.rebuild(dassTracker.load_factor, dassTracker.o_ratio); 
		}
	}


    return 0;
}