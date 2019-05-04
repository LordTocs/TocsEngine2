#pragma once
#include <type_traits>

#include "simd.h"
#include "vector.h"



namespace tocs {
namespace math {

namespace detail
{

template <class T>
using default_simd_quaternion_toggle = detail::default_simd_vector_toggle<T, 4>;

}

template <class T, class simd_toggle = typename detail::default_simd_quaternion_toggle<T>>
class quaternion_base
{

};


template <class T>
class quaternion_base<T, simd_enabled>
{
public:
	static_assert(std::is_floating_point<T>::value, "Quaternions only work with floating point types.");

	union
	{
		detail::simd_pack<T> simd;

		detail::scalar_simd_vector_accessor<T, 0> x;
		detail::scalar_simd_vector_accessor<T, 1> y;
		detail::scalar_simd_vector_accessor<T, 2> z;
		detail::scalar_simd_vector_accessor<T, 3> w;
	};

	quaternion_base()
		: simd(0,0,0,1)
	{}
private:
	quaternion_base(detail::simd_pack<T> simd)
		: simd(simd)
	{}
public:

	vector_base<T,3> VECTORCALL rotate(vector_base<T,3> vec) const
	{
		//https://blog.molecular-matters.com/2013/05/24/a-faster-quaternion-vector-multiplication/

		//t = 2 * cross(q.xyz, v)
		//v' = v + q.w * t + cross(q.xyz, t)
		
		detail::simd_pack<T> t;

		{
			const static detail::simd_pack<T> two(2, 2, 2, 0);

			auto a = simd.swizzle<1, 2, 0, 0>();
			auto b = vec.simd.swizzle<2, 0, 1, 0>();

			auto c = vec.simd.swizzle<1, 2, 0, 0>();
			auto d = simd.swizzle<2, 0, 1, 0>();

			auto ab = a.c_mul(b);
			auto cd = c.c_mul(d);

			t = two.c_mul(two, ab.sub(cd));
		}

		vector_base<T, 3> v_prime;

		{
			auto qw = detail::simd_pack<T>::swizzle<3, 3, 3, 3>(simd);
			auto qwt = detail::simd_pack<T>::c_mul(qw, t);

			detail::simd_pack<T> cross;
			{
				auto a = simd.swizzle<1, 2, 0, 0>();
				auto b = t.swizzle<2, 0, 1, 0>();

				auto c = t.swizzle<1, 2, 0, 0>();
				auto d = simd.swizzle<2, 0, 1, 0>();

				auto ab = a.c_mul(b);
				auto cd = c.c_mul(d);

				cross = ab.sub(cd);
			}

			v_prime.simd = vec.simd.add(qwt.add(cross));
		}

		return v_prime;
	}

	quaternion_base<T> VECTORCALL conjugate() const
	{
		static detail::simd_pack<T> conjugator(-1, -1, -1, 1);
		return simd.c_mul(conjugator);
	}

	quaternion_base<T> VECTORCALL inverse() const
	{
		auto conj = conjugate();
		auto mag_sqr = simd.dot<true, true, true, true>(simd);
		
		return conj.c_div(mag_sqr);
	}

	quaternion_base<T> VECTORCALL operator*(quaternion_base<T> rhs)
	{
		//X = W * op2.X + X * op2.W + Y * op2.Z - Z * op2.Y;
		//Y = W * op2.Y + Y * op2.W + Z * op2.X - X * op2.Z;
		//Z = W * op2.Z + Z * op2.W + X * op2.Y - Y * op2.X;
		//W = W * op2.W - X * op2.X - Y * op2.Y - Z * op2.Z;

		auto a1 = simd.swizzle<3, 3, 3, 3>();
		auto a2 = rhs.simd.swizzle<0, 1, 2, 0>();
		auto a = a1.c_mul(a2);

		const static detail::simd_pack<T> sign_flipper (1, 1, 1, -1);

		auto b1 = simd.swizzle<0, 1, 2, 0>();
		b1 = b1.c_mul(sign_flipper);
		auto b2 = rhs.simd.swizzle<3, 3, 3, 0>();
		auto b = b1.c_mul(b2);

		auto c1 = detail::simd_pack<T>::swizzle<1, 2, 0, 1>(simd);
		c1 = c1.c_mul(sign_flipper);
		auto c2 = rhs.simd.swizzle<2, 0, 1, 1>();
		auto c = c1.c_mul(c2);

		auto d1 = simd.swizzle<2, 0, 1, 2>();
		auto d2 = rhs.simd.swizzle<1, 2, 0, 2>();
		auto d = d1.c_mul(d2);

		return a.add(b.add(c.sub(d)));
	}

	static quaternion_base<T> VECTORCALL slerp(quaternion_base<T> from, quaternion_base<T> to, T t)
	{

	}

	static quaternion_base<T> VECTORCALL from_euler(T X, T Y, T Z)
	{

	}

	static quaternion_base<T> VECTORCALL squad(quaternion_base<T> a, quaternion_base<T> b, quaternion_base<T> c, quaternion_base<T> d, T t)
	{

	}
};


using quaternion = quaternion_base<float>;

}
}