#pragma once
#include <type_traits>

#include "simd.h"
#include "vector.h"



namespace tocs {
namespace math {

template <class T>
class quaternion_base
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
			detail::simd_pack<T> two(2, 2, 2, 0);

			auto a = detail::simd_pack<T>::swizzle<1, 2, 0, 0>(simd);
			auto b = detail::simd_pack<T>::swizzle<2, 0, 1, 0>(vec.simd);

			auto c = detail::simd_pack<T>::swizzle<1, 2, 0, 0>(vec.simd);
			auto d = detail::simd_pack<T>::swizzle<2, 0, 1, 0>(simd);

			auto ab = detail::simd_pack<T>::c_mul(a, b);
			auto cd = detail::simd_pack<T>::c_mul(c, d);

			t = detail::simd_pack<T>::c_mul(two, detail::simd_pack<T>::sub(ab, cd));
		}

		vector_base<T, 3> v_prime;

		{
			auto qw = detail::simd_pack<T>::swizzle<3, 3, 3, 3>(simd);
			auto qwt = detail::simd_pack<T>::c_mul(qw, t);

			detail::simd_pack<T> cross;
			{
				auto a = detail::simd_pack<T>::swizzle<1, 2, 0, 0>(simd);
				auto b = detail::simd_pack<T>::swizzle<2, 0, 1, 0>(t);

				auto c = detail::simd_pack<T>::swizzle<1, 2, 0, 0>(t);
				auto d = detail::simd_pack<T>::swizzle<2, 0, 1, 0>(simd);

				auto ab = detail::simd_pack<T>::c_mul(a, b);
				auto cd = detail::simd_pack<T>::c_mul(c, d);

				cross = detail::simd_pack<T>::sub(ab, cd);
			}

			v_prime.simd = detail::simd_pack<T>::add(vec.simd, detail::simd_pack<T>::add(qwt, cross));
		}

		return v_prime;
	}

	quaternion_base<T> VECTORCALL conjugate() const
	{
		return detail::simd_pack<T>::c_mul(simd, detail::simd_pack<T>(-1, -1, -1, 1));
	}

	quaternion_base<T> VECTORCALL inverse() const
	{
		auto conj = conjugate();
		auto mag_sqr = detail::simd_pack<T>::dot<true, true, true, true>(simd, simd);
		
		return detail::simd_pack<T>::c_div(conj, mag_sqr);
	}

	quaternion_base<T> VECTORCALL operator*(quaternion_base<T> op2)
	{
		//X = W * op2.X + X * op2.W + Y * op2.Z - Z * op2.Y;
		//Y = W * op2.Y + Y * op2.W + Z * op2.X - X * op2.Z;
		//Z = W * op2.Z + Z * op2.W + X * op2.Y - Y * op2.X;
		//W = W * op2.W - X * op2.X - Y * op2.Y - Z * op2.Z;

		auto a1 = detail::simd_pack<T>::swizzle<3, 3, 3, 3>(simd);
		auto a2 = detail::simd_pack<T>::swizzle<0, 1, 2, 0>(op2.simd);
		auto a = detail::simd_pack<T>::c_mul(a1, a2);

		auto b1 = detail::simd_pack<T>::swizzle<0, 1, 2, 0>(simd);
		b1 = detail::simd_pack<T>::c_mul(b1, detail::simd_pack<T>(1, 1, 1, -1));
		auto b2 = detail::simd_pack<T>::swizzle<3, 3, 3, 0>(op2.simd);
		auto b = detail::simd_pack<T>::c_mul(b1, b2);

		auto c1 = detail::simd_pack<T>::swizzle<1, 2, 0, 1>(simd);
		c1 = detail::simd_pack<T>::c_mul(c1, detail::simd_pack<T>(1, 1, 1, -1));
		auto c2 = detail::simd_pack<T>::swizzle<2, 0, 1, 1>(op2.simd);
		auto c = detail::simd_pack<T>::c_mul(c1, c2);

		auto d1 = detail::simd_pack<T>::swizzle<2, 0, 1, 2>(simd);
		auto d2 = detail::simd_pack<T>::swizzle<1, 2, 0, 2>(op2.simd);
		auto d = detail::simd_pack<T>::c_mul(d1, d2);

		return detail::simd_pack<T>::add(a, detail::simd_pack<T>::add(b, detail::simd_pack<T>::sub(c, d)));
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