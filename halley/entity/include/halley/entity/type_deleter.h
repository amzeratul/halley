#pragma once

#include <vector>

namespace Halley {
	class TypeDeleterBase
	{
	public:
		virtual ~TypeDeleterBase() {}
		virtual size_t getSize() = 0;
		virtual void callDestructor(void* ptr) = 0;
	};

	class ComponentDeleterTable
	{
	public:
		static void set(int idx, TypeDeleterBase* deleter)
		{
			auto& m = *getDeleters();
			if (m.size() <= idx) {
				m.resize(static_cast<size_t>(idx * 1.5f + 1));
			}
			m[idx] = deleter;
		}

		static TypeDeleterBase* get(int uid)
		{
			return (*getDeleters())[uid];
		}

		static std::vector<TypeDeleterBase*>*& getDeleters()
		{
			static std::vector<TypeDeleterBase*>* map;
			return map;
		}
	};

	template <typename T>
	class TypeDeleter final : public TypeDeleterBase
	{
	public:
		static void initialize()
		{
			static bool initialized = false;
			if (!initialized) {
				initialized = true;
				ComponentDeleterTable::set(T::componentIndex, new TypeDeleter<T>());
			}
		}

		size_t getSize() override
		{
			return sizeof(T);
		}

		void callDestructor(void* ptr) override
		{
			ptr = ptr; // Make it shut up about unused
			static_cast<T*>(ptr)->~T();
		}
	};
}
