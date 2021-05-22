#pragma once
#include "worker_interface.h"
#include "xml_parse.h"
#include "xml_serialize.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace worker {
namespace file_operation {
enum class Result { Success, NoData, EmptyPath, FileOpenError, FileIOError };

template <class ConcreteWorker>
class FileWorker : public AllocatedChainWorker<ConcreteWorker, Result> {
 public:
  using MyBase = AllocatedChainWorker<ConcreteWorker, Result>;
};

class EmptyPathChecker : public FileWorker<EmptyPathChecker> {
 public:
  using MyBase = FileWorker<EmptyPathChecker>;
  using chain_worker_holder = MyBase::chain_worker_holder;

 public:
  EmptyPathChecker(std::string_view path);
  void Process(Result& result) override;
  static chain_worker_holder make_instance(std::string_view path);

 private:
  std::string_view m_path;
};

class OpenerForReading : public FileWorker<OpenerForReading> {
 public:
  using MyBase = FileWorker<OpenerForReading>;
  using chain_worker_holder = MyBase::chain_worker_holder;

 public:
  OpenerForReading(std::ifstream& in, std::string_view path);
  void Process(Result& result) override;
  static chain_worker_holder make_instance(std::ifstream& in,
                                           std::string_view path);

 private:
  std::ifstream& m_input;
  std::string_view m_path;
};

class XmlReader : public FileWorker<XmlReader> {
 public:
  using MyBase = FileWorker<XmlReader>;
  using chain_worker_holder = MyBase::chain_worker_holder;

 public:
  XmlReader(xml::Reader& reader,
            xml::Document& target,
            std::optional<xml::allocator_holder> external_alloc);
  void Process(Result& result) override;
  static chain_worker_holder make_instance(
      xml::Reader& reader,
      xml::Document& target,
      std::optional<xml::allocator_holder> external_alloc = std::nullopt);

 private:
  xml::Reader& m_reader;
  xml::Document& m_doc;
  xml::allocator_holder m_external_alloc;
};

class OpenerForWriting : public FileWorker<OpenerForWriting> {
 public:
  using MyBase = FileWorker<OpenerForWriting>;
  using chain_worker_holder = MyBase::chain_worker_holder;

 public:
  OpenerForWriting(std::ofstream& out, std::string_view path);
  void Process(Result& result) override;
  static chain_worker_holder make_instance(std::ofstream& out,
                                           std::string_view path);

 private:
  std::ofstream& m_output;
  std::string_view m_path;
};

class XmlWriter : public FileWorker<XmlWriter> {
 public:
  using MyBase = FileWorker<XmlWriter>;
  using chain_worker_holder = MyBase::chain_worker_holder;

 public:
  XmlWriter(xml::Writer& writer, const xml::Document& source);
  void Process(Result& result) override;
  static chain_worker_holder make_instance(xml::Writer& writer,
                                           const xml::Document& source);

 private:
  xml::Writer& m_writer;
  const xml::Document& m_doc;
};

class PipelineBuilder : public PipelineBuilderBase<PipelineBuilder, Result> {
 public:
  using MyBase = PipelineBuilderBase<PipelineBuilder, Result>;
  using chain_worker_holder = typename MyBase::chain_worker_holder;

 public:
  PipelineBuilder& CheckPath(std::string_view path);
  PipelineBuilder& OpenForReading(std::ifstream& in, std::string_view path);
  PipelineBuilder& OpenForWriting(std::ofstream& out, std::string_view path);

  PipelineBuilder& ReadXml(
      xml::Reader& reader,
      xml::Document& target,
      std::optional<xml::allocator_holder> external_alloc = std::nullopt);
  PipelineBuilder& WriteXml(xml::Writer& writer, const xml::Document& source);
};
}  // namespace file_operation
}  // namespace worker
