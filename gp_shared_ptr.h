#ifndef _ADAPTABLE_SHARED_PTR_H_
#define _ADAPTABLE_SHARED_PTR_H_

#include "gp_atomic.h"

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
               pair.second = false;
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

#endif
