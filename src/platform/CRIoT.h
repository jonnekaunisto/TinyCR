/**
 * Holds Crioth Control and Data Classes
 * @author Xiaofeng Shi 
 */
#ifndef CRIoT_H
#define CRIoT_H

#include <iostream>
#include <map>
#include <vector>
#include <time.h>
#include <random>
#include <fstream>
#include <string>
#include "DASS_Verifier.h"
#include "DASS_Tracker.h"
#include "../utils/helpers.h"
using namespace std;

template<typename K, class V>
class CRIoT_Control_VO
{
public:
    DASS_Tracker <K, V> vo_control;
    DASS_Verifier<K, V> vo_data;
    vector<uint32_t> flipped_indexes{};

    float loadfactor = 0.95;
    float o_ratio = 1;

    CRIoT_Control_VO(){}

    /**
     * Constructor for CRIoT Control VO
     * @param pks A vector of positive keys
     * @param nks A vector of negative keys
     * @param lf Load factor
     * @param othello_ratio Othello Ratio
     */
    CRIoT_Control_VO(vector<K> &pks, vector<K> &nks, float lf = 0.95, float othello_ratio = 1)
    {
        this->loadfactor = lf;
        this->o_ratio = othello_ratio;
        vo_control.init(pks.size(), nks.size(), lf, o_ratio);
        vo_control.batch_insert(pks, nks);
        vo_data.install(vo_control);
    }

    /**
     * Alternate constructor for CRIoT Control VO
     * @param pks A vector of positive keys
     * @param nks A vector of negative keys
     * @param lf Load factor
     * @param othello_ratio Othello Ratio
     */
    void init(vector<K> &pks, vector<K> &nks, float lf = 0.95, float othello_ratio = 1)
    {
        this->loadfactor = lf;
        this->o_ratio = othello_ratio;
        vo_control.init(pks.size(), nks.size(), lf, o_ratio);
        vo_control.batch_insert(pks, nks);
        vo_data.install(vo_control);
    }

    /**
     * Encodes the data plane CRC as string arrays.
     * @returns The encoded bytes
     */
    vector<vector<uint8_t>> encode_full()
    {
        DASS_Verifier<K, V> &d_crc = vo_data;
        vector<vector<uint8_t>> v;

        /*othello*/
        /*mem_size*/
        uint32_t mem_size = d_crc.othello.mem.size();
        vector<uint8_t> mem_size_v;
        split_uint_t(mem_size, mem_size_v);
        v.push_back(mem_size_v);

        /*mem*/
        vector<uint8_t> mem_v;
        split_uint_t_vector(d_crc.othello.mem, mem_v);
        v.push_back(mem_v);

        /*ma, mb*/
        vector<uint8_t> ma_v, mb_v;
        split_uint_t(d_crc.othello.ma, ma_v);
        split_uint_t(d_crc.othello.mb, mb_v);
        v.push_back(ma_v);
        v.push_back(mb_v);

        /*hab. Hasher64 type*/
        vector<uint8_t> hab_param_v;
        split_uint_t(d_crc.othello.hab.s, hab_param_v);
        v.push_back(hab_param_v);

        /*hd*/
        vector<uint8_t> hd_param_v;
        split_uint_t(d_crc.othello.hd.s, hd_param_v);
        v.push_back(hd_param_v);


        /*vf*/
        /*memory_consumption*/
        vector<uint8_t> vf_mem_size_v;
        split_uint_t(d_crc.vf.memory_consumption, vf_mem_size_v);
        v.push_back(vf_mem_size_v);
        cout<<"memory_consumption: "<<d_crc.vf.memory_consumption<<endl;

        /*e*/
        vector<uint8_t> vf_e_v;
        stringstream eState;
        eState << d_crc.vf.e;
        string tmp = eState.str();
        uint32_t ess_length = tmp.length();
        split_uint_t(ess_length, vf_e_v);
        cout<<"ess_length: "<<ess_length<<endl;
        convert_string_to_uint8_t_vector(tmp, vf_e_v);
        v.push_back(vf_e_v);

        /*n*/
        vector<uint8_t> vf_n_v;
        uint64_t n = d_crc.vf.n;
        split_uint_t(n, vf_n_v);
        v.push_back(vf_n_v);
        cout<<"n: "<<n<<endl;

        /*m*/
        vector<uint8_t> vf_m_v;
        uint32_t m = d_crc.vf.m;
        split_uint_t(m, vf_m_v);
        v.push_back(vf_m_v);

        /*filled_cell*/
        vector<uint8_t> vf_filled_cell_v;
        uint32_t filled_cell = d_crc.vf.filled_cell;
        split_uint_t(filled_cell, vf_filled_cell_v);
        v.push_back(vf_filled_cell_v);

        /*max_kick_steps*/
        vector<uint8_t> vf_max_kick_steps_v;
        uint32_t max_kick_steps = d_crc.vf.max_kick_steps;
        split_uint_t(max_kick_steps, vf_max_kick_steps_v);
        v.push_back(vf_max_kick_steps_v);

        /*max_2_power*/
        vector<uint8_t> vf_max_2_power_v;
        uint32_t max_2_power = d_crc.vf.max_2_power;
        split_uint_t(max_2_power, vf_max_2_power_v);
        v.push_back(vf_max_2_power_v);

        /*big_seg*/
        vector<uint8_t> vf_big_seg_v;
        uint32_t big_seg = d_crc.vf.big_seg;
        split_uint_t(big_seg, vf_big_seg_v);
        v.push_back(vf_big_seg_v);

        /*isSmall*/
        vector<uint8_t> vf_isSmall_v;
        if(d_crc.vf.isSmall)
            vf_isSmall_v.push_back((uint8_t)1);
        else
            vf_isSmall_v.push_back((uint8_t)0);
        v.push_back(vf_isSmall_v);

        /*len*/
        vector<uint8_t> vf_len_v;
        for(int i=0; i<4; i++)
        {
            uint32_t tem = d_crc.vf.len[i];
            split_uint_t(tem, vf_len_v);
        }
        v.push_back(vf_len_v);

        /*fp_len*/
        vector<uint8_t> fp_len_v;
        uint32_t fp_len = d_crc.vf.fp_len;
        cout << "fp len: " << fp_len << std::endl;
        split_uint_t(fp_len, fp_len_v);
        v.push_back(fp_len_v);

        /*T*/
        vector<uint8_t> vf_T_v;
        int T_len = d_crc.vf.memory_consumption * sizeof(char) /  sizeof(uint32_t);
        for(int i=0; i<T_len; i++)
        {
            split_uint_t(d_crc.vf.T[i], vf_T_v);
        }
        v.push_back(vf_T_v);

        return v;
    }

    /**
     * Encodes the summary for sending
     * Parts are encoded in this order: the type of action, key, value, 
     * flipped_indexes size, flipped indexes.
     * type of action can be add(0), remove(1), unrevoke(2), revoke(3)
     * @param kv The key value pair that the action is done to.
     * @param action The action done.
     * @returns The encoded summary of bytes.
     */
    vector<uint8_t> encode_summary(pair<K, V> kv, uint8_t action)
    {
        vector<uint8_t> encoded;
        std::cout << unsigned(action) << " act" << std::endl;
        encoded.push_back(action);

        //encode flipped key value pair
        //encode key
        K k = kv.first;
        std::cout << "k: " << k << std::endl;
        vector<uint8_t> k_split;
        split_uint_t(k, k_split);
        for(uint8_t val : k_split)
        {
            encoded.push_back(val);
        }
        std::cout << std::endl;

        V v = kv.second;
        std::cout << "v: " << v << std::endl;

        vector<uint8_t> v_split;
        split_uint_t(v, v_split);
        for(uint8_t val : v_split)
        {
            encoded.push_back(val);
        }

        //flipped indexes encode size
        encoded.push_back(flipped_indexes.size());
        std::cout << "flipped indexes size: " << flipped_indexes.size() << std::endl;

        for(int i=0; i < flipped_indexes.size(); i++)
        {
              std::cout << flipped_indexes.at(i) << " ";
        }
        std::cout << std::endl;
        

        //encode flipped indexes
        vector<uint8_t> flipped_indexes_split;
        split_uint_t_vector(flipped_indexes, flipped_indexes_split);
        for(uint8_t val : flipped_indexes_split)
        {
            encoded.push_back(val);
        }

        return encoded;
    }

    /**
     * Inserts a key value pair into the structure.
     * @param kv The key value pair to be inserted.
     * @returns bool Indicating if the insert succeded without rebuild.
     */
    bool insert(pair<K, V> kv)
    {
        std::cout << flipped_indexes.size() << std::endl;
        flipped_indexes = vo_control.insert(kv);
        
        if(flipped_indexes.size() == 1 && flipped_indexes[0] == 0xffffffff)
        {
            /*rebuild*/
            vo_control.rebuild(loadfactor, o_ratio); 

            vo_data.install(vo_control);
            return false;
        }
        return true;
    }

    /**
     * Inserts key value pairs in a batch.
     * @param kv_pairs A vector of key value pairs to be inserted
     * @returns If the structure needed to be rebuilt.
     */
    bool batch_insert(std::vector<pair<K, V>> kv_pairs)
    {
        std::vector<K> positive_keys;
        std::vector<K> negative_keys;

        for (K kv : kv_pairs)
        {
            if(kv.second == 1)
            {
                positive_keys.push_back(kv.first);
            }
            else
            {
                negative_keys.push_back(kv.second);
            }
        }
        return vo_control.batch_insert(positive_keys, negative_keys);
    }


    /**
     * Flips a key value pair in the structure.
     * @param kv The key value pair to be flipped.
     * @returns bool Indicating if the flip succeded without rebuild.
     */
    bool setValue(pair<K, V> kv)
    {
        flipped_indexes = vo_control.setValue(kv);
        if(flipped_indexes.size() == 1 && flipped_indexes[0] == 0xffffffff)
        {
            /*rebuild*/
            vo_control.rebuild(loadfactor, o_ratio);

            vo_data.install(vo_control);
            return false;
        }
        return true;
    }

    /**
     * Erases a key from the structure.
     * TODO: IMPLEMENT
     * @param k The key to be erased.
     */
    void erase(K k)
    {
        vo_control.erase(k);
    }

    /**
	 * Queries the value of the key.
	 * @param key The key to be queried.
	 * @returns The value of the key.
	 */
    V query(K k)
    {
        return vo_control.query(k);
    }
};

template<typename K, class V>
class CRIoT_Data_VO
{
public:
    DASS_Verifier<K, V> vo_data;
    CRIoT_Data_VO(){}

    CRIoT_Data_VO(DASS_Verifier<K, V> &install_patch)
    {
        vo_data = install_patch;
    }

    void rebuild(DASS_Verifier<K, V> &rebuild_patch)
    {
        vo_data = rebuild_patch;
    }

    /**
     * Queries a key from the structure.
     * @param key Key to be queried.
     * @returns The value associated with the key.
     */
    V query(const K key)
    {
        return vo_data.query(key);
    }

    /**
     * Decodes the full update received.
     * @param s The data received from the server
     */
    void decode_full(vector<uint8_t> &s)
    {
        uint32_t offset = 1;
        /*read othello*/
        /*read mem size*/
        uint32_t mem_size = combine_chars_as_uint32_t(s, offset);
        offset+=4;

        /*mem*/
        vector<uint64_t> mem;
        for(uint32_t j=0; j<mem_size; j++)
        {
            uint64_t e = combine_chars_as_uint64_t(s, offset);
            offset +=8;
            mem.push_back(e);
        }

        /*read ma, mb*/
        uint32_t ma = combine_chars_as_uint32_t(s, offset);
        offset+=4;
        uint32_t mb = combine_chars_as_uint32_t(s, offset);
        offset+=4;

        /*hab*/
        uint64_t hab_para = combine_chars_as_uint64_t(s, offset);
        offset+=8;
        Hasher64<K> hab(hab_para);

        /*hd*/
        uint32_t hd_para = combine_chars_as_uint32_t(s, offset);
        offset+=4;
        Hasher32<K> hd(hd_para);

        DataPlaneOthello<K, V, 1, 0> data_othello(mem, ma, mb, hab, hd);
        vo_data.othello = data_othello;

        cout<<"o finished"<<endl;

        /*read vf*/
        /*memory_consumption*/
        uint64_t memory_consumption = combine_chars_as_uint64_t(s, offset);
        offset+=8;

        cout<<"memory_consumption: "<<memory_consumption<<endl;
        /*e*/
        uint32_t ess_length = combine_chars_as_uint32_t(s, offset);
        cout<<"ess_length: "<<ess_length<<endl;
        offset+=4;
        char* eState_c = new char[ess_length];
        for(int i=0; i<ess_length; i++)
        {
            memcpy(&eState_c[i], &s[offset], 1);
            offset += 1;
        }
        stringstream eState;
        eState<<eState_c;
        default_random_engine e;
        eState >> e;
        delete[] eState_c;

        /*n*/
        long long n = combine_chars_as_uint64_t(s, offset);
        offset+=8;
        cout<<"n: "<<n<<endl;

        /*m*/
        int m = combine_chars_as_uint32_t(s, offset);
        offset+=4;

        /*filled_cell*/
        int filled_cell = combine_chars_as_uint32_t(s, offset);
        offset+=4;

        /*max_kick_steps*/
        int max_kick_steps = combine_chars_as_uint32_t(s, offset);
        offset+=4;

        /*max_2_power*/
        int max_2_power = combine_chars_as_uint32_t(s, offset);
        offset+=4;

        /*big_seg*/
        int big_seg = combine_chars_as_uint32_t(s, offset);
        offset+=4;

        /*isSmall*/
        bool isSmall = (bool)s[offset];
        offset += 1;

        /*len*/
        int len[4];
        for(int i=0; i<4; i++)
        {
            len[i] = combine_chars_as_uint32_t(s, offset);
            offset += 4;
        }

        /*fp_len*/
        int fp_len = combine_chars_as_uint32_t(s, offset);
        offset+=4;

        /*T*/
        int T_len = memory_consumption * sizeof(char) /  sizeof(uint32_t);
        uint32_t *T = new uint32_t[T_len];
        for(int i=0; i<T_len; i++)
        {
            T[i] = combine_chars_as_uint32_t(s, offset);
            offset+=4;
        }

        cout<<"offset "<<offset<<endl;

        vo_data.vf.init_with_params(memory_consumption, e, n, m, filled_cell, max_kick_steps, max_2_power, big_seg, isSmall, len, fp_len, T_len, T);
        cout << fp_len;
        cout<<"vf finish" << std::endl;
    }

    /**
     * Decodes the summary
     * Parts are decoded in this order: the type of action, key, value, 
     * flipped_indexes size, flipped indexes.
     * type of action can be add(0), remove(1), unrevoke(2), revoke(3)
     * @param summary The vector of bytes of summary.
     */
    void decode_summary(vector<uint8_t> summary)
    {
        unsigned int action = unsigned(summary[0]);
        std::cout << "action: " << action << std::endl;
        int offset = 1;
        
        int K_size = sizeof(K);
        auto k_chars = vector<uint8_t>();
        for(int i = 0; i < K_size; i++){
            k_chars.push_back(summary[i + offset]);
        }
        K k;
        k = combine_chars_as_uint(k_chars, k);
        std::cout << "k: " << k << std::endl;

        offset += K_size;

        int V_size = sizeof(V);
        auto v_chars = vector<uint8_t>();
        for(int i = 0; i < V_size; i++){
            v_chars.push_back(summary[i + offset]);
        }
        V v;
        v = combine_chars_as_uint(v_chars, v);
        std::cout << "v: " << v << std::endl;


        offset += V_size;
        int flipped_indexes_size = summary[offset];
        offset += 1;

        vector<uint32_t> new_flipped_indexes = vector<uint32_t>();
        for(int i = 0; i < flipped_indexes_size; i++)
        {
            auto flipped_chars = vector<uint8_t>();
            for(int j = 0; j < sizeof(uint32_t); j++)
            {
                flipped_chars.push_back(summary[j + offset]);
            }
            uint32_t f;
            f = combine_chars_as_uint(flipped_chars, f);
            new_flipped_indexes.push_back(f);
            offset += sizeof(uint32_t);
        }

        std::cout << "flipped indexes size: " << flipped_indexes_size << std::endl;
        //std::cout << new_flipped_indexes << std::endl;
        for(int i=0; i < new_flipped_indexes.size(); i++)
        {
              std::cout << new_flipped_indexes.at(i) << " ";
        }
        std::cout << std::endl;

        vector<uint32_t>& new_flipped_indexes_ref = new_flipped_indexes;

        //add
        if(action == 0)
        {
            pair<K,V> kv(k, v);
            insert(kv, new_flipped_indexes_ref);
        }
        //remove
        else if(action == 1)
        {
            //nothing
        }
        //unrevoke(2), revoke(3)
        else if(action == 2 || action == 3)
        {
            pair<K,V> kv(k, v);
            pair<K,V> kv_ref = kv;
            setValue(kv_ref, new_flipped_indexes_ref);
        }
        else
        {
            std::cout << "Encountered unknown action: " << action << std::endl; 
        }
    }

    /**
     * Inserts a key with a value.
     * @param kv Key value pair to be inserted.
     * @param flipped_indexes The vector of flipped indexes received.
     */
    void insert(pair<K, V> kv, vector<uint32_t> &flipped_indexes)
    {
        vo_data.insert(kv, flipped_indexes);
    }

    /**
     * Sets a value.
     * @param kv Key .
     * @param flipped_indexes The vector of flipped indexes received.
     */
    void setValue(pair<K, V> &kv, vector<uint32_t> &flipped_indexes)
    {
        vo_data.setValue(kv, flipped_indexes);
    }
};

#endif