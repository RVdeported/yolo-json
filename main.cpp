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

#include "Include/serializer.hpp"
#include "benchmark/Src/benchmark_types.hpp"
struct[[= yjson::NotCompressed{}]] B
{
  int d = 10;
};

struct[[= yjson::NotCompressed{}]] A
{
  [[ = yjson::Position{10}, = yjson::MayAbsent{} ]] std::optional<int> a = 5;
  [[= yjson::Position{0}]] int c = 9;
  std::string ss = "temp";
  B bb;
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
struct Opt
{
  [[= yjson::MayAbsent{}]] std::optional<int> opt;
  std::array<int, 2> tp;
  [[= yjson::DisplayName{"test"}]] int rest;
};
int main()
{
  // char buf[] = R"({"opt":23,"tp":[1,4],"test":9})";
  // auto [rest, v] = yjson::ParseJson<^^Opt>(buf, buf + std::strlen(buf));
  // auto json = ::yjson::SerializeJson<^^Opt>(Opt{4, {1, 2}, 30}, 1);
  // std::cout << json << '\n';
  auto Lg = benchmark_types::MakeLargeFixed();
  std::cout << Lg.zero_padded3 << '\n';
  return 0;
}
