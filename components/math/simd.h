#pragma once

#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <type_traits>

//Todo what about not MSFT?
#define VECTORCALL __vectorcall

namespace tocs {
namespace math {

class simd_enabled {};

class simd_disabled {};

namespace detail {

template <class>
struct is_simd_type : public std::false_type
{};

template <>
struct is_simd_type<float> : public std::true_type
{};

//template <>
//struct is_simd_type<int> : public std::true_type
//{};


template <class T>
using enable_if_simd_type = std::enable_if<is_simd_type<T>::value>;

template <class T>
class simd_pack
{
};

template <>
class alignas(16) simd_pack<float>
{
public:
	typedef __m128 internal_pack_type;
private:
	internal_pack_type pack;
	explicit simd_pack(internal_pack_type pack)
		: pack(pack) {}
public:
	static const constexpr int element_count = 4;

	simd_pack() 
		: pack(_mm_setzero_ps())
	{}

	simd_pack(float v)
		: pack(_mm_set1_ps(v))
	{}

	simd_pack(float x, float y, float z, float w)
		: pack(_mm_setr_ps(x,y,z,w))
	{

	}

	inline simd_pack<float> VECTORCALL add(simd_pack<float> rhs) const
	{
		return simd_pack<float>(_mm_add_ps(pack, rhs.pack));
	}

	inline simd_pack<float> VECTORCALL h_add(simd_pack<float> rhs) const
	{
		return simd_pack<float>(_mm_hadd_ps(pack, rhs.pack));
	}

	inline simd_pack<float> VECTORCALL sub(simd_pack<float> rhs) const
	{
		return simd_pack<float>(_mm_sub_ps(pack, rhs.pack));
	}

	inline simd_pack<float> VECTORCALL c_mul(simd_pack<float> rhs) const
	{
		return simd_pack<float>(_mm_mul_ps(pack, rhs.pack));
	}

	inline simd_pack<float> VECTORCALL c_div(simd_pack<float> rhs) const
	{
		return simd_pack<float>(_mm_div_ps(pack, rhs.pack));
	}

	inline simd_pack<float> VECTORCALL c_less(simd_pack<float> rhs) const
	{
		return simd_pack<float>(_mm_cmplt_ps(pack, rhs.pack));
	}

	inline static simd_pack<float> VECTORCALL blend(simd_pack<float> a, simd_pack<float> b, simd_pack<float> mask)
	{
		return simd_pack<float>(_mm_blendv_ps(a.pack, b.pack, mask.pack));
	}

	template <int xi, int yi, int zi, int wi>
	inline simd_pack<float> VECTORCALL swizzle() const
	{
		return simd_pack<float>(_mm_shuffle_ps(pack, pack, _MM_SHUFFLE(xi, yi, zi, wi)));
	}

	template <int xi, int yi, int zi, int wi>
	inline static simd_pack<float> VECTORCALL shuffle(simd_pack<float> a, simd_pack<float> b)
	{
		return simd_pack<float>(_mm_shuffle_ps(a.pack, b.pack, _MM_SHUFFLE(xi, yi, zi, wi)));
	}

	template <unsigned int shufflebytes>
	inline static simd_pack<float> VECTORCALL raw_shuffle(simd_pack<float> a, simd_pack<float> b)
	{
		return simd_pack<float>(_mm_shuffle_ps(a.pack, b.pack, shufflebytes));
	}

	template <int i>
	inline simd_pack<float> VECTORCALL set(float v) const
	{
		//assume sse 4.1
		return simd_pack<float>(_mm_insert_ps(pack, _mm_set_ss(v), i * 16));
	}

	template <int i>
	inline float VECTORCALL get() const
	{
		return _mm_cvtss_f32(swizzle<i, i, i, i>(pack));
	}

	template<>
	inline float VECTORCALL get<0>() const
	{
		return _mm_cvtss_f32(pack);
	}

	template<bool xf, bool yf, bool zf, bool wf>
	inline simd_pack<float> VECTORCALL dot(simd_pack<float> rhs) const
	{
		return simd_pack<float>(_mm_dp_ps(pack, rhs.pack, xf * 0x11 | yf * 0x22 | zf * 0x33 | wf * 0x44));
	}
};

template <class T, int i>
class scalar_simd_vector_accessor
{
	//http://codrspace.com/t0rakka/simd-scalar-accessor/

	detail::simd_pack<T> pack;
public:
	scalar_simd_vector_accessor<T, i> &operator=  (T value)
	{
		pack = detail::simd_pack<T>::set<i>(pack, value);
	}

	operator T() const
	{
		return detail::simd_pack<T>::get<i>(pack);
	}
};


}
}
}
