#include "hash_map_128_bit.h"
int main() {
    // Example usage
    HashMap<int, std::string , 20> hashmap;
    HashMap<std::string, std::string, 20> hashmap2;
    for(int i = 0; i < 1000; i++)
    {
        hashmap.add(i, "Value" + std::to_string(i));
        hashmap2.add("Key" + std::to_string(i), "Value" + std::to_string(i));
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

    return 0;
}
