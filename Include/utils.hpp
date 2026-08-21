
//========================================================//
// Utils.hpp                                              //
//========================================================//
#pragma once
#include <meta>

namespace yjson
{

// Base case: types are different
template <typename T, typename U> struct is_same_template : std::false_type
{
};

// Specialization: matching template template parameters
template <template <typename...> class Template, typename... Args1,
          typename... Args2>
struct is_same_template<Template<Args1...>, Template<Args2...>> : std::true_type
{
};

// Helper variable template for ease of use
template <typename T, typename U>
inline constexpr bool is_same_template_v = is_same_template<T, U>::value;

consteval bool compare_naked_templates(std::meta::info typeA,
                                       std::meta::info typeB)
{
  // If both are template specializations, compare their underlying templates
  bool has_args_a = std::meta::has_template_arguments(typeA);
  bool has_args_b = std::meta::has_template_arguments(typeB);

  auto base_a = has_args_a ? std::meta::template_of(typeA) : typeA;
  auto base_b = has_args_b ? std::meta::template_of(typeB) : typeB;

  return base_a == base_b;
}
} // namespace yjson
