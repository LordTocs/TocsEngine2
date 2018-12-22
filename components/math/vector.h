#pragma once
#include <type_traits>
#include "simd.h"
#include "core/type_promotion.h"

namespace tocs {
namespace math {

template <class Kernel, int Dim, class Enable=void>
class vector_base
{
	Kernel values[Dim];
public:
	
};

namespace detail
{
	template <class T, int dim, class OverrideType = vector_base<T, dim>>
	using enable_simd_vector = std::enable_if<is_simd_type<T>::value && (dim > 1 && dim < 4), OverrideType>;
}

template <class T>
class vector_base<T, 4, typename detail::enable_if_simd_type<T>::type>
{
public:
	union
	{
		detail::simd_pack<T> simd;

		detail::scalar_simd_vector_accessor<T, 0> x;
		detail::scalar_simd_vector_accessor<T, 1> y;
		detail::scalar_simd_vector_accessor<T, 2> z;
		detail::scalar_simd_vector_accessor<T, 3> w;
	};

	vector_base()
		: simd(0.0f, 0.0f, 0.0f, 0.0f)
	{}

	vector_base(T x, T y, T z, T w)
		: simd(x, y, z, w)
	{}

	template<class U>
	VECTORCALL vector_base(vector_base<U, 4> copyme)
		: simd(copyme.x, copyme.y, copyme.z, copyme.w)
	{}

	T VECTORCALL dot(vector_base<T, 4> b) const;
	to_real<T>::type length() const;
	T length_sq() const;

	vector_base<to_real<T>::type, 4> normalized() const;
};

template <class T>
class vector_base<T, 3, typename detail::enable_if_simd_type<T>::type>
{
public:
	union
	{
		detail::simd_pack<T> simd;

		detail::scalar_simd_vector_accessor<T, 0> x;
		detail::scalar_simd_vector_accessor<T, 1> y;
		detail::scalar_simd_vector_accessor<T, 2> z;
	};

	vector_base()
		: simd(0.0f, 0.0f, 0.0f, 0.0f)
	{}

	vector_base(T x, T y, T z)
		: simd(x, y, z, 0.0f)
	{}

	template<class U>
	VECTORCALL vector_base(vector_base<U, 3> copyme)
		: simd(copyme.x, copyme.y, copyme.z, 0)
	{}

	vector_base<T, 3> VECTORCALL cross(vector_base<T, 3> b) const
	{
		auto a = detail::simd_pack<T>::swizzle<1, 2, 0, 0>(simd);
		auto b = detail::simd_pack<T>::swizzle<2, 0, 1, 0>(b.simd);

		auto c = detail::simd_pack<T>::swizzle<1, 2, 0, 0>(b.simd);
		auto d = detail::simd_pack<T>::swizzle<2, 0, 1, 0>(simd);

		auto ab = detail::simd_pack<T>::c_mul(a, b);
		auto cd = detail::simd_pack<T>::c_mul(c, d);

		return detail::simd_pack<T>::sub(ab, cd);
	}

	T VECTORCALL dot(vector_base<T, 3> b) const;
	to_real<T>::type length() const;
	T length_sq() const;
	vector_base<to_real<T>::type, 3> normalized() const;

	static vector_base<T, 3> forward;
	static vector_base<T, 3> up;
	static vector_base<T, 3> left;
};

template <class T>
vector_base<T, 3> vector_base<T, 3>::forward{ 0,0,1 };
template <class T>
vector_base<T, 3> vector_base<T, 3>::up{ 0,1,0 };
template <class T>
vector_base<T, 3> vector_base<T, 3>::left{ 1,0,0 };

template <class T>
class vector_base<T, 2, typename detail::enable_if_simd_type<T>::type>
{
public:
	union
	{
		detail::simd_pack<T> simd;

		detail::scalar_simd_vector_accessor<T, 0> x;
		detail::scalar_simd_vector_accessor<T, 1> y;
	};

	vector_base()
		: simd(0.0f, 0.0f, 0.0f, 0.0f)
	{}

	vector_base(T x, T y)
		: simd(x, y, 0, 0)
	{}

	template<class U>
	VECTORCALL vector_base(vector_base<U, 2> copyme)
		: simd(copyme.x, copyme.y, 0, 0)
	{}

	T VECTORCALL dot(vector_base<T, 2> b) const;
	to_real<T>::type length() const;
	T length_sq() const;
	vector_base<to_real<T>::type, 2> normalized() const;
};

//Operators

template <class T, int dim>
typename detail::enable_simd_vector<T,dim>::type VECTORCALL operator+ (vector_base<T, dim> a, vector_base<T, dim> b)
{
	return vector_base<float, dim>(detail::simd_pack<T>::add(a, b));
}

template <class T, int dim>
typename detail::enable_simd_vector<T, dim>::type VECTORCALL operator- (vector_base<T, dim> a, vector_base<T, dim> b)
{
	return vector_base<T, dim>(detail::simd_pack<T>::sub(a, b));
}

template <class T, int dim>
typename detail::enable_simd_vector<T, dim>::type VECTORCALL operator* (vector_base<T, dim> a, T b)
{
	return vector_base<T, dim>(detail::simd_pack<T>::c_mul(a.simd, detail::simd_pack<T>(b)));
}

template <class T, int dim>
typename detail::enable_simd_vector<T, dim>::type VECTORCALL operator/ (vector_base<T, dim> a, T b)
{
	return vector_base<T, dim>(detail::simd_pack<T>::c_div(a.simd, detail::simd_pack<T>(b)));
}

template <class T, class U, int dim>
typename detail::enable_simd_vector<typename type_promotion<T,U>::type, dim>::type VECTORCALL operator* (vector_base<T, dim> a, U b)
{
	typedef type_promotion<T, U>::type result_kernel_type;
	typedef vector_base<result_kernel_type, dim> result_type;

	result_type v = a;

	return result_type(detail::simd_pack<result_kernel_type>::c_mul(v.simd, detail::simd_pack<result_kernel_type>(b)));
}

template <class T, class U, int dim>
typename detail::enable_simd_vector<typename type_promotion<T, U>::type, dim>::type VECTORCALL operator/ (vector_base<T, dim> a, U b)
{
	typedef type_promotion<T, U>::type result_kernel_type;
	typedef vector_base<result_kernel_type, dim> result_type;

	result_type v = a;

	return result_type(detail::simd_pack<result_kernel_type>::c_div(v.simd, detail::simd_pack<result_kernel_type>(b)));
}

//Member functions
template <class T, int dim>
typename detail::enable_simd_vector<T, dim, T>::type VECTORCALL vector_base<T, dim>::dot(vector_base<T, dim> rhs) const
{
	return detail::simd_pack<T>::dot<dim >= 1, dim >= 2, dim >= 3, dim >= 4>(simd, rhs.simd).get<0>();
}


template <class T, int dim>
typename detail::enable_simd_vector<T, dim, T>::type VECTORCALL vector_base<T, dim>::length_sq() const
{
	return dot(*this);
}

template <class T, int dim>
typename detail::enable_simd_vector<T, dim, typename to_real<T>::type>::type VECTORCALL vector_base<T, dim>::length() const
{
	return std::sqrt(length_sq());
}

template <class T, int dim>
typename detail::enable_simd_vector<T, dim, typename vector_base<typename to_real<T>::type, dim>>::type VECTORCALL vector_base<T, dim>::normalized() const
{
	return vector_base<to_real<T>::type, dim>(*this) / length();
}


using vector3 = vector_base<float, 3>;
using vector2 = vector_base<float, 2>;

}
}