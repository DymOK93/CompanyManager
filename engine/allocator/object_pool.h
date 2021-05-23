#pragma once
#include "pool_allocator.h"

#include <memory>
#include <utility>

/***********************************************************
ObjectPool предоставляет возможность использования MakeDummyObjectHolder
пула для выделения памяти под объекты конкретного типа
при помощи аллокатора, реализованного в виде синглтона
(для каждой инстанциации ObjectPool<OpTy>
существует единственный экземпляр аллокатора).
object_holder позволяет хранить и удалять
указатели полиморфных типов.
Типы команд передаются в виде шаблонного параметра (CRTP)
************************************************************/
template <class Interface>
using object_holder = std::unique_ptr<Interface, void (*)(Interface*)>;

template <class Interface>
object_holder<Interface> MakeDummyObjectHolder() {
  return object_holder<Interface>(nullptr, [](Interface*) {});
}

template <class Interface, class Object = Interface>
class ObjectPool {
 public:
  using shared_allocator_t = utility::memory::PoolAllocator<Object>;
  using object_holder = object_holder<Interface>;

 protected:
  template <class... Types>  //Создает указатель unique_ptr<Interface> на объект
                             //типа Object,
  static object_holder allocate_instance(
      Types&&... args) {  //передавая в конструктор
                          //последнего все полученные
                          //аргументы
    static_assert(std::is_base_of_v<Interface, Object>,
                  "Interface must be the parent of same type of Object");
    auto& alloc{get_allocator()};
    object_holder obj_holder(alloc.allocate(1), &deleter);
    std::allocator_traits<shared_allocator_t>::construct(
        alloc,
        static_cast<Object*>(
            obj_holder.get()),  //С помощью static_assert проверено, что такое
                                //преобразование возможно
        std::forward<Types>(args)...);
    return obj_holder;
  }

 private:
  static void deleter(Interface* object) noexcept {
    auto& alloc{get_allocator()};
    std::allocator_traits<shared_allocator_t>::destroy(alloc, object);
    get_allocator().deallocate(static_cast<Object*>(object), 1);
  }
  static shared_allocator_t& get_allocator() {
    static shared_allocator_t shared_allocator;
    return shared_allocator;
  }
};
