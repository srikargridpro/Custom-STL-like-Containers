#include "hash_map_128_bit.h"
#include <iostream>
#include "shared_ptr"
int main() {
    // Example usage
    HashMap<int, std::string , 20> hashmap;
    HashMap<std::string, std::string, 20> hashmap2;
    for(int i = 0; i < 1000; i++)
    {
        hashmap.set(i, "Value" + std::to_string(i));
        hashmap2.set("Key" + std::to_string(i), "Value" + std::to_string(i));
    }

    for(int i = 0; i < 100; i++)
    {
        std::cout << "Value for key " << i << ": " << hashmap.get(i) << std::endl;
        std::cout << "Value for key " << "Key" + std::to_string(i) << ": " << hashmap2.get("Key" + std::to_string(i)) << std::endl;
    }
    
    size_t count = 0;
    for(auto& it : hashmap)
    {
      std::cout << "Value for key " << *it.key << ": " << *it.value << std::endl;
      ++count;
    }
    std::cout << "iter count = " << count << std::endl;
    std::cout << "Total size = " << hashmap.getTotalSize() << std::endl;
    
    // Shared Ptr test
    gp::atomic<int> atomic_obj(10);
    atomic_obj = 19;
    ++atomic_obj;
    atomic_obj == 20 ? std::cout << "Atomic object is 20\n" : std::cout << "Atomic object is not 20\n";
    atomic_obj += 20;
    gp::atomic<char> (10);
    gp::semi_atomic<int> semi_atomic_obj(10);
    atomic_obj = semi_atomic_obj;

    std::vector<int> vec = {1, 2, 3, 4, 5};
    for(auto& val : vec)
    {
        std::cout << val << " ";
    }

    gp::shared_ptr<std::vector<int>> shared_ptr_obj((vec));
    std::cout << "After shared pointer\n";
    for(auto& val : vec)
    {
        std::cout << val << " ";
    }
    
    // reclaimer<std::vector<int>>::reclaim();

}
    return 0;
}
