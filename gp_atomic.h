#include <atomic>
#include <type_traits>
#include <unordered_map>


namespace gp {

class spinlock {
public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) { // Spin until the lock is acquired }
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
