#pragma once

#include <halley/data_structures/vector.h>

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
		void set(int idx, TypeDeleterBase* deleter)
		{
			if (int(map.size()) <= idx) {
				map.resize(static_cast<size_t>(idx) * 3 / 2 + 1, nullptr);
			}
			map[idx] = deleter;
		}

		TypeDeleterBase* get(int uid) const
		{
			return map[uid];
		}

		bool hasComponent(int uid) const
		{
			return map.size() > size_t(uid) && map[uid] != nullptr;
		}

	private:
		Vector<TypeDeleterBase*> map;
	};

	template <typename T>
	class TypeDeleter final : public TypeDeleterBase
	{
	public:
		static void initialize(ComponentDeleterTable& table)
		{
			if (!table.hasComponent(T::componentIndex)) {
				table.set(T::componentIndex, new TypeDeleter<T>());
			}
		}

		size_t getSize() override
		{
			return sizeof(T);
		}

		void callDestructor(void* ptr) override
		{
#ifdef _MSC_VER
			ptr = ptr; // Make it shut up about unused
#endif
			static_cast<T*>(ptr)->~T();
		}
	};
}
