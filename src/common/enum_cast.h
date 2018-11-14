#ifndef enum_cast_h__
#define enum_cast_h__

#include <type_traits>

// yes ugly ...
template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
	return static_cast<typename std::underlying_type<E>::type>(e);
}

#endif // enum_cast_h__
