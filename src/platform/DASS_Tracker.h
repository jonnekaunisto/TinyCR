/**
 * Holds binary control plane classes
 * @author Xiaofeng Shi 
 */
#ifndef BINARY_CONTROL_PLANE_H
#define BINARY_CONTROL_PLANE_H
#include <vector>
#include <iostream>
#include "../utils/common.h"
#include "control_plane_othello.h"
#include "data_plane_othello.h"
#include "bloom_filter.h"
#include "cuckoo.h"
using namespace std;


/*
    CuckooFilter Othello:
    CuckooFilter + Othello
*/
template<typename K, class V>
class DASS_Tracker
{
public:
    ControlPlaneOthello<K, V, 1, 0> othello;
    VacuumFilter<K> vf;
    unordered_set<K> PK; /*positive key set*/
    unordered_set<K> NK; /*negative key set*/

    uint32_t n_positive_keys;
    uint32_t n_negative_keys;
    float key_ratio;
    
    vector<K> *TNK_table; /*table of true negative keys stored at their corresponding cuckoo position*/
    vector<K> *FPK_table; /*table of flase positive keys stored at their corresponding cuckoo position*/
    int fp_len;
    float load_factor = 0.95;
    float o_ratio = 1;

    DASS_Tracker(){}

    DASS_Tracker(uint32_t n_pos, uint32_t n_neg, float lf = 0.95, float othello_ratio = 1)
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
    
    DASS_Tracker(DASS_Tracker const &obj)
    {
        this->n_positive_keys = obj.n_positive_keys;
        this->n_negative_keys = obj.n_negative_keys;
        this->vf = obj.vf;
        this->othello = obj.othello;
    }
    
    /**
     * Clears the vacuum filter.
     */
    void clear()
    {
        free(vf.T);
    }

    /**
     * Rebuilds the Tracker
     * @param lf TODO:write what this is
     * @param othello_ratio The othello ratio
     */
    void rebuild(float lf, float othello_ratio)
    {
        clear();
        
        vector<K> positive_keys(PK.begin(), PK.end());
        vector<K> negative_keys(NK.begin(), NK.end());
        PK.clear();
        NK.clear();
        init(positive_keys.size(), negative_keys.size(), lf, othello_ratio);
        batch_insert(positive_keys, negative_keys);
    }

    /**
     * A constructor for DASS_Tracker
     * @param n_pos The number of positive keys.
     * @param n_neg The number of negative keys.
     * @param lf TODO:write what this is
     * @param othello_ratio The othello ratio
     */
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

    /**
     * Calculates the optimal fp len
     * @returns The optimal fp len
     */
    int optimal_fp_len()
    {
        float optimal_fpr = 0.618*this->key_ratio/0.95;
        /*ceiling  or floor*/
        int fp_length = int((log2(1/optimal_fpr)+2)/0.95)+1;
        if (fp_length < 4)
            fp_length = 4;
        return fp_length;
    }

    /**
     * Inserts keys in batch
     * @param positive_keys A vector of positive keys
     * @param negative_keys A vector of negative keys
     * @returns false is if the structure was rebuilt and true if it wasn't
     */
    bool batch_insert(const vector<K> &positive_keys, const vector<K> &negative_keys)
    {
        if (this->n_positive_keys != positive_keys.size() || this->n_negative_keys != negative_keys.size())
        {
            cout<<"Key error, insertion failed"<<endl;
            return true;
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

                batch_insert(positive_keys, negative_keys);
                return false;
            }
            PK.insert(positive_keys[i]);

        }
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

        othello = ControlPlaneOthello<K, V, 1, 0>(this->n_positive_keys+FPK.size(), this->o_ratio);

        for(int i = 0; i<this->n_positive_keys; i++)
            othello.insert(pair <K, V> (positive_keys[i],1));
        
        for(int i = 0; i<FPK.size(); i++)
        {
            othello.insert(pair <K, V> (FPK[i],0));
        }
        return true;
    }


    /*dynamic features of vf_othello*/
    /**
     * Inserts a key value pair
     * @param kv Key value pair to be inserted.
     * @returns The flipped indexes from this insertion.
     */
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
            /*vf.insert creates a risk for other negative key predictions:
            after insertion, an existing negtive key might be recognized as a FP key, 
            and this key is not stored by othello, thus yeilding an error
            solve the problem: everytime after vf.insert, query all negative keys, and insert the new FP keys into othello*/
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

    /**
     * Sets the value of a key
     * @param kv Key value of the key to be set to a value.
     */
    vector<uint32_t> setValue(pair<K, V> kv)
    {
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

            if (vf.insert(kv.first) == 1)
            {
                cout<<"vf is too full to insert!"<<endl;
                flipped_indexes.clear();
                flipped_indexes.push_back(0xffffffff);
                return flipped_indexes;
            }

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

    /**
     * Erases a key
     * @param k The key to be erased.
     */
    void erase(K k)
    {
        if(query(k) == 1)
        {
            PK.erase(k);
            vf.del(k);
            pair<int, int> pos_pair = vf.position_pair(k);
            int pos1 = pos_pair.first;
            int pos2 = pos_pair.second;
            
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
            //used to have vf.query(k) == 1
            if(vf.lookup(k) == 1)
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

    /**
     * Queries the value of the key.
     * @param key The key to be queried.
     * @returns The value of the key.
     */
    V query(const K key)
    {
        if (vf.lookup(key))
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

#endif