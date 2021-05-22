#pragma once
#include "object_pool.h"

#include <stdexcept>

/***********************************************************
Шаблон ChainWorker позволяет собирать цепочки команд
(паттерн также известен как "Конвейер") и запускать их
выполнение. В отличие от ICommand, не накладывает никаких
ограничений на повтор операций, однако требует явной передачи
ссылки на целевой объект в метод Process()

Тип команды передается в виде шаблонного параметра (CRTP)
************************************************************/
template <class TargetTy>
class ChainWorker;

template <class TargetTy>
using chain_worker_holder = object_holder<ChainWorker<TargetTy>>;

template <class TargetTy>
class ChainWorker {
 public:
  using chain_worker_holder = chain_worker_holder<TargetTy>;

 public:
  virtual void Process(TargetTy& target) = 0;
  void SetNext(chain_worker_holder&& next) { m_next = move(next); }
  virtual ~ChainWorker() = default;

 protected:
  void pass_on(TargetTy& target) {
    if (m_next) {
      m_next->Process(target);
    }
  }

 private:
  chain_worker_holder m_next{MakeDummyObjectHolder<ChainWorker>()};
};

/***********************************************************
Промежуточный класс, объединяющий возможности
ObjectPool и ChainWorker
***********************************************************/
template <class ConcreteWorker, class TargetTy>
class AllocatedChainWorker
    : public ObjectPool<ChainWorker<TargetTy>, ConcreteWorker>,
      public ChainWorker<TargetTy> {
 public:
  using MyAllocationBase = ObjectPool<ChainWorker<TargetTy>, ConcreteWorker>;
  using MyWorkerBase = ChainWorker<TargetTy>;
  using chain_worker_holder = typename MyWorkerBase::chain_worker_holder;

 public:
  static chain_worker_holder make_instance() {
    return MyAllocationBase::allocate_instance();
  }
};

/***********************************************************
Базовый класс для создания построителей цепочек.
***********************************************************/
template <class ConcreteBuilder, class TargetTy>
class PipelineBuilderBase {
 public:
  using ChainWorker = ChainWorker<TargetTy>;
  using chain_worker_holder = chain_worker_holder<TargetTy>;

 public:
  chain_worker_holder Assemble() {
    if (!m_strong_head) {
      throw std::runtime_error("Empty pipeline");
    }
    m_weak_tail = nullptr;
    return std::move(m_strong_head);
  }

 protected:
  ConcreteBuilder& attach_node(chain_worker_holder&& worker) {
    if (!m_strong_head) {
      m_strong_head = std::move(worker);
      m_weak_tail = m_strong_head.get();
    } else {
      ChainWorker* old_tail = m_weak_tail;
      m_weak_tail = worker.get();
      old_tail->SetNext(move(worker));
    }
    return get_context();
  }

  ConcreteBuilder& get_context() {
    return static_cast<ConcreteBuilder&>(*this);
  }

 private:
  chain_worker_holder m_strong_head{MakeDummyObjectHolder<ChainWorker>()};
  ChainWorker* m_weak_tail{nullptr};
};
