#pragma once

#include <pthread.h>

#include <system_error>

namespace pmembench {

  template <class T> class thread_specific_ptr {
    pthread_key_t _key;

  public:
    using pointer = T*;

    thread_specific_ptr() {
      int ec = pthread_key_create(&_key, thread_specific_ptr::at_thread_exit);
      if (ec) {
        throw std::system_error(std::error_code(ec, std::system_category()),
                                "thread_specific_ptr construction failed");
      }
    }

    ~thread_specific_ptr() { pthread_key_delete(_key); }

    static void at_thread_exit(void* p) { delete static_cast<pointer>(p); }

    pointer get() const { return static_cast<pointer>(pthread_getspecific(_key)); }
    T& operator*() const { return *get(); }
    pointer operator->() const { return get(); }

    pointer release() {
      pointer p = get();
      pthread_setspecific(_key, 0);
      return p;
    }

    void reset(pointer v) {
      pointer old = get();
      pthread_setspecific(_key, v);
      delete old;
    }
  };

  template <class T> class thread_local_value {
  public:
    thread_local_value(T def = 0) : _default(def) {}

    T get() const {
      T* p = _val.get();
      if (p) return *p;
      return _default;
    }

    void set(const T& v) {
      T* p = _val.get();
      if (p) {
        *p = v;
        return;
      }
      _val.reset(new T(v));
    }

    T& getRef() {
      T* p = _val.get();
      if (p) {
        return *p;
      }
      p = new T(_default);
      _val.reset(p);
      return *p;
    }

  private:
    thread_specific_ptr<T> _val;
    const T _default;
  };

}  // namespace pmembench
