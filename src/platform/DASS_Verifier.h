#ifndef BINARY_DATA_PLANE_H
#define BINARY_DATA_PLANE_H

#include <iostream>
#include <vector>
#include "DASS_Tracker.h"
#include "cuckoo.h"

/**
 * Client side structure for quering keys and updating it's key set.
 */
template<typename K, class V>
class DASS_Verifier
{
public:
    DataPlaneOthello<K, V, 1, 0> othello;
    VacuumFilter<K> vf;

    DASS_Verifier(){}

    /**
     * Constructor for DASS_Verifier.
     * @param vo_control An instance of DASS_Tracker.
     */
    DASS_Verifier(DASS_Tracker<K, V> &vo_control)
    {
        this->vf = vo_control.vf;
        this->othello = DataPlaneOthello<K, V, 1, 0>(vo_control.othello);
    }

    /**
     * Install DASS_Verifier from DASS_Tracker.
     * @param vo_control An instance of DASS_Tracker.
     */
    void install(DASS_Tracker<K, V> &vo_control)
    {
        this->vf = vo_control.vf;
        this->othello = DataPlaneOthello<K, V, 1, 0>(vo_control.othello);
    }

    /**
     * Install DASS_Verifier from another DASS_Verifier.
     * @param vo_control An instance of DASS_Verifier.
     */
    void operator = (DASS_Verifier<K, V> const &vo_data)
    {
        this->vf = vo_data.vf;
        this->othello = DataPlaneOthello<K, V, 1, 0>(vo_data.othello);
    }

    /**
     * Queries the value of a key.
     * @param key Key to be queried.
     * @returns Value of the key.
     */
    V query(const K key)
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

    /**
     * Queries keys in a batch.
     * @param keys Keys to be queried.
     * @returns Values of the keys queried.
     */
    vector<V> batch_query(const vector<K> keys)
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

    /**
     * Measures batch query performance.
     * @param keys The keys to be inserted.
     * @returns The time it took to query the keys.
     */
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

    /**
     * Gets the memory cost of the structure.
     * @returns The memory consumption of the structure..
     */
    uint64_t getMemoryCost()
    {
        return vf.memory_consumption + othello.getMemoryCost();
    }

    /**
     * Inserts a kv pair with flipped indexes.
     * @param kv Key value pair to be inserted.
     * @param flipped_indexes The flipped indexes resulting from this insertion.
     */
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
                    cout<<"data vf is too full to insert!"<<endl;
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
    }

    /**
     * Sets value of a key.
     * @param kv The key value pair that is to be set.
     * @param flipped_indexes The flipped indexes resulting from this operation.
     */
    void setValue(pair<K, V> kv, vector<uint32_t> &flipped_indexes)
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

#endif