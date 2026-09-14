#pragma once
#include "tokenizer.hpp"
#include "utils.hpp"
#include <charconv>
#include <cstdlib>
#include <format>
#include <meta>
#include <stdexcept>

namespace yjson
{

namespace detail
{
struct Val;
template <std::meta::info S> consteval Val GetShemaObj(int idx);

} // namespace detail

consteval void ExpectType(const Token a_in, const TokenType a_t)
{
  if (a_in.type != a_t)
    throw std::invalid_argument("Schema encountered unexpected token");
}

consteval void ExpectVal(const Token a_in, const std::string_view a_v)
{
  if (a_in.text != a_v)
    throw std::invalid_argument("Schema have unexpected val");
}

namespace detail
{

struct Val
{
  std::meta::info type;             // base type
  std::vector<std::meta::info> ann; // annotations
  int end_idx;
};

template <typename T>
concept Number = std::integral<T> || std::floating_point<T>;
template <typename T>
consteval T ParseRawNum(std::string_view a_sv)
  requires Number<T>
{
  T out{};
  (void)std::from_chars(a_sv.data(), a_sv.data() + a_sv.size(), out);

  return out;
}

template <std::meta::info S>
consteval std::vector<std::meta::info> GetSchemaAnnotations(int & idx)
{
  constexpr auto tokens = Tokenize<S>();
  std::vector<std::meta::info> anns;
  while (tokens[++idx].type == TokenType::Comma)
  {
    ExpectType(tokens[++idx], TokenType::String);
    if (tokens[idx].text == R"("minLength")")
    {
      ExpectType(tokens[++idx], TokenType::Colon);
      MinSize a{ParseRawNum<int>(tokens[++idx].text)};

      anns.push_back(^^a);
    }
    else
    {
      ExpectType(tokens[++idx], TokenType::Colon);
      idx++;
    }
  }

  return anns;
}

template <std::meta::info S> consteval Val GetSchemaObj(int & idx)
{
  constexpr auto tokens = Tokenize<S>();

  ExpectVal(tokens[idx++], "\"type\"");
  ExpectType(tokens[idx++], TokenType::Colon);

  // Parsing of array of types
  bool is_null = false;
  std::string_view type;
  if (tokens[idx].type == TokenType::ArrayBegin)
  {
    while (tokens[idx++].type != TokenType::ArrayEnd)
    {
      if (tokens[idx].text == R"("null")")
      {
        is_null = true;
      }
      else if (type.size() != 0)
      {
        throw std::invalid_argument("Variadic types are not supported");
      }
      else
      {
        type = tokens[idx].text;
      }
      idx++;
    }
  }
  else
  {
    ExpectType(tokens[idx], TokenType::String);
    type = tokens[idx].text;
  }
  // Annotations
  auto anns = GetSchemaAnnotations<S>(idx);

  std::meta::info base_t;
  if (type == R"("string")")
  {
    base_t = ^^std::string_view;
  }
  else if (type == R"("number")")
  {
    base_t = ^^double;
  }
  else if (type == R"("integer")")
  {
    base_t = ^^int;
  }
  else if (type == R"("object")")
  {
    base_t = ^^std::string_view;
  }
  else if (type == R"("boolean")")
  {
    base_t = ^^bool;
  }
  else if (type == R"("array")")
  {
    base_t = ^^std::string_view;
  }
  else
  {
    throw std::invalid_argument(R"(Unexpected type)");
  }

  if (is_null)
  {
    base_t = std::meta::substitute(^^std::optional, {
                                                        base_t});
  }
  // idx++;
  return {base_t, anns};
}
} // namespace detail

template <std::meta::info S> consteval std::meta::info ParseSchema()
{
  std::string_view s = detail::AsStringView<S>();

  constexpr auto tokens = Tokenize<S>();

  int idx = 0;
  if (tokens[idx++].type != TokenType::ObjectBegin)
    throw std::invalid_argument("Schema does not have opening bracet");

  //--------------------------------------------------------//
  // Metadata processing                                    //
  //--------------------------------------------------------//
  while (true)
  {
    ExpectType(tokens[idx], TokenType::String);
    if (tokens[idx].text[0] != '$')
      // metadata ends
      break;

    ExpectType(tokens[++idx], TokenType::Colon);
    ExpectType(tokens[++idx], TokenType::String);
    ExpectType(tokens[++idx], TokenType::Comma);

    // Hopefully, it is not an empty schema
    ExpectType(tokens[++idx], TokenType::String);
  }

  detail::Val res = detail::GetSchemaObj<S>(idx);

  struct A;
  [=] consteval
  {
    std::meta::define_aggregate(
        ^^A, {
                 std::meta::data_member_spec(
                     res.type, {.name = "x", .annotations = res.ann})});
  };
  return std::meta::type_of(std::meta::data_member_spec(
      res.type, {.name = "x", .annotations = res.ann}));
}

} // namespace yjson
