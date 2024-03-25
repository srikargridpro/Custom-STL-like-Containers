#ifndef _128_BIT_HASH_MAP_H_
#define _128_BIT_HASH_MAP_H_
#include <iostream>
#include <deque>
#include <memory>
#include <set>
#include <stack>

// Custom 128-bit hash struct
struct _128_BIT_HASH_
{
   union 
   {
      uint8_t     _8_bit_id[16]; 
      uint16_t    _16_bit_id[8];
      uint32_t    _32_bit_id[4];
      uint64_t    _64_bit_id[2];
   };
     
    _128_BIT_HASH_() {
        _64_bit_id[0] = 0;
        _64_bit_id[1] = 0;
    }


    _128_BIT_HASH_(const _128_BIT_HASH_& hash) {
        _64_bit_id[0] = hash._64_bit_id[0];
        _64_bit_id[1] = hash._64_bit_id[1];
    }
    
    _128_BIT_HASH_(const uint64_t& id_1, const uint64_t& id_2) {
        if(id_1 < id_2)
        {
            _64_bit_id[0] = id_1;
            _64_bit_id[1] = id_2;
        }
        else
        {
            _64_bit_id[0] = id_2;
            _64_bit_id[1] = id_1;
        }
    }

    _128_BIT_HASH_(const uint32_t& id_1, const uint32_t& id_2, const uint32_t& id_3, const uint32_t& id_4) {
        std::set<uint32_t> ids = {id_1, id_2, id_3, id_4};
        
        for(int i = 0; i < 4; i++)
        {
            _32_bit_id[i] = *ids.begin();
            ids.erase(ids.begin());
        }
    }

    _128_BIT_HASH_(const uint16_t& id_1, const uint16_t& id_2, const uint16_t& id_3, const uint16_t& id_4, const uint16_t& id_5, const uint16_t& id_6, const uint16_t& id_7, const uint16_t& id_8) {
        std::set<uint16_t> ids = {id_1, id_2, id_3, id_4, id_5, id_6, id_7, id_8};
        
        for(int i = 0; i < 8; i++)
        {
            _16_bit_id[i] = *ids.begin();
            ids.erase(ids.begin());
        }
    }

    _128_BIT_HASH_& operator=(const _128_BIT_HASH_& hash) {
        _64_bit_id[0] = hash._64_bit_id[0];
        _64_bit_id[1] = hash._64_bit_id[1];
        return *this;
    }

    bool operator==(const _128_BIT_HASH_& hash) const {
        return _64_bit_id[0] == hash._64_bit_id[0] && _64_bit_id[1] == hash._64_bit_id[1];
    }

    bool operator==(_128_BIT_HASH_& hash) {
        return _64_bit_id[0] == hash._64_bit_id[0] && _64_bit_id[1] == hash._64_bit_id[1];
    }


    bool operator!=(const _128_BIT_HASH_& hash) const {
        return _64_bit_id[0] != hash._64_bit_id[0] || _64_bit_id[1] != hash._64_bit_id[1];
    }
};

/// Custom hash function specialization for _128_BIT_HASH_

// Custom pair struct
template <typename Key, typename Value>
struct pair {
    std::shared_ptr<Key>   key;
    std::shared_ptr<Value> value;
    _128_BIT_HASH_         hash_value;

    pair(const std::shared_ptr<Key>& key, const std::shared_ptr<Value>& value, const _128_BIT_HASH_& hash_value) : key(key), value(value), hash_value(hash_value) {}
   ~pair() {}
    
    pair(const pair& p) {
        key = p.key;
        value = p.value;
        hash_value = p.hash_value;
    }

    pair& operator=(const pair& p) {
        key = p.key;
        value = p.value;
        hash_value = p.hash_value;
        return *this;
    }

    void invalidate() {
        hash_value._64_bit_id[0] = 0xffffffffffffffff;
        hash_value._64_bit_id[1] = 0xffffffffffffffff;
        key.reset(nullptr);
        value.reset(nullptr);
    }

    bool isValid() {
        return hash_value._64_bit_id[0] != 0xffffffffffffffff && hash_value._64_bit_id[1] != 0xffffffffffffffff;
    }
};

// Custom hash function object
template <typename Key>
struct hashfuntor {
    _128_BIT_HASH_ operator()(const Key& key) const {
        // Custom hash function implementation
        _128_BIT_HASH_ hash_val;
        hash_val._32_bit_id[0] = std::hash<Key>{}(key);
        return hash_val;
    }
};

// Custom hash map class with 128-bit hash tables
template <typename Key, typename Value, size_t max_domains = 10, typename Hash = hashfuntor<Key>>
class HashMap {
private:
    std::deque<pair<Key, Value>> hash_table[max_domains];
    Hash hash_fun;
    struct free_index 
    {
        size_t domain_index;
        size_t pair_index;
        free_index() {}
        free_index(size_t domain_index, size_t pair_index) : domain_index(domain_index), pair_index(pair_index) {}
       ~free_index() {}
    }; 
    std::stack<free_index> free_indices;

public:
    // Constructor
    HashMap() {}

    // Destructor
    ~HashMap() {}

    ///@brief Add key-value pair to the hashmap
    void set(const Key& key, const Value& value) {
        _128_BIT_HASH_ hash_val = hash_fun(key);
        size_t domain_index = eval_domain_index(hash_val);
        /// Search if the key already exists
        for (auto& pair : hash_table[domain_index]) {
            if ((pair.hash_value) == hash_val) {
                *(pair.value) = value;
                  return;
            }
        }
        /// else create a new pair in the domain
        hash_table[domain_index].push_back(pair(std::make_shared<Key>(key), std::make_shared<Value>(value), hash_val));
    }

    ///@brief Retrieve value associated with key
    Value& get(const Key& key) {
        _128_BIT_HASH_ hash_val = hash_fun(key);
        size_t domain_index = eval_domain_index(hash_val);
        for (auto& pair : hash_table[domain_index]) {
            if ((pair.hash_value) == hash_val) {
                return *(pair.value);
            }
        }
        throw std::out_of_range("Key not found");
    }

    ///@brief Remove key-value pair from the hashmap
    void remove(const Key& key) {
        _128_BIT_HASH_ hash_val = hash_fun(key);
        size_t domain_index = eval_domain_index(hash_val);
        size_t index = 0;
        for(auto& pair : hash_table[domain_index]) {
            if ((pair.hash_value) == hash_val) {
                pair.invalidate();
                free_indices.push({domain_index,index});
            }
            ++index;
        }
    }

    ///@brief Check if key exists in the hashmap
    bool contains(const Key& key) {
        _128_BIT_HASH_ hash_val = hash_fun(key);
        size_t domain_index = eval_domain_index(hash_val);

        for (const auto& pair : hash_table[domain_index]) {
            if (pair.hash_value == hash_val) {
                return true;
            }
        }
        return false;
    }

    ///@brief Get the size of the hashmap for a specific domain
    size_t getDomainSize(size_t domain_index) const {
        if(domain_index >= max_domains)
        {
            throw std::out_of_range("Domain index out of range");
        }
        return hash_table[domain_index].size();
    }

    ///@brief Get the total size of the hashmap across all max_domains
    size_t getTotalSize() const {
        size_t total_size = 0;
        for (size_t i = 0; i < max_domains; ++i) {
            total_size += hash_table[i].size();
        }
        return total_size;
    }
    
    class iterator {
    public :
    iterator(HashMap<Key, Value, max_domains, Hash>* hashmap) : m_hashmap(hashmap), m_domain_index(0), m_pair_index(0) 
    {
        for(size_t i = 0; i < max_domains; i++)
        {
            if(m_hashmap->hash_table[i].size() > 0)
            {
                    m_domain_index = i;
                    m_pair_index = 0;
                    break;
            }
        }
    }

    iterator(HashMap<Key, Value, max_domains, Hash>* hashmap, const size_t& domain_index, const size_t& pair_index) : m_hashmap(hashmap), m_domain_index(domain_index), m_pair_index(pair_index) {}

    void operator++() 
    {  
      if(m_pair_index < m_hashmap->hash_table[m_domain_index].size() - 1)
      {
         ++m_pair_index;
      }
      
      else
      {
        for(size_t i = m_domain_index + 1; i <= max_domains; ++i)
        {
            if(m_hashmap->hash_table[i].size() > 0)
            {
                m_domain_index = i;
                m_pair_index = 0;
                break;
            }
        }

        if(m_domain_index == max_domains)
        {
            m_domain_index = 0xffffffff;
            m_pair_index   = 0xffffffff;
        }
      }
    }

    pair<Key, Value>& operator*() {
      pair<Key, Value>& key_pair = m_hashmap->hash_table[m_domain_index][m_pair_index];
      while(!key_pair.hash_value.isValid()) { ++(*this);}         
        return key_pair;
    }

    bool operator==(const iterator& it) {
        return (m_domain_index == it.m_domain_index && m_pair_index == it.m_pair_index);
    }

    bool operator!=(const iterator& it) {
        return (m_domain_index != it.m_domain_index && m_pair_index != it.m_pair_index);
    }

    private :
    HashMap<Key, Value, max_domains, Hash>* m_hashmap;
    size_t m_domain_index;
    size_t m_pair_index;

    };   

    iterator begin() {
        return iterator(this);
    }

    iterator end() { 
        return iterator(this, 0xffffffff, 0xffffffff);
    }

    private :

    size_t eval_domain_index(const _128_BIT_HASH_& hash_val) {
        size_t sub_domain[4];
        
        sub_domain[0] = hash_val._32_bit_id[0] % max_domains;
        sub_domain[1] = hash_val._32_bit_id[1] % max_domains;
        sub_domain[2] = hash_val._32_bit_id[2] % max_domains;
        sub_domain[3] = hash_val._32_bit_id[3] % max_domains;
        
        size_t domain_index = (sub_domain[0] + sub_domain[1] + sub_domain[2] + sub_domain[3]) % max_domains;
        return domain_index;
    }
};

#endif
