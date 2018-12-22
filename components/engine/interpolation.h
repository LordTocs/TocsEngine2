#pragma once

namespace tocs {
namespace engine {

template <class ValType>
class linear_interp
{
public:
	static ValType interpolate(const ValType &a, const ValType &b, float t)
	{
		return a * (1.0f - t) + b * t;
	}
};

template <class ValType>
class no_interp
{
public:
	static ValType interpolate(const ValType &a, const ValType &b, float t)
	{
		return a;
	}
};

}
}


