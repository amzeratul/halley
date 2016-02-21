#pragma once

class TypeDeleterBase
{
public:
	virtual size_t getSize() = 0;
	virtual void callDestructor(void* ptr) = 0;
};

template <typename T>
class TypeDeleter : public TypeDeleterBase
{
public:
	size_t getSize()
	{
		return sizeof(T);
	}

	void callDestructor(void* ptr)
	{
		ptr = ptr; // Make it shut up about unused
		((T*)ptr)->~T();
	}
};

class TypeUIDSizeTable
{
public:
	static void set(int uid, TypeDeleterBase* deleter)
	{
		getMap()[uid] = deleter;
	}

	static TypeDeleterBase* get(int uid)
	{
		return getMap()[uid];
	}

private:

	static std::map<int, TypeDeleterBase*>& getMap()
	{
		static std::map<int, TypeDeleterBase*> map;
		return map;
	}
};
