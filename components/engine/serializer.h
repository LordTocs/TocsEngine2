#pragma once


namespace tocs {
namespace engine {

template <class ValType>
class serializer
{
public:

	static void write(const ValType &value, void *dest)
	{
		*reinterpret_cast<ValType *> (dest) = value;
	}

	static void read(void *source, ValType &result)
	{
		result = *reinterpret_cast<ValType*>(source);
	}

	static size_t size_in_bytes() { return sizeof(ValType); }
};

}
}

