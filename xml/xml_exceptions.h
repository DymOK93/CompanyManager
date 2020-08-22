#pragma once
#include <string>
#include <stdexcept>

namespace xml {
    class document_builder_error : public std::runtime_error {
	public:
        using std::runtime_error::runtime_error;
	};

    class parse_error : public std::runtime_error {
	public:
         using std::runtime_error::runtime_error;
	};

    class bad_declaration : public std::runtime_error {
	public:
         using std::runtime_error::runtime_error;
	};

    class bad_attribute : public std::runtime_error {
	public:
         using std::runtime_error::runtime_error;
	};

    class bad_indent : public std::runtime_error {
	public:
         using std::runtime_error::runtime_error;
	};
}
