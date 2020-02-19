#include "../include/common.h"
#include "../include/control_plane_othello.h"
#include "../include/data_plane_othello.h"
#include "../include/bloom_filter.h"
#include "../include/cuckoo.h"
#include <assert.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <iostream>


using namespace std;


/*
	Binary classifier:
	Assume n_positive_keys <= n_negative_keys
*/
template<typename K, class V>
class Binary_Control_Plane
{
public:
	uint32_t n_positive_keys;
	uint32_t n_negative_keys;
	float key_ratio;

	Binary_Control_Plane()
	{
	}

	Binary_Control_Plane(uint32_t n_pos, uint32_t n_neg)
	{
		n_positive_keys = n_pos;
		n_negative_keys = n_neg;
		key_ratio = n_positive_keys/(float)n_negative_keys;
	}

	virtual bool batch_insert(const vector<K> &positive_keys, const vector<K> &negative_keys) = 0;
	// virtual V query(const K &key);
	// virtual vector<V> batch_query(const vector<K> &keys);
};


/*
	Filter Cascades:
	Optimized with the CRLite settings by defalut
*/
template<typename K, class V>
class Binary_Filter_Cascades_Control_Plane:  public Binary_Control_Plane<K, V>
{
public:
	float fpr_lv1;
	float fpr_base;
	vector <BloomFilter<K, 1>> bf_array;
	vector<K> PK; /*positive key set*/
	vector<K> NK; /*negative key set*/

	Binary_Filter_Cascades_Control_Plane()
	{

	}

	void init(uint32_t n_pos, uint32_t n_neg, float fpr_lv1_customize = 0, float fpr_base_customize = 0)
	{
		this->n_positive_keys = n_pos;
		this->n_negative_keys = n_neg;
		this->key_ratio = this->n_positive_keys/(float)this->n_negative_keys;

		if(fpr_base_customize == 0)
			fpr_base = 0.5;
		else
			fpr_base = fpr_base_customize;

		if(fpr_lv1_customize == 0)
			fpr_lv1 = this->key_ratio * sqrt(0.5);
		else
			fpr_lv1 = fpr_lv1_customize;

		BloomFilter<K, 1> bf_lv1(this->n_positive_keys, fpr_lv1);
		bf_array.push_back(bf_lv1);
	}

	Binary_Filter_Cascades_Control_Plane(uint32_t n_pos, uint32_t n_neg, float fpr_lv1_customize = 0, float fpr_base_customize = 0):Binary_Control_Plane<K, V>(n_pos, n_neg)
	{
		if(fpr_base_customize == 0)
			fpr_base = 0.5;
		else
			fpr_base = fpr_base_customize;

		if(fpr_lv1_customize == 0)
			fpr_lv1 = this->key_ratio * sqrt(0.5);
		else
			fpr_lv1 = fpr_lv1_customize;

		BloomFilter<K, 1> bf_lv1(this->n_positive_keys, fpr_lv1);
		bf_array.push_back(bf_lv1);
	}

	// ~Binary_Filter_Cascades_Control_Plane()
	// {
		
	// }

	bool batch_insert(const vector<K> &positive_keys, const vector<K> &negative_keys)
	{
		if (this->n_positive_keys != positive_keys.size() || this->n_negative_keys != negative_keys.size())
		{
			cout<<"Key error, insertion failed"<<endl;
		}
		vector<K> fp_vector;
		vector<K> tp_vector;
		tp_vector = positive_keys;
		for(int i = 0; i<this->n_positive_keys; i++)
		{
			PK.push_back(positive_keys[i]);
			bf_array[0].insert(positive_keys[i]);
		}

		for(int i = 0; i<this->n_negative_keys; i++)
		{
			NK.push_back(negative_keys[i]);
			if (bf_array[0].isMember(negative_keys[i]))
				fp_vector.push_back(negative_keys[i]);
		}
		while(true)
		{
			
			if(fp_vector.size()==0)
				break;
			else
			{
				BloomFilter<K, 1> bf_next(fp_vector.size(), fpr_base);
				// cout<<fp_vector.size()<<bf_next.k<<" "<<bf_next.m<<endl;
				// cout<<fp_vector.size()<<" "<<fp_vector[0]<<endl;
				for(int i=0; i<fp_vector.size();i++)
				{
					bf_next.insert(fp_vector[i]);
				}
				vector<K> tp_vector_next = fp_vector;
				bf_array.push_back(bf_next);
				fp_vector.clear();
				for(int i = 0; i<tp_vector.size(); i++)
				{
					if (bf_next.isMember(tp_vector[i]))
						fp_vector.push_back(tp_vector[i]);
				}
				tp_vector.clear();
				tp_vector = tp_vector_next;
			}
		}
		return true;
	}

	// void online_insert(pair<K, V> kv)
	// {
	// 	if(kv.second == 1)
	// 		PK.push_back(kv.second);
	// 	else if(kv.second == 0)
	// 		NK.push_back(kv.second);

	// 	/*rebuild*/
	// 	Binary_Filter_Cascades_Control_Plane <K, V>fc_new(PK.size(), NK.size());
	// }
	V query(const K &key)
	{
		for(int i = 0; i<bf_array.size()-1;i++)
		{
			if(!bf_array[i].isMember(key))
			{
				if(i%2 == 0)
					return V(0);
				else
					return V(1);
			}
			else if(i == (bf_array.size()-1))
			{
				if(i%2 == 0)
					return V(1);
				else
					return V(0);
			}
		}
	}
};


/*
	Binary Othello:
	Vanilla othello
*/
template<typename K, class V>
class Binary_Othello_Control_Plane: public Binary_Control_Plane<K, V>
{
public:
	ControlPlaneOthello<K, V, 1, 0> othello;

	Binary_Othello_Control_Plane()
	{

	}

	void init(uint32_t n_pos, uint32_t n_neg)
	{
		this->n_positive_keys = n_pos;
		this->n_negative_keys = n_neg;
		this->key_ratio = this->n_positive_keys/(float)this->n_negative_keys;

		othello = ControlPlaneOthello<K, V, 1, 0>(n_pos+n_neg);
	}

	Binary_Othello_Control_Plane(uint32_t n_pos, uint32_t n_neg):Binary_Control_Plane<K, V>(n_pos, n_neg)
	{
		othello = ControlPlaneOthello<K, V, 1, 0>(n_pos+n_neg);

	}

	bool batch_insert(const vector<K> &positive_keys, const vector<K> &negative_keys)
	{
		if (this->n_positive_keys != positive_keys.size() || this->n_negative_keys != negative_keys.size())
		{
			cout<<"Key error, insertion failed"<<endl;
		}
		int i = 0;
		while (i < positive_keys.size()) {
			othello.insert(	pair <K, V> (positive_keys[i],1));
			i ++;
		}
		i = 0;
		while (i < negative_keys.size()) {
			othello.insert(	pair <K, V> (negative_keys[i],0));
			i ++;
		}
		return true;
	}

	V query(const K &key)
	{
		V q;
		othello.query(key,q);
		return q;
	}


};


/*
	BloomFilter Othello:
	BloomFilter + Othello
*/
template<typename K, class V>
class Binary_BF_Othello_Control_Plane: public Binary_Control_Plane<K, V>
{
public:
	ControlPlaneOthello<K, V, 1, 0> othello;
	BloomFilter<K, 1> bf;
	float bf_fpr;


	Binary_BF_Othello_Control_Plane(uint32_t n_pos, uint32_t n_neg):Binary_Control_Plane<K, V>(n_pos, n_neg)
	{
		bf_fpr = optimal_fpr();
		bf = BloomFilter<K, 1>(this->n_positive_keys, bf_fpr);
	}

	float optimal_fpr()
	{
		/*optimal fpr of Bloom Filter to achieve minimal memory cost*/
		return this->key_ratio * 1.44/(log(2)*2.33);
	}

	bool batch_insert(const vector<K> &positive_keys, const vector<K> &negative_keys)
	{
		if (this->n_positive_keys != positive_keys.size() || this->n_negative_keys != negative_keys.size())
		{
			cout<<"Key error, insertion failed"<<endl;
		}
		for(int i = 0; i<this->n_positive_keys; i++)
		{
			bf.insert(positive_keys[i]);
		}
		vector <K> fp_keys{};
		for(int i = 0; i<this->n_negative_keys; i++)
		{
			if(bf.isMember(negative_keys[i]))
			{
				fp_keys.push_back(negative_keys[i]);
			}
				
		}

		cout<<"bf fpr: "<<fp_keys.size()/(float)this->n_negative_keys<<endl;
		othello = ControlPlaneOthello<K, V, 1, 0>(this->n_positive_keys+fp_keys.size());
		for(int i = 0; i<this->n_positive_keys; i++)
			othello.insert(pair <K, V> (positive_keys[i],1));
		
		for(int i = 0; i<fp_keys.size(); i++)
		{
			othello.insert(pair <K, V> (fp_keys[i],0));
		}
		return true;
	}

};


/*
	CuckooFilter Othello:
	CuckooFilter + Othello
*/
template<typename K, class V>
class Binary_VF_Othello_Control_Plane: public Binary_Control_Plane<K, V>
{
public:
	ControlPlaneOthello<K, V, 1, 0> othello;
	VacuumFilter<K> vf;
	unordered_set<K> PK; /*positive key set*/
	unordered_set<K> NK; /*negative key set*/
	
	vector<K> *TNK_table; /*table of true negative keys stored at their corresponding cuckoo position*/
	vector<K> *FPK_table; /*table of flase positive keys stored at their corresponding cuckoo position*/
	int fp_len;
	float load_factor = 0.95;
	float o_ratio = 1;

	Binary_VF_Othello_Control_Plane()
	{

	}

	Binary_VF_Othello_Control_Plane(uint32_t n_pos, uint32_t n_neg, float lf = 0.95, float othello_ratio = 1):Binary_Control_Plane<K, V>(n_pos, n_neg)
	{
		this->load_factor = lf;
		this->o_ratio = othello_ratio;
		fp_len = optimal_fp_len();
		int n = this->n_positive_keys;
		int b = 4;
		uint64_t m = (uint64_t) (n / b / load_factor);
        m += m & 1;
        int maxSteps = 400;
        vf.init(m, b, maxSteps, fp_len);

        TNK_table = new vector<K>[vf.n];
        FPK_table = new vector<K>[vf.n];
	}

	// ~Binary_VF_Othello_Control_Plane()
	// {
	// 	// delete(TNK_table);
	// 	// delete(FPK_table);
	// 	// cout<<"delete"<<endl;
	// }
	
	Binary_VF_Othello_Control_Plane(Binary_VF_Othello_Control_Plane const &obj)
	{
		this->n_positive_keys = obj.n_positive_keys;
		this->n_negative_keys = obj.n_negative_keys;
		this->vf = obj.vf;
		this->othello = obj.othello;
	}
	
	void clear()
	{
		free(vf.T);
	}

	void rebuild(float lf, float othello_ratio)
	{
		clear();
		
		vector<K> positive_keys(PK.begin(), PK.end());
		vector<K> negative_keys(NK.begin(), NK.end());
		PK.clear();
		NK.clear();
		// cout<<"1. clear"<<endl;
		init(positive_keys.size(), negative_keys.size(), lf, othello_ratio);
		// cout<<"2. init"<<endl;
		batch_insert(positive_keys, negative_keys);
		// cout<<"3. insert"<<endl;

	}

	void init(uint32_t n_pos, uint32_t n_neg, float lf = 0.95, float othello_ratio = 1)
	{ 
		this->load_factor = lf;
		this->o_ratio = othello_ratio;

		this->n_positive_keys = n_pos;
		this->n_negative_keys = n_neg;
		this->key_ratio = this->n_positive_keys/(float)this->n_negative_keys;
		std::cout << "npos: " << n_pos << " n_neg: " << n_neg << "\n";
		fp_len = optimal_fp_len();
		std::cout << fp_len << "\n";
		int n = this->n_positive_keys;
		int b = 4;
		uint64_t m = (uint64_t) (n / b / load_factor);
        m += m & 1;
        int maxSteps = 400;
        vf.init(m, b, maxSteps, fp_len);

        delete[] TNK_table;
		delete[] FPK_table;

        TNK_table = new vector<K>[vf.n];
        FPK_table = new vector<K>[vf.n];
        cout<<"success"<<endl;

	}

	int optimal_fp_len()
	{

		float optimal_fpr = 0.618*this->key_ratio/0.95;
		/*ceiling  or floor*/
		int fp_length = int((log2(1/optimal_fpr)+2)/0.95)+1;
		if (fp_length < 4)
			fp_length = 4;
		return fp_length;
	}

	bool batch_insert(const vector<K> &positive_keys, const vector<K> &negative_keys)
	{
		if (this->n_positive_keys != positive_keys.size() || this->n_negative_keys != negative_keys.size())
		{
			cout<<"Key error, insertion failed"<<endl;
		}

		for(uint64_t i = 0; i<this->n_positive_keys; i++)
		{

			if (vf.insert(positive_keys[i])==1)
			{
				cout<<"!!!!!!!!!!!!!insert failed: "<<vf.get_load_factor()<<endl;
				cout<<"rebuild vf with longer fp"<<endl;
				clear();
				PK.clear();
				NK.clear();

				fp_len += 1;
				int n = this->n_positive_keys;
				int b = 4;
				uint64_t m = (uint64_t) (n / b / 0.95);
		        m += m & 1;
		        int maxSteps = 400;
		        vf.init(m, b, maxSteps, fp_len);

				// cout<<"4. return to insert"<<endl;
				return batch_insert(positive_keys, negative_keys);
			}
			PK.insert(positive_keys[i]);

		}
		// cout<<"vf success!!!!"<<endl;
		vector<K> FPK; /*false positive key set*/
		for(uint64_t i = 0; i<this->n_negative_keys; i++)
		{
			if(vf.lookup(negative_keys[i]))
			{
				FPK.push_back(negative_keys[i]);

				pair<uint64_t, uint64_t> pos_pair = vf.position_pair(negative_keys[i]);
				uint64_t pos1 = pos_pair.first;
				uint64_t pos2 = pos_pair.second;
				FPK_table[pos1].push_back(negative_keys[i]);
				FPK_table[pos2].push_back(negative_keys[i]);
			}
			else
			{

				/*record the negative keys at their corresponding cuckoo positions*/
				pair<int, int> pos_pair = vf.position_pair(negative_keys[i]);
				int pos1 = pos_pair.first;
				int pos2 = pos_pair.second;
				TNK_table[pos1].push_back(negative_keys[i]);
				TNK_table[pos2].push_back(negative_keys[i]);

			}
			NK.insert(negative_keys[i]);

		}

		// cout<<"fpr"<<fp_keys.size()/(float)n_negative_keys<<endl;
		// cout<<"fpk success!!!!"<<endl;
		othello = ControlPlaneOthello<K, V, 1, 0>(this->n_positive_keys+FPK.size(), this->o_ratio);

		for(int i = 0; i<this->n_positive_keys; i++)
			othello.insert(pair <K, V> (positive_keys[i],1));
		
		for(int i = 0; i<FPK.size(); i++)
		{
			// cout<<fp_keys[i]<<endl;
			othello.insert(pair <K, V> (FPK[i],0));
		}
		// cout<<"othello success!!!!"<<endl;
		return true;
	}


	/*dynamic features of vf_othello*/

	vector<uint32_t> insert(pair<K, V> kv)
	{
		vector<uint32_t> flipped_indexes{};

		if(kv.second == 1)
		{
			PK.insert(kv.first);
			if (vf.insert(kv.first) == 1)
			{
				cout<<"vf is too full to insert!"<<endl;
				flipped_indexes.clear();
				flipped_indexes.push_back(0xffffffff);

				return flipped_indexes;
			}
			// cout<<"ccc "<<vf.get_load_factor()<<" "<<kv.first<<endl;
			/*vf.insert creates a risk for other negative key predictions:
			after insertion, an existing negtive key might be recognized as a FP key, 
			and this key is not stored by othello, thus yeilding an error
			solve the problem: everytime after vf.insert, query all negative keys, and insert the new FP keys into othello*/
			pair<int, int> pos_pair = vf.position_pair(kv.first);
			int pos1 = pos_pair.first;
			int pos2 = pos_pair.second;
			// cout<<"pos1: "<<TNK_table[pos1].size()<<" pos2: "<<TNK_table[pos2].size()<<endl;
			auto it = TNK_table[pos1].begin();	
			while (it != TNK_table[pos1].end())
			{
				// cout<<"it1 "<<*it<<endl;
				if(vf.lookup(*it) == 1)
				{
					FPK_table[pos1].push_back(*it);
					pair<K, V> fpkv (*it, 0);
					it = TNK_table[pos1].erase(it);
					// cout<<"new fp key: "<<fpkv.first<<" "<<othello.isMember(fpkv.first)<<endl;
					if(!othello.insert_on_demand(fpkv, flipped_indexes))
					{
						cout<<"othello is too full to insert!"<<endl;
						flipped_indexes.clear();
						flipped_indexes.push_back(0xffffffff);
						return flipped_indexes;
					}
				}
				else 
				{
					++it;
				}
			}

			it = TNK_table[pos2].begin();	
			while (it != TNK_table[pos2].end())
			{
				// cout<<"it2 "<<*it<<endl;
				if(vf.lookup(*it) == 1)
				{
					FPK_table[pos2].push_back(*it);
					pair<K, V> fpkv (*it, 0);
					it = TNK_table[pos2].erase(it);
					// cout<<"new fp key: "<<fpkv.first<<" "<<othello.isMember(fpkv.first)<<endl;
					if(!othello.isMember(fpkv.first))
					{
						if(!othello.insert_on_demand(fpkv, flipped_indexes))
						{
							cout<<"othello is too full to insert!"<<endl;
							flipped_indexes.clear();
							flipped_indexes.push_back(0xffffffff);
							return flipped_indexes;
						}
					}
					
				}
				else 
				{
					++it;
				}
			}




			V v1;
			othello.query(kv.first, v1);
			bool is_success = othello.insert_on_demand(kv, flipped_indexes);
			if(is_success)
			{
				/*no rebuild*/
				return flipped_indexes;
				
			}
			else
			{
				/*need rebuild*/
				cout<<"othello is too full to insert!"<<endl;
				flipped_indexes.clear();
				flipped_indexes.push_back(0xffffffff);
				return flipped_indexes;
			}
			
		}
		else
		{
			NK.insert(kv.first);
			if(vf.lookup(kv.first) == 1)
			{
				pair<int, int> pos_pair = vf.position_pair(kv.first);
				int pos1 = pos_pair.first;
				int pos2 = pos_pair.second;
				FPK_table[pos1].push_back(kv.first);
				FPK_table[pos2].push_back(kv.first);
				bool is_success = othello.insert_on_demand(kv, flipped_indexes);
				if(is_success)
				{
					/*no rebuild*/
					return flipped_indexes;
				}
				else
				{
					/*need rebuild*/
					cout<<"othello is too full to insert!"<<endl;
					flipped_indexes.clear();
					flipped_indexes.push_back(0xffffffff);
					return flipped_indexes;
				}
			}
			else
			{
				// cout<<"no change for othello "<<endl;
				pair<int, int> pos_pair = vf.position_pair(kv.first);
				int pos1 = pos_pair.first;
				int pos2 = pos_pair.second;
				TNK_table[pos1].push_back(kv.first);
				TNK_table[pos2].push_back(kv.first);
				flipped_indexes.clear();
				return flipped_indexes;
			}
				
			
		}
	}

	vector<uint32_t> valueFlip(pair<K, V> kv)
	{
		// cout<<kv.first<<" "<<kv.second<<endl; 
		vector<uint32_t> flipped_indexes{};

		

		V v = query(kv.first);

		if(v == kv.second)
		{
			flipped_indexes.clear();
			return flipped_indexes;
		}
		else if(v == 1 and kv.second == 0)
		{
			/*change from positive to negative*/
			PK.erase(kv.first);
			NK.insert(kv.first);
			vf.del(kv.first);
			pair<int, int> pos_pair = vf.position_pair(kv.first);
			int pos1 = pos_pair.first;
			int pos2 = pos_pair.second;

			auto it = FPK_table[pos1].begin();	
			while (it != FPK_table[pos1].end())
			{
				if(vf.lookup(*it) == 0)
				{
					
					TNK_table[pos1].push_back(*it);
					
					if(othello.isMember(*it))
					{
						othello.erase_on_demand(*it, flipped_indexes);
					}
					it = FPK_table[pos1].erase(it);	
				}
				else 
				{
					++it;
				}
			}

			it = FPK_table[pos2].begin();	
			while (it != FPK_table[pos2].end())
			{
				if(vf.lookup(*it) == 0)
				{
					TNK_table[pos2].push_back(*it);
					
					if(othello.isMember(*it))
					{
						othello.erase_on_demand(*it, flipped_indexes);
					}
					it = FPK_table[pos2].erase(it);			
				}
				else 
				{
					++it;
				}
			}
			bool vf_query = vf.lookup(kv.first);

			if(vf_query == 1)
			{
				/*othello.valueFlip(kv)*/
				pair<int, int> pos_pair = vf.position_pair(kv.first);
				int pos1 = pos_pair.first;
				int pos2 = pos_pair.second;
				FPK_table[pos1].push_back(kv.first);
				FPK_table[pos2].push_back(kv.first);
				othello.valueFlip_on_demand(kv, flipped_indexes);

				return flipped_indexes;
			}
			else
			{
				/*othello.delete(k)*/
				pair<int, int> pos_pair = vf.position_pair(kv.first);
				int pos1 = pos_pair.first;
				int pos2 = pos_pair.second;
				TNK_table[pos1].push_back(kv.first);
				TNK_table[pos2].push_back(kv.first);

				
				othello.query(kv.first,v);
				othello.erase_on_demand(kv.first, flipped_indexes);

				return flipped_indexes;
			}
			// /*othello.valueFlip(kv)*/
			// vector<uint64_t> pre_othello_mem = othello.mem;
			// othello.valueFlip(kv);
			// vector<uint64_t> post_othello_mem = othello.mem;
			// flipped_indexes = othello_flipped_indexes(pre_othello_mem, post_othello_mem);
			// return flipped_indexes;
			

		}
		else if(v == 0 and kv.second == 1)
		{
			/*change from negative to positive*/
			NK.erase(kv.first);
			PK.insert(kv.first);
			
			bool vf_query = vf.lookup(kv.first);
			if(vf_query == 1)
			{
				pair<int, int> pos_pair = vf.position_pair(kv.first);
				int pos1 = pos_pair.first;
				int pos2 = pos_pair.second;
				FPK_table[pos1].erase(remove(FPK_table[pos1].begin(), FPK_table[pos1].end(), kv.first), FPK_table[pos1].end());
				FPK_table[pos2].erase(remove(FPK_table[pos2].begin(), FPK_table[pos2].end(), kv.first), FPK_table[pos2].end());
			}
			else
			{
				pair<int, int> pos_pair = vf.position_pair(kv.first);
				int pos1 = pos_pair.first;
				int pos2 = pos_pair.second;
				TNK_table[pos1].erase(remove(TNK_table[pos1].begin(), TNK_table[pos1].end(), kv.first), TNK_table[pos1].end());
				TNK_table[pos2].erase(remove(TNK_table[pos2].begin(), TNK_table[pos2].end(), kv.first), TNK_table[pos2].end());
			}

			// cout<<"1087430 before: "<<vf.lookup(1087430)<<" "<<othello.isMember(1087430)<<endl;
			if (vf.insert(kv.first) == 1)
			{
				cout<<"vf is too full to insert!"<<endl;
				flipped_indexes.clear();
				flipped_indexes.push_back(0xffffffff);
				return flipped_indexes;
			}
			// cout<<"1087430 after: "<<vf.lookup(1087430)<<" "<<othello.isMember(1087430)<<endl;

			pair<int, int> pos_pair = vf.position_pair(kv.first);
			int pos1 = pos_pair.first;
			int pos2 = pos_pair.second;
			auto it = TNK_table[pos1].begin();	
			while (it != TNK_table[pos1].end())
			{
				if(vf.lookup(*it) == 1)
				{
					FPK_table[pos1].push_back(*it);
					pair<K, V> fpkv (*it, 0);
					it = TNK_table[pos1].erase(it);
					if(othello.isMember(fpkv.first))
					{
						cout<<"fpkv.first: "<<fpkv.first<<endl;
					}
					if(!othello.insert_on_demand(fpkv, flipped_indexes))
					{
						cout<<"othello is too full to insert!"<<endl;
						flipped_indexes.clear();
						flipped_indexes.push_back(0xffffffff);
						return flipped_indexes;
					}
				}
				else 
				{
					++it;
				}
			}

			it = TNK_table[pos2].begin();	
			while (it != TNK_table[pos2].end())
			{
				if(vf.lookup(*it) == 1)
				{
					FPK_table[pos2].push_back(*it);
					pair<K, V> fpkv (*it, 0);
					it = TNK_table[pos2].erase(it);
					if(!othello.isMember(fpkv.first))
					{
						if(!othello.insert_on_demand(fpkv, flipped_indexes))
						{
							cout<<"othello is too full to insert!"<<endl;
							flipped_indexes.clear();
							flipped_indexes.push_back(0xffffffff);
							return flipped_indexes;
						}
					}
					
				}
				else 
				{
					++it;
				}
			}


			if(vf_query == 1 && othello.isMember(kv.first) == 1)
			{	
				/*othello.valueFlip(kv)*/
				othello.valueFlip_on_demand(kv, flipped_indexes);
				return flipped_indexes;
			}
			else if (othello.isMember(kv.first) == 1)
			{
				othello.valueFlip_on_demand(kv, flipped_indexes);
				return flipped_indexes;
			}
			else
			{
				
				/*othello.insert(k,1)*/
				
				
				bool is_success = othello.insert_on_demand(kv, flipped_indexes);
				if(is_success)
				{
					/*no rebuild*/
					return flipped_indexes;
				}
				else
				{
					/*need rebuild*/
					cout<<"othello is too full to insert"<<endl;
					flipped_indexes.clear();
					flipped_indexes.push_back(0xffffffff);
					return flipped_indexes;
				}

			}

		}


	}

	void erase(K k)
	{
		if(query(k) == 1)
		{
			PK.erase(k);
			vf.del(k);
			pair<int, int> pos_pair = vf.position_pair(k);
			int pos1 = pos_pair.first;
			int pos2 = pos_pair.second;
			// if(vf.query(k) == 1)
			// {
			// 	TNK_table[pos1].push_back(k);
			// 	TNK_table[pos2].push_back(k);
			// }
			// else
			// {
			// 	FPK_table[pos1].push_back(k);
			// 	FPK_table[pos2].push_back(k);
			// }
			auto it = FPK_table[pos1].begin();	
			while (it != FPK_table[pos1].end())
			{
				if(vf.lookup(*it) == 0)
				{
					TNK_table[pos1].push_back(*it);
					it = FPK_table[pos1].erase(it);
					othello.erase(*it);
				}
				else 
				{
					++it;
				}
			}

			it = FPK_table[pos2].begin();	
			while (it != FPK_table[pos2].end())
			{
				if(vf.lookup(*it) == 0)
				{
					TNK_table[pos2].push_back(*it);
					it = FPK_table[pos2].erase(it);
					othello.erase(*it);
				}
				else 
				{
					++it;
				}
			}


			othello.erase(k);
		}
		else
		{
			NK.erase(k);
			if(vf.query(k) == 1)
			{
				pair<int, int> pos_pair = vf.position_pair(k);
				int pos1 = pos_pair.first;
				int pos2 = pos_pair.second;
				FPK_table[pos1].erase(remove(FPK_table[pos1].begin(), FPK_table[pos1].end(), k), FPK_table[pos1].end());
				FPK_table[pos2].erase(remove(FPK_table[pos2].begin(), FPK_table[pos2].end(), k), FPK_table[pos2].end());
				othello.erase(k);
			}
		}
	}

	vector<uint32_t> othello_flipped_indexes(vector<uint64_t>& pre_mem, vector<uint64_t>& post_mem)
	{
		vector<uint32_t> flipped_indexes{};
		for(uint32_t i=0; i<pre_mem.size();i++)
		{
			uint64_t diff = pre_mem[i] ^ post_mem[i];
			uint8_t index = 0;
			while (diff > 0)
			{
				uint64_t last_bit = diff & (uint64_t)1;
				if (last_bit == 1)
				{
					/*flipped_id: the first 24 bits represents the vector index, the last 6 bits represents the position in the uint32 bucket*/
					uint32_t flipped_id = (i<<6) + (uint32_t)index;
					flipped_indexes.push_back(flipped_id);
				}
				index += 1;
				diff = diff>> 1;
			}

		}
		return flipped_indexes;
	}

	V query(const K &key)
	{
		bool vf_query;
		if(vf.lookup(key))
			vf_query = true;
		else
			vf_query = false;

		if (vf_query)
		{
			V v;
			othello.query(key,v);
			return v;
		}
		else
		{
			return (V)0;
		}
	
	}

};



/*
	Optimal Balanced Othello:
	Balanced Othello Optimized by Cuckoo or Bloom Filter
*/
// template<typename K, class V>
// class Binary_Opt_Balanced_Othello_Control_Plane: public Binary_Control_Plane<K, V>
// {
// public:
// 	Binary_Control_Plane<K,V> opt_balanced_othello;

// 	/*
// 		The ratio threshold to switch BF_othello to VF_othello such that throughput and memory are optimized.
// 		May differ in different appilications.
// 		Requires further analysis.
// 	*/
// 	int ratio_threshold = 8;
// 	Binary_Opt_Balanced_Othello_Control_Plane(uint32_t n_pos, uint32_t n_neg):Binary_Control_Plane<K, V>(n_pos, n_neg)
// 	{
// 		if(this->key_ratio > ratio_threshold)
// 		{
// 			opt_balanced_othello = Binary_VF_Othello_Control_Plane<K, V>(n_pos, n_neg);
// 		}
// 		else
// 		{
// 			opt_balanced_othello = Binary_BF_Othello_Control_Plane<K, V>(n_pos, n_neg);
// 		}
// 	}

// 	bool batch_insert(const vector<K> &positive_keys, const vector<K> &negative_keys)
// 	{
// 		return opt_balanced_othello.batch_insert(positive_keys, negative_keys);
// 	}
// };