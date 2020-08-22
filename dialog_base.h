#pragma once
#include "message.h"

#include <utility>

namespace dialog {
	class DialogBase {
	public:
		DialogBase(message::Plantation& plantation)
			: m_plantation(std::addressof(plantation))
		{
		}
	protected:
		message::Factory& get_factory(message::Type type) const {						//Логическая константность объекта не наруешается
			return *(*m_plantation)[static_cast<size_t>(type)];
		}
	private:
		mutable message::Plantation* m_plantation;		
	};
}
