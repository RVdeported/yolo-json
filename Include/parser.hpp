#include <meta>
#include <iostream>
#include <ranges>
#include <span>
#include <print>

namespace yjson
{



template<typename T>
consteval std::span<const std::meta::info> 
GetMembers()
{
  return std::define_static_array(
    std::meta::members_of(^^T, std::meta::access_context::unchecked()));
}
template <typename T> T Parse(const char * a_in, int sz = -1) { return T{}; }


template <class T, int Idx> void GetOrderedField()
{
  constexpr auto flds = GetMembers<T>();
  //--------------------------------------------------------//
  // Collect annotations of the struct                      //
  //--------------------------------------------------------//
  static constexpr auto annots = std::define_static_array(std::meta::annotations_of(^^T));
  static_assert(annots.size() == 2);
  template for (constexpr auto ann : annots)
  {
    std::cout << std::meta::display_string_of(std::meta::type_of(ann)) << '\n';
  }


  // consteval bool req_alph =   

  // return ^^T;
}

} // namespace yjson
