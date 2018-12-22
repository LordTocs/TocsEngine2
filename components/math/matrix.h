#pragma once
#include "simd.h"
#include "vector.h"

#include <cmath>
#include <limits>

namespace tocs {
namespace math {

template <class T, int rows, int cols>
class matrix
{
public:
	matrix()
	{

	}
};

namespace detail
{
	//https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html#_appendix

	template<class T>
	inline simd_pack<T> VECTORCALL Mat2Mul(simd_pack<T> vec1, simd_pack<T> vec2)
	{
		return simd_pack<T>::add(simd_pack<T>::c_mul(vec1, simd_pack<T>::swizzle<0, 3, 0, 3>(vec2)), simd_pack<T>::c_mul(simd_pack<T>::swizzle<1, 0, 3, 2>(vec1), simd_pack<T>::swizzle<2, 1, 2, 1>(vec2)));
	}

	template<class T>
	inline simd_pack<T> VECTORCALL Mat2AdjMul(simd_pack<T> vec1, simd_pack<T> vec2)
	{
		return simd_pack<T>::add(simd_pack<T>::c_mul(simd_pack<T>::swizzle<3, 3, 0, 0>(vec1), vec2), simd_pack<T>::c_mul(simd_pack<T>::swizzle<1, 1, 2, 2>(vec1), simd_pack<T>::swizzle<2, 3, 0, 1>(vec2)));
	}

	template<class T>
	inline simd_pack<T> VECTORCALL Mat2MulAdj(simd_pack<T> vec1, simd_pack<T> vec2)
	{
		return simd_pack<T>::add(simd_pack<T>::c_mul(vec1, simd_pack<T>::swizzle<3, 0, 3, 0>(vec2)), simd_pack<T>::c_mul(simd_pack<T>::swizzle<1, 0, 3, 2>(vec1), simd_pack<T>::swizzle<2, 1, 2, 1>(vec2)));
	}
}

template <class T>
class matrix<T, 4, 4>
{
public:
	union
	{
		vector_base<T, 4> rows[4];
	};

	matrix()
	{
	}

	matrix<T, 4, 4> VECTORCALL transposed()
	{
		matrix<T, 4, 4> result;
		simd_pack<T> t0, t1, t2, t3;

		t0 = simd_pack<T>::raw_shuffle<0x44>(rows[0].simd, rows[1].simd);
		t1 = simd_pack<T>::raw_shuffle<0xEE>(rows[0].simd, rows[1].simd);
		t2 = simd_pack<T>::raw_shuffle<0x44>(rows[2].simd, rows[3].simd);
		t3 = simd_pack<T>::raw_shuffle<0xEE>(rows[2].simd, rows[3].simd);

		result.rows[0].simd = simd_pack<T>::raw_shuffle<0x88>(t0, t1);
		result.rows[1].simd = simd_pack<T>::raw_shuffle<0xDD>(t0, t1);
		result.rows[2].simd = simd_pack<T>::raw_shuffle<0x88>(t2, t3);
		result.rows[3].simd = simd_pack<T>::raw_shuffle<0xDD>(t2, t3);
	
		return result;
	}

	static matrix<T, 4, 4> create_frustum(float left, float right, float bottom, float top, float near, float far)
	{
		matrix<T, 4, 4> result;

		result.rows[0] = vector_base<T, 4>(2 * near / (right - left), 0, 0, (right + left) / (right - left));
		result.rows[1] = vector_base<T, 4>(0, 2 * near / (top - bottom), 0, (top + bottom) / (top - bottom));
		result.rows[2] = vector_base<T, 4>(0, 0, -(far + near) / (far - near), (-2*far*near)/(far-near));
		result.rows[3] = vector_base<T, 4>(0,0,-1,0);

		return result;
	}

	static matrix<T, 4, 4> create_projection(float fov, float aspect_ratio, float near, float far)
	{
		float tangent = std::tan(fov / 2);
		float height = near * tangent;
		float width = height * aspect_ratio;

		return create_frustum(-width, width, -height, height, near, far);
	}

	matrix<T, 4, 4> inverse() const
	{
		//Todo check major-ness
		//https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html

		simd_pack<T> a = simd_pack<T>::shuffle<0, 1, 0, 1>(rows[0].simd, row[1].simd);
		simd_pack<T> b = simd_pack<T>::shuffle<2, 3, 2, 3>(rows[0].simd, row[1].simd);
		simd_pack<T> c = simd_pack<T>::shuffle<0, 1, 0, 1>(rows[2].simd, row[3].simd);
		simd_pack<T> d = simd_pack<T>::shuffle<2, 3, 2, 3>(rows[3].simd, row[3].simd);

		simd_pack<T> det_a = simd_pack<T>(rows[0].x * rows[1].y - rows[0].y * rows[1].x);
		simd_pack<T> det_b = simd_pack<T>(rows[0].z * rows[1].w - rows[0].w * rows[1].z);
		simd_pack<T> det_c = simd_pack<T>(rows[2].x * rows[3].y - rows[2].y * rows[3].x);
		simd_pack<T> det_d = simd_pack<T>(rows[2].z * rows[3].w - rows[2].w * rows[3].z);

		simd_pack<T> d_c = detail::Mat2AdjMul(d, c);

		simd_pack<T> a_b = detail::Mat2AdjMul(a, b);

		simd_pack<T> x = simd_pack<T>::sub(simd_pack<T>::c_mul(det_d, a), detail::Mat2Mul(b, d_c));

		simd_pack<T> w = simd_pack<T>::sub(simd_pack<T>::c_mul(det_c, d), detail::Mat2Mul(c, a_b));

		simd_pack<T> det_m = simd_pack<T>::c_mul(det_a, det_d);

		simd_pack<T> y = simd_pack<T>::sub(simd_pack<T>::c_mul(det_b, c), detail::Mat2MulAdj(d, a_b));

		simd_pack<T> z = simd_pack<T>::sub(simd_pack<T>::c_mul(det_c, b), detail::Mat2MulAdj(a, d_c));

		det_m = simd_pack<T>::add(det_m, simd_pack<T>::c_mul(det_b, det_c));

		simd_pack<T> tr = simd_pack<T>::c_mul(a_b, simd_pack<T>::swizzle<0, 2, 1, 3>(d_c));
		tr = simd_pack<T>::h_add(tr, tr);
		tr = simd_pack<T>::h_add(tr, tr);

		det_m = simd_pack<T>::sub(det_m, tr);

		const static simd_pack<T> adj_sign_mask(1.f, -1.f, -1.f, 1.f);

		simd_pack<T> r_det_m = simd_pack<T>::div(adj_sign_mask, det_m);

		x = simd_pack<T>::c_mul(x, r_det_m);
		y = simd_pack<T>::c_mul(y, r_det_m);
		z = simd_pack<T>::c_mul(z, r_det_m);
		w = simd_pack<T>::c_mul(w, r_det_m);

		matrix<T, 4, 4> result;

		result.rows[0].simd = simd_pack<T>::shuffle<3, 1, 3, 1>(x, y);
		result.rows[1].simd = simd_pack<T>::shuffle<2, 0, 2, 0>(x, y);
		result.rows[2].simd = simd_pack<T>::shuffle<3, 1, 3, 1>(z, w);
		result.rows[3].simd = simd_pack<T>::shuffle<2, 0, 2, 0>(z, w);

		return result;
	}
	
	matrix<T, 4, 4> transform_inverse() const
	{
		//Todo check major-ness
		//https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html

		matrix<T, 4, 4> result;

		simd_pack<T> t0 = simd_pack<T>::shuffle<0, 1, 0, 1>(rows[0].simd, row[1].simd);
		simd_pack<T> t1 = simd_pack<T>::shuffle<2, 3, 2, 3>(rows[0].simd, row[1].simd);
		result.rows[0].simd = simd_pack<T>::shuffle<0, 2, 0, 3>(t0, rows[2].simd);
		result.rows[1].simd = simd_pack<T>::shuffle<1, 3, 1, 3>(t0, rows[2].simd);
		result.rows[2].simd = simd_pack<T>::shuffle<0, 2, 2, 3>(t1, rows[2].simd);


		simd_pack<T> sizeSqr;
		sizeSqr = simd_pack<T>::c_mul(result.rows[0].simd, result.rows[0].simd);
		sizeSqr = simd_pack<T>::add(sizeSqr, simd_pack<T>::c_mul(result.rows[1].simd, result.rows[1].simd));
		sizeSqr = simd_pack<T>::add(sizeSqr, simd_pack<T>::c_mul(result.rows[2].simd, result.rows[2].simd));

		static simd_pack<T> one(1);
		static simd_pack<T> epislon(std::numeric_limits<T>::epsilon());

		//avoid div by zero
		simd_pack<T> rSizeSqr = simd_pack<T>::blend(simd_pack<T>::c_div(one, sizeSqr), one, simd_pack<T>::c_less(sizeSqr, epislon));

		result.rows[0].simd = simd_pack<T>::c_mul(result.rows[0].simd, rSizeSqr);
		result.rows[1].simd = simd_pack<T>::c_mul(result.rows[1].simd, rSizeSqr);
		result.rows[2].simd = simd_pack<T>::c_mul(result.rows[2].simd, rSizeSqr);

		result.rows[3].simd = simd_pack<T>::c_mul(result.rows[0].simd, simd_pack<T>::swizzle<0, 0, 0, 0>(rows[3].simd));
		result.rows[3].simd = simd_pack<T>::add(result.rows[3].simd, simd_pack<T>::c_mul(result.rows[1].simd, simd_pack<T>::swizzle<1, 1, 1, 1>(rows[3].simd)));
		result.rows[3].simd = simd_pack<T>::add(result.rows[3].simd, simd_pack<T>::c_mul(result.rows[2].simd, simd_pack<T>::swizzle<2, 2, 2, 2>(rows[3].simd)));
		result.rows[3].simd = simd_pack<T>::sub(simd_pack<T>(0,0,0,1), result.rows[3].simd);
	}

	T &operator()(int r, int c)
	{
		return rows[r][c];
	}

	const T &operator()(int r, int c) const
	{
		return rows[r][c];
	}
};




template <class T>
matrix<T, 4, 4> VECTORCALL operator* (matrix<T, 4, 4> op1, matrix<T, 4, 4> op2)
{
	//https://stackoverflow.com/questions/18499971/efficient-4x4-matrix-multiplication-c-vs-assembly/18508113#18508113

	matrix<T, 4, 4> result;
	for (int i = 0; i < 4; ++i)
	{
		simd_pack<T> brodcast0 = op1.rows[i].simd.swizzle<0, 0, 0, 0>();
		simd_pack<T> brodcast1 = op1.rows[i].simd.swizzle<1, 1, 1, 1>();
		simd_pack<T> brodcast2 = op1.rows[i].simd.swizzle<2, 2, 2, 2>();
		simd_pack<T> brodcast3 = op1.rows[i].simd.swizzle<3, 3, 3, 3>();

		auto a = simd_pack<T>::add(simd_pack<T>::c_mul(brodcast0, op2.rows[0].simd), simd_pack<T>::c_mul(brodcast1, op2.rows[1].simd));
		auto b = simd_pack<T>::add(simd_pack<T>::c_mul(brodcast2, op2.rows[2].simd), simd_pack<T>::c_mul(brodcast3, op2.rows[3].simd));

		result.rows[i].simd = simd_pack<T>::add(a, b);
	}
	return result;
}


template <class T>
class matrix<T, 3, 3>
{
public:
	union
	{
		struct
		{
			vector_base<T, 3> row0;
			vector_base<T, 3> row1;
			vector_base<T, 3> row2;
		};
	};

	matrix()
	{
	}
};



using matrix4 = matrix<float, 4, 4>;

}
}
