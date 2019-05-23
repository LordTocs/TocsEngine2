#pragma once

namespace tocs {
namespace core {


//This class helps avoid static initialization fiascoes by using the zero initialization for static storage to init a pointer, and conditionally constructing the value on first access.
template <class T>
class static_storage
{
	T* value;
public:
	static_storage() {}

	~static_storage()
	{
		delete value;
		value = nullptr;
	}

	static_storage(const static_storage &) = delete;
	static_storage(static_storage &&) = delete;
	static_storage& operator=(const static_storage &) = delete;
	static_storage& operator=(static_storage &&) = delete;

	T *operator->()
	{
		conditionally_construct();
		return value;
	}

	const T *operator->() const
	{
		conditionally_construct();
		return value;
	}

	T &operator*()
	{
		conditionally_construct();
		return *value;
	}

	const T &operator*() const
	{
		conditionally_construct();
		return *value;
	}
private:
	void conditionally_construct()
	{
		if (!value)
		{
			value = new T();
		}
	}

	void conditionally_construct() const
	{
		if (!value)
		{
			const_cast<T *> (value) = new T();
		}
	}
};

}
}