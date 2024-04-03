#include <atomic>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace gp {
class spinlock {
public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            // Spin until the lock is acquired
        }
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};


template<typename T, typename DerivedClass>
class atomic_interface_base {
    static_assert(
        std::is_same_v<T, uint8_t>  || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> ||
        std::is_same_v<T, uint64_t> || std::is_same_v<T, int>      || std::is_same_v<T, float>    ||
        std::is_same_v<T, double>   || std::is_same_v<T, bool>     || std::is_same_v<T, char>     || 
        std::is_same_v<T, std::byte>,
        "T must be one of the specified base data types (uint8_t, uint16_t, uint32_t, uint64_t, int, float, double, bool, char, or std::byte)"
    );
public:
    T load() {
        return static_cast<DerivedClass*>(this)->load_impl();
    }

    void store(const T& input) {
        static_cast<DerivedClass*>(this)->store_impl(input);
    }

    T fetch_add(const T& value) {
        return static_cast<DerivedClass*>(this)->fetch_add_impl(value);
    }

    T fetch_sub(const T& value) {
        return static_cast<DerivedClass*>(this)->fetch_sub_impl(value);
    }

    bool compare_exchange(T& expected, const T& desired) {
        return static_cast<DerivedClass*>(this)->compare_exchange_impl(expected, desired);
    }

    T operator+ (const T& value) {
        return fetch_add(value);
    }

    T operator- (const T& value) {
        return fetch_sub(value);
    }

    T operator+= (const T& value) {
        return fetch_add(value);
    }

    T operator-= (const T& value) {
        return fetch_sub(value);
    }

    T operator++(int) {
        return fetch_add(1);
    }

    T operator--(int) {
        return fetch_sub(1);
    }


    T operator* (const T& value) {
        return load() * value;
    }

    T operator/ (const T& value) {
        return load() / value;
    }
    
    T operator++() {
        return fetch_add(1);
    }

    T operator--() {
        return fetch_sub(1);
    }
};

template <typename T>
class atomic : public atomic_interface_base<T, atomic<T>> {
public:
    atomic(const T& input_data) : m_atomic_data(input_data) {}

    atomic(const atomic& other) : m_atomic_data(other.m_atomic_data.load()) {}

    atomic& operator=(const atomic& other) {
        m_atomic_data.store(other.m_atomic_data.load());
        return *this;
    }

    bool operator==(const atomic& other) const {
        return m_atomic_data.load() == other.m_atomic_data.load();
    }

    bool operator!=(const atomic& other) const {
        return !(*this == other);
    }
    
    bool operator==(const T& other) const {
        return m_atomic_data.load() == other;
    }

private:
    T load_impl() {
        return m_atomic_data.load();
    }

    void store_impl(const T& input) {
        m_atomic_data.store(input);
    }

    T fetch_add_impl(const T& value) {
        return m_atomic_data.fetch_add(value);
    }

    T fetch_sub_impl(const T& value) {
        return m_atomic_data.fetch_sub(value);
    }

    bool compare_exchange_impl(T& expected, const T& desired) {
        return m_atomic_data.compare_exchange_strong(expected, desired);
    }

    std::atomic<T> m_atomic_data;
    friend class atomic_interface_base<T, atomic<T>>;
};

template <typename T>
class semi_atomic : public atomic_interface_base<T, semi_atomic<T>> {
public:
    semi_atomic(const T& input_data) : m_data_object(input_data) { }

    semi_atomic(const semi_atomic& other) : m_data_object(other.m_data_object) {}

    semi_atomic& operator=(const semi_atomic& other) {
        m_data_object = other.m_data_object;
        return *this;
    }

    bool operator==(const semi_atomic& other) const {
        return m_data_object == other.m_data_object;
    }

    bool operator!=(const semi_atomic& other) const {
        return !(*this == other);
    }

    bool operator==(const T& other) const {
        return m_data_object == other;
    }
    
    atomic<T> convert_to_atomic() {
        return atomic_interface_base<T, atomic<T>>(m_data_object);
    }

    /// Cast to atomic
    operator atomic<T>() {
        return atomic<T>(m_data_object);
    }

private:
    T& load_impl() {
        return m_data_object;
    }

    void store_impl(const T& input) {
        m_data_object = input;
    }

    T& fetch_add_impl(const T& value) {
        m_spinlock.lock();
        m_data_object += value;
        m_spinlock.unlock();
        return m_data_object;
    }

    T& fetch_sub_impl(const T& value) {
        m_spinlock.lock();
        m_data_object -= value;
        m_spinlock.unlock();
        return m_data_object;
    }

    bool compare_exchange_impl(T& expected, const T& desired) {
        m_spinlock.lock();
        if (m_data_object == expected) {
            m_data_object = desired;
            m_spinlock.unlock();
            return true;
        }
        expected = m_data_object;
        m_spinlock.unlock();
        return false;
    }

    T m_data_object;
    T* m_data;
    spinlock m_spinlock;
    friend class atomic_interface_base<T, semi_atomic<T>>;
};

   // Shared Pointer Implementation
   // Forward Declarations

   template <typename T, typename ref_counter>
   class control_block;

   template <typename T>
   class reclaimer;
   
   template <typename T>
   class deleter;

   template <typename T, typename ref_counter, typename memory_manager>
   class _shared_ptr_;

   /// @brief  Shared Pointer
   /// @tparam T The type of the data
   /// @tparam ref_counter The reference counter type (atomic(default), semi_atomic, etc.)
   /// @tparam memory_manager The memory manager type (reclaimer(default), deleter, etc.)

   template <typename T, typename ref_counter = uint32_t, template<typename> typename memory_manager = deleter>
   using shared_ptr = _shared_ptr_<T, ref_counter, memory_manager<control_block<T, ref_counter>>>;

   // Control Block Implementation
   template <typename T, typename ref_counter = atomic<int>>
   class control_block
   { 
       public:
       control_block() : ref_count(1), data(nullptr) {}
       control_block(T&& data) : ref_count(1), data(new T(std::move(data)))   {}
      ~control_block() 
       { 
         if(data != nullptr && ref_count == 0) delete data; 
       }
       ref_counter ref_count;
       T* data;
   };

   // Reclaimer Implementation
   template <typename T>
   class reclaimer
   {
      public:
      void operator()(T* data) {
          reclaimer::retire(data);
      }

      void operator()(const T* data) {
          reclaimer::retire(data);
      }      

      static reclaimer& get_instance()
      {
        static reclaimer instance;
        return instance;
      }

      static void retire(T* data)
      {
          get_instance().data[data] = true;
      }

      static void retire(const T* cb)
      {
          retire(cb);
      }

      static void reclaim()
      {
        for(auto& pair : get_instance().data)
        {
           if(pair.second)
           {
               delete pair.first;
           }
        }
      }
    
      std::unordered_map<T*, bool> data;
      private:
      reclaimer() = default;
      reclaimer(const reclaimer&) = delete;
      reclaimer& operator=(const reclaimer&) = delete;
      reclaimer(reclaimer&&) = delete;
      reclaimer& operator=(reclaimer&&) = delete;
     ~reclaimer() { reclaim(); }
 
   };

   // Deleter Implementation
    template<typename T>
    class deleter {
       public:
       void operator()(T* data) {
           delete data;
       }

       void operator()(const T* data) {
           delete data;
       } 

       static void retire(T* data) {
         if(data != nullptr)
            delete data;
       }
       
       static void retire(const T* data) {
         if(data != nullptr)
            delete data;
       }      
   };




   // Shared Pointer Implementation
   template <typename T, typename ref_counter = atomic<uint32_t>, typename memory_manager = reclaimer<control_block<T, ref_counter>>>
   class _shared_ptr_
   {
     // Only one member variable ie. the control block 
     private : 
     control_block<T, ref_counter>* cb;

     public:
     _shared_ptr_(T& data)  : cb(new control_block<T, ref_counter>((std::move(data)))) {}
     _shared_ptr_(T&& data) : cb(new control_block<T, ref_counter>((std::move(data)))) {}     

     _shared_ptr_(const _shared_ptr_& other) : cb(other.cb) { ++(cb->ref_count); }
    
     _shared_ptr_& operator=(const _shared_ptr_& other) {
        if (this != &other) {
            retire();
            cb = other.cb;
            ++cb->ref_count;
        }
        return *this;
     }

     ~_shared_ptr_() {
       retire();
     }
    
     const bool is_null() const {
        return cb->data == nullptr;
     }
    
     bool is_null_throw() {
        if (is_null()) {
            throw std::runtime_error("The shared pointer is null");
        }
        return false;
     }

     T* operator->() {
        is_null_throw();
        return cb->data;
     }

     T& operator*() {
        is_null_throw();
        return *cb->data;
     }

     const int use_count() const {
        return cb->ref_count;
     }

     const bool unique() const {
        return cb->ref_count == 1;
     }

     bool operator==(const _shared_ptr_& other) {
        return cb == other.cb;
     }

     bool operator!=(const _shared_ptr_& other) {
        return !(*this == other);
     }

     bool operator==(const T* other) {
        return cb->data == other;
     }

     bool operator!=(const T* other) {
        return !(*this == other);
     }

     bool operator==(const void* other) {
        return static_cast<void*>(cb->data) == other;
     }

     void swap(_shared_ptr_& other) {
        std::swap(cb, other.cb);
     }

     operator bool() {
        return cb->data != nullptr;
     }

     void reset() {
        retire();
        cb = nullptr;
     }

     void reset(const T&& data) {
        retire();
        cb = new control_block<T, ref_counter>(std::move(data));
     }
 
     void retire() {
        --cb->ref_count;
        if((cb->ref_count) == 0)
           memory_manager::retire(cb);
        cb = nullptr;
     }

     _shared_ptr_ copy_new_instance() {
        if(cb->ref_count > 0) {
            --cb->ref_count;
            return _shared_ptr_(*cb->data);
        }
        throw std::runtime_error("Cannot copy new instance. The reference count is not greater than 0");
     }

};
} // namespace gp

int main()
{
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