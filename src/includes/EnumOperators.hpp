#pragma once
#include <type_traits>

template<typename E>
inline constexpr bool has_enum_operators = false;

template<typename E> requires(has_enum_operators<E>)
constexpr E operator ~(E a){
	return E(~(std::underlying_type_t<E>)a);
}

template<typename E> requires(has_enum_operators<E>)
constexpr E operator |(E a, E b){
	return E(std::underlying_type_t<E>(a) | std::underlying_type_t<E>(b));
}

template<typename E> requires(has_enum_operators<E>)
constexpr E operator &(E a, E b){
	return E(std::underlying_type_t<E>(a) & std::underlying_type_t<E>(b));
}

template<typename E> requires(has_enum_operators<E>)
constexpr E operator ^(E a, E b){
	return E(std::underlying_type_t<E>(a) ^ std::underlying_type_t<E>(b));
}

template<typename E> requires(has_enum_operators<E>)
constexpr E& operator |=(E& a, E b){
	return (E&)((std::underlying_type_t<E>&)a |= std::underlying_type_t<E>(b));
}

template<typename E> requires(has_enum_operators<E>)
constexpr E& operator &=(E& a, E b){
	return (E&)((std::underlying_type_t<E>&)a &= std::underlying_type_t<E>(b));
}

template<typename E> requires(has_enum_operators<E>)
constexpr E& operator ^=(E& a, E b){
	return (E&)((std::underlying_type_t<E>&)a ^= std::underlying_type_t<E>(b));
}

template<typename E> requires(has_enum_operators<E>)
constexpr bool isSet(E a, E b){
	return (std::underlying_type_t<E>(a) & std::underlying_type_t<E>(b)) != 0;
}

template<typename E> requires(has_enum_operators<E>)
constexpr bool operator %(E a, E b) {
	return isSet(a, b);
}
