#pragma once
#include "simd.h"
#include "vector.h"

#include <cmath>
#include <limits>

namespace tocs {
namespace math {

template <class T, int rows, int cols, class simd_toggle=simd_disabled>
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
	inline simd_pack<T> VECTORCALL mat2_mul(simd_pack<T> vec1, simd_pack<T> vec2)
	{
		return vec1.c_mul(vec2.swizzle<0, 3, 0, 3>()).add(vec1.swizzle<1, 0, 3, 2>().c_mul(vec2.swizzle<2, 1, 2, 1>()));
	}

	template<class T>
	inline simd_pack<T> VECTORCALL mat2_adj_mul(simd_pack<T> vec1, simd_pack<T> vec2)
	{
		return vec1.swizzle<3, 3, 0, 0>().c_mul(vec2).add(vec1.swizzle<1, 1, 2, 2>().c_mul(vec2.swizzle<2, 3, 0, 1>()));
	}

	template<class T>
	inline simd_pack<T> VECTORCALL mat2_mul_adj(simd_pack<T> vec1, simd_pack<T> vec2)
	{
		return vec1.c_mul(vec2.swizzle<3, 0, 3, 0>()).add(vec1.swizzle<1, 0, 3, 2>().c_mul(vec2.swizzle<2, 1, 2, 1>()));
	}
}

template <class T>
class matrix<T, 4, 4, simd_enabled>
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

		t0 = simd_pack<T>::template raw_shuffle<0x44>(rows[0].simd, rows[1].simd);
		t1 = simd_pack<T>::template raw_shuffle<0xEE>(rows[0].simd, rows[1].simd);
		t2 = simd_pack<T>::template raw_shuffle<0x44>(rows[2].simd, rows[3].simd);
		t3 = simd_pack<T>::template raw_shuffle<0xEE>(rows[2].simd, rows[3].simd);

		result.rows[0].simd = simd_pack<T>::template raw_shuffle<0x88>(t0, t1);
		result.rows[1].simd = simd_pack<T>::template raw_shuffle<0xDD>(t0, t1);
		result.rows[2].simd = simd_pack<T>::template raw_shuffle<0x88>(t2, t3);
		result.rows[3].simd = simd_pack<T>::template raw_shuffle<0xDD>(t2, t3);
	
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

		auto a = simd_pack<T>::template shuffle<0, 1, 0, 1>(rows[0].simd, row[1].simd);
		auto b = simd_pack<T>::template shuffle<2, 3, 2, 3>(rows[0].simd, row[1].simd);
		auto c = simd_pack<T>::template shuffle<0, 1, 0, 1>(rows[2].simd, row[3].simd);
		auto d = simd_pack<T>::template shuffle<2, 3, 2, 3>(rows[3].simd, row[3].simd);

		auto det_a = simd_pack<T>(rows[0].x * rows[1].y - rows[0].y * rows[1].x);
		auto det_b = simd_pack<T>(rows[0].z * rows[1].w - rows[0].w * rows[1].z);
		auto det_c = simd_pack<T>(rows[2].x * rows[3].y - rows[2].y * rows[3].x);
		auto det_d = simd_pack<T>(rows[2].z * rows[3].w - rows[2].w * rows[3].z);

		auto d_c = detail::mat2_adj_mul(d, c);

		auto a_b = detail::mat2_adj_mul(a, b);

		auto x = det_d.c_mul(a).sub(detail::mat2_mul(b, d_c));

		auto w = det_c.c_mul(d).sub(detail::mat2_mul(c, a_b));

		auto det_m = det_a.c_mul(det_d);

		auto y = det_b.c_mul(c).sub(detail::mat2_mul_adj(d, a_b));

		auto z = det_c.c_mul(b).sub(detail::mat2_mul_adj(a, d_c));

		det_m = det_m.add(det_b.c_mul(det_c));

		auto tr = a_b.c_mul(d_c.swizzle<0, 2, 1, 3>());
		tr = tr.h_add(tr);
		tr = tr.h_add(tr);

		det_m = det_m.sub(tr);

		const static simd_pack<T> adj_sign_mask(1.f, -1.f, -1.f, 1.f);

		auto r_det_m = adj_sign_mask.div(det_m);

		x = x.c_mul(r_det_m);
		y = y.c_mul(r_det_m);
		z = z.c_mul(r_det_m);
		w = w.c_mul(r_det_m);

		matrix<T, 4, 4> result;

		result.rows[0].simd = simd_pack<T>::template shuffle<3, 1, 3, 1>(x, y);
		result.rows[1].simd = simd_pack<T>::template shuffle<2, 0, 2, 0>(x, y);
		result.rows[2].simd = simd_pack<T>::template shuffle<3, 1, 3, 1>(z, w);
		result.rows[3].simd = simd_pack<T>::template shuffle<2, 0, 2, 0>(z, w);

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

		const static simd_pack<T> one(1);
		const static simd_pack<T> epislon(std::numeric_limits<T>::epsilon());

		//avoid div by zero
		auto rSizeSqr = simd_pack<T>::blend(simd_pack<T>::c_div(one, sizeSqr), one, simd_pack<T>::c_less(sizeSqr, epislon));

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

		auto a = brodcast0.c_mul(op2.rows[0].simd).add(brodcast1.c_mul(op2.rows[1].simd));
		auto b = brodcast2.c_mul(op2.rows[2].simd).add(brodcast3.c_mul(op2.rows[3].simd));

		result.rows[i].simd = a.add(b);
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



using matrix4 = matrix<float, 4, 4, simd_enabled>;

}
}
