#include <assert.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <iostream>
#include <time.h>
#include "Binary_Control_Plane.h"
#include "../utils/cuckoo.h"

template<typename K, class V>
class Binary_Data_Plane
{
public:
	virtual V query(const K &key);
	virtual vector<V> batch_query(const vector<K> &keys);
	virtual float batch_query_performance(const vector<K> &keys);
	virtual uint64_t getMemoryCost() = 0;
};

template<typename K, class V>
class Binary_Filter_Cascades_Data_Plane:  public Binary_Data_Plane<K, V>
{
public:
	vector <BloomFilter<K, 1>> bf_array;

	Binary_Filter_Cascades_Data_Plane()
	{

	}


	Binary_Filter_Cascades_Data_Plane(Binary_Filter_Cascades_Control_Plane<K, V> &fc_control)
	{
		this->bf_array = fc_control.bf_array;
	}

	void install(Binary_Filter_Cascades_Control_Plane<K, V> &fc_control)
	{
		this->bf_array = fc_control.bf_array;
	}

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

	vector<V> batch_query(const vector<K> &keys)
	{
		vector<V> results;
		for(int k=0; k<keys.size();k++)
		{
			for(int i = 0; i<bf_array.size()-1;i++)
			{
				if(!bf_array[i].isMember(keys[k]))
				{
					if(i%2 == 0)
					{
						results.push_back(V(0));
						break;
					}

					else
					{
						results.push_back(V(1));
						break;
					}

				}
				else if(i == (bf_array.size()-1))
				{
					if(i%2 == 0)
					{
						results.push_back(V(1));
						break;
					}
					else
					{
						results.push_back(V(0));
						break;
					}

				}
			}
		}
	}

	float batch_query_performance(const vector<K> &keys)
	{
		unsigned long s = clock();
		V v;
		for(int k=0; k<keys.size();k++)
		{
			for(int i = 0; i<bf_array.size()-1;i++)
			{
				if(!bf_array[i].isMember(keys[k]))
				{
					if(i%2 == 0)
					{
						v = 0;
						break;
					}

					else
					{
						v = 1;
						break;
					}

				}
				else if(i == (bf_array.size()-1))
				{
					if(i%2 == 0)
					{
						v = 1;
						break;
					}
					else
					{
						v = 0;
						break;
					}

				}
			}
		}
		double update_diff = (clock() - s) / double(CLOCKS_PER_SEC) * 1000000;
		return update_diff/(float)keys.size();
	}

	uint64_t getMemoryCost()
	{
		uint64_t memory = 0;
		for(int i=0;i<bf_array.size();i++)
		{
			memory += bf_array[i].getMemoryCost();
		}
		return memory;
	}

};

template<typename K, class V>
class Binary_Othello_Data_Plane: public Binary_Data_Plane<K, V>
{
public:
	DataPlaneOthello<K, V, 1, 0> data_othello;

	Binary_Othello_Data_Plane()
	{

	}

	Binary_Othello_Data_Plane(Binary_Othello_Control_Plane<K, V> &o_control)
	{
		data_othello = DataPlaneOthello<K,V, 1, 0>(o_control.othello);
	}

	void install(Binary_Othello_Control_Plane<K, V> &o_control)
	{
		data_othello = DataPlaneOthello<K,V, 1, 0>(o_control.othello);
	}

	V query(const K &key)
	{
		V q;
		data_othello.query(key,q);
		return q;
	}

	vector<V> batch_query(const vector<K> &keys)
	{
		vector<V> results;
		for(int k=0; k<keys.size();k++)
		{
			V q;
			data_othello.query(keys[k],q);
			results.push_back(q);
		}
		return results;
	}

	float batch_query_performance(const vector<K> &keys)
	{
		unsigned long s = clock();
		for(int k=0; k<keys.size();k++)
		{
			V q;
			data_othello.query(keys[k],q);
		}
		double update_diff = (clock() - s) / double(CLOCKS_PER_SEC) * 1000000;
		return update_diff/(float)keys.size();
	}

	uint64_t getMemoryCost()
	{
		return data_othello.getMemoryCost();
	}
	
};

template<typename K, class V>
class Binary_BF_Othello_Data_Plane: public Binary_Data_Plane<K, V>
{
public:
	BloomFilter<K, 1> bf;
	DataPlaneOthello<K, V, 1, 0> othello;

	Binary_BF_Othello_Data_Plane(Binary_BF_Othello_Control_Plane<K, V> &bo_control)
	{
		this->bf = bo_control.bf;
		this->othello = DataPlaneOthello<K, V, 1, 0>(bo_control.othello);
	}

	V query(const K &key)
	{
		bool bf_query = bf.isMember(key);
		if (bf_query)
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

	vector<V> batch_query(const vector<K> &keys)
	{
		vector<V> results;
		for(int i = 0; i<keys.size();i++)
		{
			bool bf_query = bf.isMember(keys[i]);
			if (bf_query)
			{
				V v;
				othello.query(keys[i],v);
				results.push_back(v);
			}
			else
			{
				results.push_back((V)0);
			}
		}
		return results;
	}

	float batch_query_performance(const vector<K> &keys)
	{
		unsigned long s = clock();
		for(int k=0; k<keys.size();k++)
		{
			query(keys[k]);
		}
		double update_diff = (clock() - s) / double(CLOCKS_PER_SEC) * 1000000;
		return update_diff/(float)keys.size();
	}

	uint64_t getMemoryCost()
	{
		return bf.getMemoryCost() + othello.getMemoryCost();
	}

};


template<typename K, class V>
class Binary_VF_Othello_Data_Plane: public Binary_Data_Plane<K, V>
{
public:
	DataPlaneOthello<K, V, 1, 0> othello;
	VacuumFilter<K> vf;

	Binary_VF_Othello_Data_Plane()
	{

	}

	Binary_VF_Othello_Data_Plane(Binary_VF_Othello_Control_Plane<K, V> &vo_control)
	{
		this->vf = vo_control.vf;
		this->othello = DataPlaneOthello<K, V, 1, 0>(vo_control.othello);
	}

	void install(Binary_VF_Othello_Control_Plane<K, V> &vo_control)
	{
		this->vf = vo_control.vf;
		this->othello = DataPlaneOthello<K, V, 1, 0>(vo_control.othello);
	}

	void operator = (Binary_VF_Othello_Data_Plane<K, V> const &vo_data)
	{
		// cout<<"aaaaaaaaaa"<<endl;
		this->vf = vo_data.vf;
		// cout<<"bbbbbbbbb"<<endl;
		this->othello = DataPlaneOthello<K, V, 1, 0>(vo_data.othello);
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

	vector<V> batch_query(const vector<K> &keys)
	{
		vector<V> results;
		for(int i = 0; i<keys.size();i++)
		{
			bool vf_query;
			
			if(vf.lookup(keys[i]))
				vf_query = true;
			else
				vf_query = false;
			if (vf_query)
			{
				V v;
				othello.query(keys[i],v);

				results.push_back(v);
			}
			else
			{
				results.push_back((V)0);
			}
		}
		return results;
	}

	float batch_query_performance(const vector<K> &keys)
	{
		unsigned long s = clock();
		for(int k=0; k<keys.size();k++)
		{
			query(keys[k]);
		}
		double update_diff = (clock() - s) / double(CLOCKS_PER_SEC) * 1000000;
		return update_diff/(float)keys.size();
	}

	uint64_t getMemoryCost()
	{
		return vf.memory_consumption + othello.getMemoryCost();
	}


	/*dynamic functions*/
	void insert(pair<K, V> kv, vector<uint32_t> &flipped_indexes)
	{
		V v;
		othello.query(kv.first, v);
		if(flipped_indexes.size() == 0)
		{
			if(kv.second == 1)
			{
				if (vf.insert(kv.first) == 1)
				{
					// cout<<"aaa "<<vf.get_load_factor()<<endl;
					cout<<"aaa data vf is too full to insert!"<<endl;
					exit (EXIT_FAILURE);
				}

			}
		}
		else if(flipped_indexes.size() == 1 & flipped_indexes[0] == 0xffffffff)
		{
			cout<< "rebuild!!!" <<endl;
		}
		else
		{
			if(kv.second == 1)
			{
				if (vf.insert(kv.first) == 1)
				{
					cout<<"bbb data vf is too full to insert!"<<endl;
					cout<<"bbb "<<vf.get_load_factor()<<" "<<kv.first<<endl;
					exit (EXIT_FAILURE);
				}
				othello.mem_value_flipping(flipped_indexes);

			}
			else
			{
				if(vf.lookup(kv.first) == 1)
					othello.mem_value_flipping(flipped_indexes);
			}
		}

		// cout<<"ccc "<<vf.get_load_factor()<<endl;

	}

	void valueFlip(pair<K, V> kv, vector<uint32_t> &flipped_indexes)
	{

		V v = query(kv.first);
		if(v == kv.second)
		{
			return;
		}
		else if(v == 1 and kv.second == 0)
		{
			/*change from positive to negative*/
			vf.del(kv.first);
			if(flipped_indexes.size()>0)
			{
				if(flipped_indexes.size() == 1 & flipped_indexes[0] == 0xffffffff)
				{
					cout<< "rebuild!!!" <<endl;
					return;
				}
				othello.mem_value_flipping(flipped_indexes);
			}

		}
		else if(v == 0 and kv.second == 1)
		{
			/*change from negative to positive*/
			if (vf.insert(kv.first) == 1)
			{
				cout<<"data vf is too full to insert!"<<endl;
			}
			if(flipped_indexes.size()>0)
			{
				if(flipped_indexes.size() == 1 & flipped_indexes[0] == 0xffffffff)
				{
					cout<< "rebuild!!!" <<endl;
					return;
				}
				othello.mem_value_flipping(flipped_indexes);
			}
		}
	}
};