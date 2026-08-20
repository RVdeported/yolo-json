#include <meta>
#include <ranges>

namespace yjson
{
template <typename T> T Parse(const char * a_in, int sz = -1) { return T{}; }

template <class T, int Idx> consteval std::meta::info GetOrderedField()
{
  constexpr auto info = ^^T;
  constexpr auto flds = std::define_static_array(
      std::meta::members_of(info, std::meta::access_context::unchecked()));

  constexpr auto rel_flds = std::define_static_array(std::views::filter(
      flds, [](auto & v)
      { return std::meta::has_identifier(v) && !std::meta::is_function(v); }));

  return info;
}

} // namespace yjson
