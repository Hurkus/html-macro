#pragma once
#include <type_traits>

#define ENUM_OPERATORS(E) \
	constexpr E operator ~(E a){ return E(~(std::underlying_type_t<E>)a); } \
	constexpr E operator |(E a, E b){ return E(std::underlying_type_t<E>(a) | std::underlying_type_t<E>(b)); } \
	constexpr E operator &(E a, E b){ return E(std::underlying_type_t<E>(a) & std::underlying_type_t<E>(b)); } \
	constexpr E operator ^(E a, E b){ return E(std::underlying_type_t<E>(a) ^ std::underlying_type_t<E>(b)); } \
	constexpr E& operator |=(E& a, E b) { return (E&)((std::underlying_type_t<E>&)a |= std::underlying_type_t<E>(b)); } \
	constexpr E& operator &=(E& a, E b) { return (E&)((std::underlying_type_t<E>&)a &= std::underlying_type_t<E>(b)); } \
	constexpr E& operator ^=(E& a, E b) { return (E&)((std::underlying_type_t<E>&)a ^= std::underlying_type_t<E>(b)); } \
	constexpr bool isSet(E a, E b){ return (std::underlying_type_t<E>(a) & std::underlying_type_t<E>(b)) != 0; }
