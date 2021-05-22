#pragma once
#include <type_traits>
#include "xml.h"

namespace xml {
inline allocator_holder MakeDefaultAllocator() {
  return std::make_shared<allocator_t>();
}

template <class ConcreteBuilder>
class BuilderBase {
 public:
  ConcreteBuilder& SetAllocator(allocator_holder alloc) {
    m_allocator = std::move(alloc);
    return get_context<ConcreteBuilder>();  //ѕоскольку
                                            //get_context<ConcreteBuilder>() -
                                            //protected, обратитьс€ к ней смогут
                                            //только наследники BuilderBase
  }

  ConcreteBuilder& Reset() {
    m_allocator.reset();
    return get_context<ConcreteBuilder>();
  }

 protected:
  template <class OtherBuilder>
  std::enable_if_t<  //ƒополнительна€ проверка
      std::is_base_of_v<BuilderBase, OtherBuilder>,
      OtherBuilder&>
  get_context() {
    return static_cast<OtherBuilder&>(*this);
  }

  allocator_holder get_allocator() {
    return m_allocator ? m_allocator : MakeDefaultAllocator();
  }

  allocator_holder take_allocator() {
    return m_allocator ? move(m_allocator) : MakeDefaultAllocator();
  }

 private:
  allocator_holder m_allocator;
};
}  // namespace xml
