
//========================================================//
// Utils.hpp                                              //
//========================================================//
#pragma once
#include <meta>
#include <ranges>

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
  bool has_args_a = std::meta::has_template_arguments(typeA);
  bool has_args_b = std::meta::has_template_arguments(typeB);

  auto base_a = has_args_a ? std::meta::template_of(typeA) : typeA;
  auto base_b = has_args_b ? std::meta::template_of(typeB) : typeB;

  return base_a == base_b;
}

template <typename T>
consteval auto has_annotation(std::meta::info r, T const & value) -> bool
{
  return std::ranges::contains(annotations_of_with_type(r, ^^T),
                               std::meta::reflect_constant(value),
                               std::meta::constant_of);
}

template <typename T, typename U> consteval auto get_annotations()
{
  constexpr auto ann =
      std::define_static_array(std::meta::annotations_of_with_type(^^U, ^^T));
  return ann;
}

template <typename T, std::meta::info entity> consteval auto get_annotations()
{
  return std::define_static_array(
      std::meta::annotations_of_with_type(entity, ^^T));
}

template <typename T> consteval auto GetRelFields()
{
  constexpr auto flds = std::define_static_array(
      std::meta::members_of(^^T, std::meta::access_context::unchecked()));

  return std::define_static_array(std::views::filter(
      flds, [](auto & v)
      { return std::meta::has_identifier(v) && !std::meta::is_function(v); }));
}

} // namespace yjson
