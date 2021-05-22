#pragma once
#include "worker_interface.h"

#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace worker {
namespace ui_internal {
/********************************************************************************
Шаблон SingleHandlerWorker позволяет реализовывать цепочки обработчиков
для данных произвольного типа. Принимаемый в конструкторе callable-object
будет вызван в Process() с передачей аргумента TargetTy&.
Преимущество такого подхода - гибкость настройки и возможность работать с
внутренними полями и методами классов путём их захвата в lambda expressions.
Движение по цепочку продолжится только в том случае, если обработчик вернёт
true.
*********************************************************************************/
template <class TargetTy>
class SingleHandlerWorker
    : public AllocatedChainWorker<SingleHandlerWorker<TargetTy>, TargetTy> {
 public:
  using MyBase = AllocatedChainWorker<SingleHandlerWorker, TargetTy>;
  using chain_worker_holder = typename MyBase::chain_worker_holder;

 public:
  SingleHandlerWorker(std::function<bool(TargetTy&)> handler)
      : m_handler(std::move(handler)) {}
  void Process(TargetTy& target) {
    if (m_handler(target)) {
      MyBase::pass_on(target);
    }
  }
  static chain_worker_holder make_instance(
      std::function<bool(TargetTy&)> handler) {
    return MyBase::allocate_instance(std::move(handler));
  }

 protected:
  std::function<bool(TargetTy&)> m_handler;
};

template <class TargetTy>
class PipelineBuilder
    : public PipelineBuilderBase<PipelineBuilder<TargetTy>, TargetTy> {
 public:
  using MyBase = PipelineBuilderBase<PipelineBuilder, TargetTy>;
  using chain_worker_holder = typename MyBase::chain_worker_holder;

 public:
  PipelineBuilder& AddHandler(std::function<bool(TargetTy&)> handler) {
    return MyBase::attach_node(
        SingleHandlerWorker<TargetTy>::make_instance(std::move(handler)));
  }
};
}  // namespace ui_internal
}  // namespace worker
