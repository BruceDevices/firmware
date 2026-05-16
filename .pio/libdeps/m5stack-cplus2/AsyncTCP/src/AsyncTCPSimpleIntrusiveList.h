// Simple intrusive list class
#pragma once

template<typename T> class SimpleIntrusiveList {
  static_assert(std::is_same<decltype(std::declval<T>().next), T *>::value, "Template type must have public 'T* next' member");

public:
  typedef T value_type;
  typedef value_type *value_ptr_type;
  typedef value_ptr_type *value_ptr_ptr_type;

  // Static utility methods
  static size_t list_size(value_ptr_type chain) {
    size_t count = 0;
    for (auto c = chain; c != nullptr; c = c->next) {
      ++count;
    }
    return count;
  }

  static void delete_list(value_ptr_type chain) {
    while (chain) {
      auto t = chain;
      chain = chain->next;
      delete t;
    }
  }

public:
  // Object methods

  SimpleIntrusiveList() : _head(nullptr), _tail(&_head) {}
  ~SimpleIntrusiveList() {
    clear();
  }

  // Noncopyable, nonmovable
  SimpleIntrusiveList(const SimpleIntrusiveList<T> &) = delete;
  SimpleIntrusiveList(SimpleIntrusiveList<T> &&) = delete;
  SimpleIntrusiveList<T> &operator=(const SimpleIntrusiveList<T> &) = delete;
  SimpleIntrusiveList<T> &operator=(SimpleIntrusiveList<T> &&) = delete;

  inline void push_back(value_ptr_type obj) {
    if (obj) {
      *_tail = obj;
      _tail = &obj->next;
      ++_size;
    }
  }

  inline void push_front(value_ptr_type obj) {
    if (obj) {
      if (_head == nullptr) {
        _tail = &obj->next;
      }
      obj->next = _head;
      _head = obj;
      ++_size;
    }
  }

  inline value_ptr_type pop_front() {
    auto rv = _head;
    if (_head) {
      if (_tail == &_head->next) {
        _tail = &_head;
      }
      _head = _head->next;
      --_size;
    }
    return rv;
  }

  inline void clear() {
    // Assumes all elements were allocated with "new"
    delete_list(_head);
    _head = nullptr;
    _tail = &_head;
    _size = 0;
  }

  inline size_t size() const {
    return _size;
  }

  template<typename function_type> inline value_ptr_type remove_if(const function_type &condition) {
    value_ptr_type removed = nullptr;
    value_ptr_ptr_type current_ptr = &_head;
    while (*current_ptr != nullptr) {
      value_ptr_type current = *current_ptr;
      if (condition(*current)) {
        // Remove this item from the list by moving the next item in
        *current_ptr = current->next;
        // If we were the last item, reset tail
        if (current->next == nullptr) {
          _tail = current_ptr;
        }
        --_size;
        // Prepend this item to the removed list
        current->next = removed;
        removed = current;
        // do not advance current_ptr
      } else {
        // advance current_ptr
        current_ptr = &(*current_ptr)->next;
      }
    }

    // Return the removed entries
    return removed;
  }

  inline value_ptr_type begin() const {
    return _head;
  }

  bool validate_tail() const {
    if (_head == nullptr) {
      return (_tail == &_head);
    }
    auto p = _head;
    while (p->next != nullptr) {
      p = p->next;
    }
    return _tail == &p->next;
  }

private:
  // Data members
  value_ptr_type _head;
  value_ptr_ptr_type _tail;
  size_t _size;

};  // class simple_intrusive_list
