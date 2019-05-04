#pragma once
#include <type_traits>
#include "simd.h"
#include "core/type_promotion.h"

namespace tocs {
namespace math {


namespace detail
{
	template <class T, int dim, class Enable=void>
	class simd_vector_compatible : public std::false_type
	{};

	template <class T, int dim>
	class simd_vector_compatible <T, dim, typename std::enable_if<is_simd_type<T>::value && (dim > 1 && dim < 4)>::type> : public std::true_type
	{};

	template <class T, int dim>
	using enable_if_simd_vector = std::enable_if<simd_vector_compatible<T, dim>::value>;


	//template <class T, class U, int dim>
	//using enable_if_simd_type_promotion_vector = std::enable_if<
}

namespace detail
{
	template <class T, int Dim, class Enable=void>
	class default_simd_vector_toggle
	{
	public:
		typedef simd_disabled type;
	};

	template <class T, int Dim>
	class default_simd_vector_toggle<T, Dim, typename enable_if_simd_vector<T, Dim>>
	{
	public:
		typedef simd_enabled type;
	};
}

template <class T, int Dim, class Enable = simd_disabled>//typename detail::default_simd_vector_toggle<T, Dim>::type>
class vector_base
{
	T values[Dim];
public:

	//T dot(vector_base<T, Dim, simd_enabled> b) const;
	//typename to_real<T>::type length() const;
	//T length_sq() const;
	//vector_base<typename to_real<T>::type, Dim, simd_enabled> normalized() const;
};

//SIMD Specializations

template <class T>
class vector_base<T, 4, simd_enabled>
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
	vector_base(vector_base<U, 4> copyme)
		: simd(copyme.x, copyme.y, copyme.z, copyme.w)
	{}

	T VECTORCALL dot(vector_base<T, 4, simd_enabled> b) const;
	typename to_real<T>::type length() const;
	T length_sq() const;

	vector_base<typename to_real<T>::type, 4, simd_enabled> normalized() const;

private:
	vector_base(detail::simd_pack<T> pack)
		: simd(pack)
	{}
};

template <class T>
class vector_base<T, 3, simd_enabled>
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
	vector_base(vector_base<U, 3> copyme)
		: simd(copyme.x, copyme.y, copyme.z, 0)
	{}

	vector_base<T, 3, simd_enabled> VECTORCALL cross(vector_base<T, 3, simd_enabled> rhs) const
	{
		detail::simd_pack<T> a = simd.swizzle<1, 2, 0, 0>();
		detail::simd_pack<T> b = rhs.simd.swizzle<2, 0, 1, 0>();

		detail::simd_pack<T> c = rhs.simd.swizzle<1, 2, 0, 0>();
		detail::simd_pack<T> d = simd.swizzle<2, 0, 1, 0>();

		detail::simd_pack<T> ab = a.c_mul(b);
		detail::simd_pack<T> cd = c.c_mul(d);

		return ab.sub(cd);
	}

	T VECTORCALL dot(vector_base<T, 3, simd_enabled> b) const;
	typename to_real<T>::type length() const;
	T length_sq() const;
	vector_base<typename to_real<T>::type, 3, simd_enabled> normalized() const;

	static vector_base<T, 3, simd_enabled> forward;
	static vector_base<T, 3, simd_enabled> up;
	static vector_base<T, 3, simd_enabled> left;
private:
	vector_base(detail::simd_pack<T> pack)
		: simd(pack)
	{}
};

template <class T>
vector_base<T, 3, simd_enabled> vector_base<T, 3, simd_enabled>::forward{ 0,0,1 };
template <class T>
vector_base<T, 3, simd_enabled> vector_base<T, 3, simd_enabled>::up{ 0,1,0 };
template <class T>
vector_base<T, 3, simd_enabled> vector_base<T, 3, simd_enabled>::left{ 1,0,0 };

template <class T>
class vector_base<T, 2, simd_enabled>
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
	vector_base(vector_base<U, 2> copyme)
		: simd(copyme.x, copyme.y, 0, 0)
	{}

	T VECTORCALL dot(vector_base<T, 2> b) const;
	typename to_real<T>::type length() const;
	T length_sq() const;
	vector_base<typename to_real<T>::type, 2> normalized() const;

private:
	vector_base(detail::simd_pack<T> pack)
		: simd(pack)
	{}
};

//SIMD to SIMD Operators

template <class T, int dim>
vector_base<T, dim, simd_enabled> VECTORCALL operator+ (vector_base<T, dim, simd_enabled> a, vector_base<T, dim, simd_enabled> b)
{
	return vector_base<float, dim>(a.add(b));
}

template <class T, int dim>
vector_base<T, dim, simd_enabled> VECTORCALL operator- (vector_base<T, dim, simd_enabled> a, vector_base<T, dim, simd_enabled> b)
{
	return vector_base<T, dim>(a.sub(b));
}

//SIMD - Scalar Operators

//These have to run type promotion and preserve the if SIMD is enabled if possible.

template <class T, class U, int dim>
vector_base<typename type_promotion<T, U>::type, dim, typename detail::default_simd_vector_toggle<typename type_promotion<T, U>::type, dim>::type> VECTORCALL operator* (vector_base<T, dim, simd_enabled> a, U b)
{
	typedef type_promotion<T, U>::type result_kernel_type;
	typedef vector_base<result_kernel_type, dim> result_type;

	result_type v = a;

	return result_type(v.simd.c_mul(detail::simd_pack<result_kernel_type>(b)));
}

template <class T, class U, int dim>
vector_base<typename type_promotion<T, U>::type, dim, typename detail::default_simd_vector_toggle<typename type_promotion<T, U>::type, dim>::type> VECTORCALL operator/ (vector_base<T, dim, simd_enabled> a, U b)
{
	typedef type_promotion<T, U>::type result_kernel_type;
	typedef vector_base<result_kernel_type, dim> result_type;

	result_type v = a;

	return result_type(v.simd.c_div(detail::simd_pack<result_kernel_type>(b)));
}


//SIMD Member functions
template <class T, int dim>
T VECTORCALL vector_base<T, dim, simd_enabled>::dot(vector_base<T, dim, simd_enabled> rhs) const
{
	return simd.dot<(dim >= 1), (dim >= 2), (dim >= 3), (dim >= 4)>(rhs.simd).get<0>();
}

template <class T, int dim>
T VECTORCALL vector_base<T, dim, simd_enabled>::length_sq() const
{
	return dot(*this);
}

template <class T, int dim>
typename to_real<T>::type VECTORCALL vector_base<T, dim, simd_enabled>::length() const
{
	return std::sqrt(length_sq());
}

template <class T, int dim>
vector_base<typename to_real<T>::type, dim> VECTORCALL vector_base<T, dim, simd_enabled>::normalized() const
{
	return vector_base<to_real<T>::type, dim>(*this) / length();
}


//Simd enabled 4 float vector.
using vector4 = vector_base<float, 4, simd_enabled>;

//Simd enabled 3 float vector.
using vector3 = vector_base<float, 3, simd_enabled>;

//Simd enabled 2 float vector.
using vector2 = vector_base<float, 2, simd_enabled>;

//using vector4i = vector_base<int, 4>;
//using vector3i = vector_base<int, 3>;
//using vector2i = vector_base<int, 2>;

//using vector4p = vector_base<float, 4, simd_disabled>;
//using vector3p = vector_base<float, 3, simd_disabled>;
//using vector2p = vector_base<float, 2, simd_disabled>;

//using vector4ip = vector_base<int, 4, simd_disabled>;
//using vector3ip = vector_base<int, 3, simd_disabled>;
//using vector2ip = vector_base<int, 2, simd_disabled>;

}
}