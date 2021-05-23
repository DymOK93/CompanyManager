#pragma once
#include "cm_engine_dll_interface.hpp"

#include "file_workers.h"
#include "xml_wrappers.h"
#include "xml_wrappers_builders.h"

#include <fstream>
#include <functional>
#include <iostream>
#include <memory>

class CM_ENGINE_API CompanyManager {
 private:
  struct FileInfo {
    bool is_loaded{false};
    bool is_saved{true};
    std::string current_path;
  };

  struct XmlTree {
    xml::Document m_document;
    wrapper::Company company;
  };

 public:
  CompanyManager& Create();
  worker::file_operation::Result Load();
  worker::file_operation::Result Save();

  bool IsSaved() const noexcept;
  bool IsLoaded() const noexcept;
  explicit operator bool() const noexcept;

  std::string_view GetPath() const noexcept;
  CompanyManager& SetPath(std::string path) noexcept;
  CompanyManager& Reset() noexcept;

  const wrapper::Company& Read() const;
  wrapper::Company& Modify();  //—брасывает флаг is_saved

  xml::allocator_holder GetAllocator() const;

 private:
  bool empty_path() const noexcept;
  const std::string& get_path() const noexcept;
  void update_stats_after_create();
  void update_stats_after_load();

  static XmlTree build_default_tree();
  static xml::node_holder make_xml_declaration(xml::allocator_holder alloc);

  static wrapper::Company build_wrappers_tree(xml::Document& doc);
  static void tune_xml_writer(xml::Writer& writer);

 private:
  FileInfo m_file;
  XmlTree m_xml_tree;
};
