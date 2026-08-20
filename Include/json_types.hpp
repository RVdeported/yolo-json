
#include "annotations.hpp"
#include <meta>
#include <string>

namespace yjson
{
//--------------------------------------------------------//
// Basic type                                             //
//--------------------------------------------------------//
template <class T>
concept basic_json_type =
    std::integral<T> || std::floating_point<T> ||
    std::is_same_v<T, std::string> || std::is_same_v<T, bool>;

//--------------------------------------------------------//
// Array types                                            //
//--------------------------------------------------------//
template <class T>
concept json_array_back_type = requires(T a) {
  a.begin();
  a.end();
  a.emplace_back();
};

template <class T>
concept json_array_old_type = requires(T a) {
  a.begin();
  a.end();
  a.emplace();
};

template <class T>
concept json_array_type = json_array_old_type<T> || json_array_back_type<T>;

//--------------------------------------------------------//
// Optional                                               //
//--------------------------------------------------------//
template <class T>
concept json_optional = requires(T t) {
  typename T::value_type;
  { t.has_value() } -> std::same_as<bool>;
  { t.value() } -> std::same_as<typename T::value_type &>;
};

} // namespace yjson
