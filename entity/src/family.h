#pragma once

namespace Halley {
	class Family {
	public:
		size_t count() const;
		void* getElement(size_t n);
	};
}
