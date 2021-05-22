#pragma once

#include <iterator>
#include <utility>

template <class Iterator>
class Range {
 public:
  using ValueType = typename std::iterator_traits<Iterator>::value_type;

 public:
  Range(Iterator begin, Iterator end)
      : m_begin{std::move(begin)}, m_end{std::move(end)} {}
  Iterator begin() const { return m_begin; }
  Iterator end() const { return m_end; }

 private:
  Iterator m_begin;
  Iterator m_end;
};