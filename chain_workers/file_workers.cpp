#include "file_workers.h"
using namespace std;

namespace worker {
	namespace file_operation {
		EmptyPathChecker::EmptyPathChecker(string_view path)
			: m_path(path)
		{
		}

		void EmptyPathChecker::Process(Result& result) {
			if (!m_path.empty()) {
				result = Result::Success;
				MyBase::pass_on(result);
			}
			else {
				result = Result::EmptyPath;
			}
		}

		EmptyPathChecker::chain_worker_holder EmptyPathChecker::make_instance(string_view path) {
			return allocate_instance(path);
		}

		OpenerForReading::OpenerForReading(ifstream& in, string_view path)
			: m_input(in), m_path(path)
		{
		}

		void OpenerForReading::Process(Result& result) {
			m_input.open(m_path.data());
			if (m_input.is_open()) {
				result = Result::Success;
				MyBase::pass_on(result);
			}
			else {
				result = Result::FileOpenError;
			}
		}

		OpenerForReading::chain_worker_holder OpenerForReading::make_instance(ifstream& in, string_view path) {
			return allocate_instance(in, path);
		}

		XmlReader::XmlReader(
			xml::Reader& reader,
			xml::Document& target,
			std::optional<xml::allocator_holder> external_alloc
		) : m_reader(reader), m_doc(target) {
			if (external_alloc) {
				m_external_alloc = move(*external_alloc);
			}
		}

		void XmlReader::Process(Result& result) {
			try {
				if (m_external_alloc) {
					m_doc = m_reader.Load(move(m_external_alloc));
				}
				else {
					m_doc = m_reader.Load();
				}
			}
			catch (...) {
				result = Result::FileIOError;
				throw;
			}
			result = m_reader.Fail() ?
				Result::FileIOError : Result::Success;
			if (result == Result::Success) {
				MyBase::pass_on(result);
			}
		}

		XmlReader::chain_worker_holder XmlReader::make_instance(
			xml::Reader& reader,
			xml::Document& target,
			std::optional<xml::allocator_holder> external_alloc
		) {
			return allocate_instance(reader, target, external_alloc);
		}

		OpenerForWriting::OpenerForWriting(ofstream& out, string_view path)
			: m_output(out), m_path(path)
		{
		}

		void OpenerForWriting::Process(Result& result) {
			m_output.open(m_path.data());
			if (m_output.is_open()) {
				result = Result::Success;
				MyBase::pass_on(result);
			}
			else {
				result = Result::FileOpenError;
			}
		}

		OpenerForWriting::chain_worker_holder OpenerForWriting::make_instance(ofstream& out, string_view path) {
			return allocate_instance(out, path);
		}

		XmlWriter::XmlWriter(xml::Writer& writer, const xml::Document& source)
			: m_writer(writer), m_doc(source)
		{
		}

		void XmlWriter::Process(Result& result) {
			try {
				m_writer.Save(m_doc);
			}
			catch (...) {
				result = Result::FileIOError;
				throw;
			}
			result = m_writer.Fail() ?
				Result::FileIOError : Result::Success;
			if (result == Result::Success) {
				MyBase::pass_on(result);
			}
		}

		XmlWriter::chain_worker_holder XmlWriter::make_instance(xml::Writer& writer, const xml::Document& source) {
			return allocate_instance(writer, source);
		}

		PipelineBuilder& PipelineBuilder::CheckPath(string_view path) {
			return MyBase::attach_node(EmptyPathChecker::make_instance(path));
		}

		PipelineBuilder& PipelineBuilder::OpenForReading(ifstream& in, string_view path) {
			return MyBase::attach_node(OpenerForReading::make_instance(in, path));
		}

		PipelineBuilder& PipelineBuilder::OpenForWriting(ofstream& out, string_view path) {
			return MyBase::attach_node(OpenerForWriting::make_instance(out, path));
		}

		PipelineBuilder& PipelineBuilder::ReadXml(
			xml::Reader& reader,
			xml::Document& target,
			std::optional<xml::allocator_holder> external_alloc) {
			return MyBase::attach_node(XmlReader::make_instance(reader, target));
		}

		PipelineBuilder& PipelineBuilder::WriteXml(xml::Writer& writer, const xml::Document& source) {
			return MyBase::attach_node(XmlWriter::make_instance(writer, source));
		}
	}	
}