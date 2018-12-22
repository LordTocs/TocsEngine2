#pragma once
#include <type_traits>
#include <utility>

namespace tocs {

template <class T0, class T1>
class type_promotion
{
public:
	typedef decltype(std::declval<T0>() + std::declval<T1>()) type;
};

template <class T, class Enable = void>
class to_real
{};

template <class T>
class to_real<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
{
public:
	typedef T type;
};


template <class T>
class to_real<T, typename std::enable_if<std::is_integral<T>::value && sizeof(T) <= sizeof(float)>::type>
{
public:
	typedef float type;
};

template <class T>
class to_real<T, typename std::enable_if<std::is_integral<T>::value && (sizeof(T)>sizeof(float))>::type>
{
public:
	typedef double type;
};
}
