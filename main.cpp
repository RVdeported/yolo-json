#include "Include/annotations.hpp"
#include "Include/utils.hpp"
// #include "Include/json_parser.hpp"
#include <Include/parser.hpp>
#include <cassert>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <meta>
#include <ostream>
#include <print>
#include <ranges>
#include <type_traits>
#include <vector>

struct B
{
  int d = 10;
  std::vector<int> arr{1, 2, 3, 4, 5};
};

struct[[= yjson::NotCompressed{}]] A
{
  [[= yjson::Position{10}, = yjson::MayAbsent{}]] std::optional<int> a = 5;
  [[= yjson::Position{0}]] int c = 9;
  std::string ss = "temp";
};

template <class T>
concept basic_json_type =
    std::integral<T> || std::floating_point<T> ||
    std::is_same_v<T, std::string> || std::is_same_v<T, bool>;

template <class T>
concept json_array_back_type = requires(T a) {
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

template <basic_json_type T> void print_val(T & A, std::ostream & stream)
{
  std::print(stream, "{}", A);
}

template <typename T> void print_val(T & A, std::ostream & stream)
{
  print_(A, stream);
}

template <> void print_val(bool & A, std::ostream & stream)
{
  std::print(stream, "{}", A ? "true" : "false");
}

template <json_array_type T> void print_val(T & A, std::ostream & stream)
{
  std::print(stream, "[");
  for (const auto [i, v] : A | std::views::enumerate)
  {
    if (i != 0) [[likely]]
      std::print(stream, ",");
    print_val(v, stream);
  }
  std::print(stream, "]");
}

template <> void print_val(std::string & A, std::ostream & stream)
{
  std::print(stream, "\"{}\"", A);
}

template <typename T> void print_(T & A, std::ostream & stream)
{
  constexpr auto ref = ^^T;
  constexpr auto flds = std::define_static_array(
      std::meta::members_of(ref, std::meta::access_context::unchecked()));

  std::print(stream, "{{");
  constexpr auto rel_flds = std::define_static_array(std::views::filter(
      flds, [](auto & v)
      { return std::meta::has_identifier(v) && !std::meta::is_function(v); }));
  constexpr auto sz = std::ranges::size(rel_flds);
  template for (constexpr auto idx : std::views::indices(sz))
  {
    constexpr auto & member = rel_flds[idx];
    if constexpr (idx != 0)
    {
      std::print(stream, ",");
    }
    constexpr auto name = std::meta::identifier_of(member);
    auto & val = A.[:member:];
    std::print(stream, "\"{}\":", name);
    print_val(val, stream);
  }
  std::print(stream, "}}");
}

int main()
{
  static_assert(yjson::IsOption<^^std::optional<int>>());
  std::string test = "{  \"c\"  :  34 , \"a\"  : 31       ,  \"ss\":\"test\"}";
    
  auto [rest, a] = yjson::ParseJson<^^A>(test.data(), test.data() + test.size());
  
  std::cout << a.c << "|" << a.a.value_or(0) << "|" << a.ss << '\n';
  // static_assert(have[1] == -1);
  // template for (constexpr auto v : ss)
  // {
  //   // if constexpr (std::meta::has_identifier(v))
  //   {
  //     // std::cout << name << '\n';
  //     // std::cout << [:v:].data << '\n';
  //
  //     // constexpr bool aaa = (std::meta::type_of(v) ==
  //     //                       std::meta::type_of(^^yjson::CompTimeStr<5>));
  //   }
  // }
  return 0;
}
