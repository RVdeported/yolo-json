
#include "yolo-json/schema.hpp"
#include "yolo-json/tokenizer.hpp"
#include <iostream>
#include <meta>
// #include <yolo-json/tokenizer.hpp>
int main()
{
  constexpr static auto s = std::meta::reflect_constant_string(R"(
{
  "type": "object",
  "properties": {
    "name": { "type": "string"},
    "age":  { "type": "integer", "minimum": 0 }
  },
  "required": ["name", "age"]
})");
  constexpr auto a = yjson::ParseSchema<s>();

  // constexpr auto tokens = yjson::Tokenize<s>();
  // for (auto v : tokens)
  //   std::cout << v.text << '\n';
  typename[:a:] t{"NAME", 34};
  std::cout << t.name << '\n';
  std::cout << std::meta::display_string_of(a) << '\n';
  std::cout << "Done\n";
  return 0;
}
