#pragma once
#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include "array_ptr.h"

class ReserveProxyObj {
public:
  ReserveProxyObj(size_t i) {
      capacity_to_reserve = i;
  }
  size_t capacity_to_reserve;
};

template <typename Type>
class SimpleVector {
public:
  using Iterator = Type*;
  using ConstIterator = const Type*;
  SimpleVector() noexcept = default;

  SimpleVector(ReserveProxyObj other) {
      Reserve(other.capacity_to_reserve);
  }

  // Создаёт вектор из size элементов, инициализированных значением по умолчанию
  explicit SimpleVector(size_t size) : array_ptr_(size) {
      // заполняем значением поумолчанию
      size_ = size;
      capacity_ = size_;
      std::fill(array_ptr_.Get(), array_ptr_.Get() + size_, Type());
  }

  // Создаёт вектор из size элементов, инициализированных значением value
  SimpleVector(size_t size, const Type& value) : array_ptr_(size) {
      size_ = size;
      capacity_ = size_;
      for (size_t i = 0; i < size; ++i) {
          array_ptr_[i] = value;
      }
  }

  // Создаёт вектор из std::initializer_list
  SimpleVector(std::initializer_list<Type> init) : array_ptr_(init.size()) {
      // перемещаем значения из списка инициализации в вектор
      size_ = init.size();
      capacity_ = size_;
      std::move(init.begin(), init.end(), array_ptr_.Get());
  }


  SimpleVector(const SimpleVector& other) {
      if (!other.IsEmpty()) {
          SimpleVector<Type> tmp(other.size_);
          std::copy(other.begin(), other.end(), tmp.begin());
          swap(tmp);
      }
  }

  SimpleVector& operator=(const SimpleVector& rhs) {
      if (*this != rhs) {
          SimpleVector<Type> tmp(rhs);
          swap(tmp);
      }
      return *this;
  }

  void swap(SimpleVector& other) noexcept {
      array_ptr_.swap(other.array_ptr_);
      std::swap(size_, other.size_);
      std::swap(capacity_, other.capacity_);
  }

  //------------------------------------- методы управения вектором ------------------------------ 
    // Возвращает количество элементов в массиве
  size_t GetSize() const noexcept {
      return size_;
  }

  // Возвращает вместимость массива
  size_t GetCapacity() const noexcept {
      return capacity_;
  }

  // Сообщает, пустой ли массив
  bool IsEmpty() const noexcept {
      return size_ == 0;
  }

  // Возвращает ссылку на элемент с индексом index
  Type& operator[](size_t index) noexcept {
      return array_ptr_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  const Type& operator[](size_t index) const noexcept {
      return array_ptr_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  Type& At(size_t index) {
      if (index >= size_) {
          throw std::out_of_range("index out of range");
      }
      return array_ptr_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  const Type& At(size_t index) const {
      if (index >= size_) {
          throw std::out_of_range("index out of range");
      }
      return array_ptr_[index];
  }

  // Обнуляет размер массива, не изменяя его вместимость
  void Clear() noexcept {
      size_ = 0;
  }

  // увеличивает емкость вектора, переносим старые данные в новую память
  void Reserve(size_t new_capacity) {
      if (new_capacity >= capacity_) {
          ArrayPtr<Type> tmp(new_capacity);
          std::move(begin(), end(), tmp.Get());
          array_ptr_.swap(tmp);
          // array_ptr tmp сам очищает за собой память
          capacity_ = new_capacity;
      }
  }
  // Изменяет размер массива.
  // При увеличении размера новые элементы получают значение по умолчанию для типа Type
  void Resize(size_t new_size) {
      if (new_size > size_) {
          // если емкость меньше нового размера резервируем новую память 
          if (capacity_ <= new_size) {
              Reserve(new_size);
              // заполняем пустые ячейки от последнего эл массива до конца 
              std::fill(this->end(), array_ptr_.Get() + new_size, Type());
          }
          else {
              std::fill(this->end(), array_ptr_.Get() + new_size, Type());
          }
      }
      size_ = new_size;
  }

  // При нехватке места увеличивает вдвое вместимость вектора
  void PushBack(const Type& item) {
      if (capacity_ == 0) {
          SimpleVector tmp(1, item);
          swap(tmp);
          size_ = 1;
          capacity_ = 1;
      }
      else if (size_ == capacity_) {
          size_t new_capacity = capacity_ * 2;
          // создаю всеменный массив и с новой емкостью и копируую в него старый
          SimpleVector<Type> tmp(new_capacity);
          std::copy(begin(), end(), tmp.begin());
          //размер нового массива не меняется меняется только емкость
          tmp.size_ = size_;
          // добавляю новый эл в конец массива
          tmp[size_] = item;
          swap(tmp);
          size_++;
          capacity_ = new_capacity;
      }
      else {
          array_ptr_[size_] = item;
          size_++;
      }
  }

  // Вставляет значение value в позицию pos.
  // Возвращает итератор на вставленное значение
  // Если перед вставкой значения вектор был заполнен полностью,
  // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
  Iterator Insert(ConstIterator pos, const Type& value) {
      if (capacity_ == 0) {
          PushBack(value);
          return Iterator{ end() - 1 };
      }
      auto dist = std::distance(cbegin(), pos);
      size_t tmp_capacity = capacity_;

      if (capacity_ == size_) {
          tmp_capacity *= 2;
      }
      //создаю временный вектор увеличиваю емковть вдвое 
      SimpleVector<Type> tmp(tmp_capacity);
      // копирую вектор до позиции вставки  
      std::copy(begin(), (begin() + dist), tmp.begin());
      // вставляю эл 
      tmp.array_ptr_[dist] = value;
      // копирую вторую половину вектора после эл вставки
      std::copy_backward((begin() + dist), end(), tmp.end());
      swap(tmp);
      size_ = ++(tmp.size_);
      capacity_ = tmp_capacity;
      return Iterator{ Iterator(begin() + dist) };
  }

  // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
  void PopBack() noexcept {
      if (size_) {
          --size_;
      }
  }

  // Удаляет элемент вектора в указанной позиции 
  Iterator Erase(ConstIterator pos) {
      if (pos == end()) {
          return end();
      }
      std::copy(Iterator(pos + 1), end(), Iterator(pos));
      size_--;
      return Iterator(pos);
  }

  // ------------------------------- реализация итераторов -------------------------------------

   // Возвращает итератор на начало массива
   // Для пустого массива может быть равен (или не равен) nullptr
  Iterator begin() noexcept {
      return Iterator{ array_ptr_.Get() };
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator end() noexcept {
      return Iterator{ array_ptr_.Get() + size_ };
  }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator begin() const noexcept {
      return  ConstIterator{ array_ptr_.Get() };
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator end() const noexcept {
      return ConstIterator{ array_ptr_.Get() + size_ };
  }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cbegin() const noexcept {
      return ConstIterator{ array_ptr_.Get() };
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cend() const noexcept {
      return ConstIterator{ array_ptr_.Get() + size_ };
  }

private:
  size_t size_ = 0;
  size_t capacity_ = 0;
  ArrayPtr<Type> array_ptr_;
};

//======================================= Перегрузка операторов сравнения =========================================
ReserveProxyObj Reserve(size_t capacity_to_reserve) {
  return ReserveProxyObj(capacity_to_reserve);
}

template<typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return lhs.GetSize() == rhs.GetSize() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return !(operator==(lhs, rhs));
}

template<typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return std::lexicographical_compare(lhs.begin(), lhs.end(),
      rhs.begin(), rhs.end());
}

template<typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return (operator==(lhs, rhs)) || (operator<(lhs, rhs));
}

template<typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return !(operator<=(lhs, rhs));
}

template<typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return (operator==(lhs, rhs)) || (operator>(lhs, rhs));
}
