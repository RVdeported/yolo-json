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

/**
 * @brief Strips the enclosing double quotes from a JSON string lexeme.
 * @param s the lexeme (e.g. @c "\"name\"")
 * @return the inner text (@c "name"), or @a s unchanged when not quoted
 */
consteval std::string_view Unquote(std::string_view s)
{
  if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
    return s.substr(1, s.size() - 2);
  return s;
}

//--------------------------------------------------------//
// SchemaType                                             //
//--------------------------------------------------------//
//! Carrier for an injected aggregate type.
//!
//! `define_aggregate` must be evaluated from a `consteval` block enclosed by
//! the scope of the type being defined ([expr.const]/31), so a function-local
//! `struct` cannot be completed by it. Instead the aggregate is materialized
//! as `Inner`, a nested class of a class template keyed by the member specs;
//! instantiating `SchemaType<Ms...>` runs the `consteval` block and completes
//! `Inner` (same shape as the GCC testsuite's json-parser.C).
template <std::meta::info... Ms> struct SchemaType
{
  struct Inner;
  consteval
  {
    std::meta::define_aggregate(^^Inner, {
                                             Ms...});
  }
};

//! Alias naming the completed aggregate directly (used with substitute).
template <std::meta::info... Ms>
using SchemaTypeInner = SchemaType<Ms...>::Inner;

//--------------------------------------------------------//
// SchemaParser                                           //
//--------------------------------------------------------//
//! The mutually recursive compile-time schema parser.
//!
//! Member functions may call each other regardless of declaration order,
//! which avoids redeclaring function templates whose return type contains a
//! reflection splice (same pattern as parser.hpp's ObjectParser).
struct SchemaParser
{
  /**
   * @brief Parses the annotation key/value pairs following a field's type.
   *
   * Called with @a idx on the token right after the type value (either a ','
   * introducing the next annotation, or the '}' closing the descriptor).
   * Consumes each @c "key": value pair and leaves @a idx on the '}' closing
   * the type descriptor.
   *
   * @tparam S constant-string reflection of the whole schema
   * @param idx current token index (updated in place)
   * @return the collected field annotations
   */
  template <std::meta::info S>
  static consteval std::vector<std::meta::info> GetSchemaAnnotations(int & idx)
  {
    constexpr auto tokens = Tokenize<S>();
    std::vector<std::meta::info> anns;
    while (tokens[idx].type == TokenType::Comma)
    {
      ExpectType(tokens[++idx], TokenType::String); // annotation key
      if (tokens[idx].text == R"("minLength")")
      {
        ExpectType(tokens[++idx], TokenType::Colon);
        MinSize a{ParseRawNum<int>(tokens[++idx].text)};
        anns.push_back(^^a);
      }
      else
      {
        ExpectType(tokens[++idx], TokenType::Colon);
        idx++; // skip the annotation value token
      }
      idx++; // consume the annotation value token
    }

    return anns;
  }

  /**
   * @brief Parses the "properties" / "required" sections of an object type and
   *        defines the corresponding aggregate.
   *
   * Called with @a idx on the ',' right after the "object" type value. Leaves
   * @a idx on the '}' closing the type descriptor.
   *
   * @tparam S constant-string reflection of the whole schema
   * @param idx current token index (updated in place)
   * @return the reflection of the defined aggregate
   */
  template <std::meta::info S>
  static consteval std::meta::info GetSchemaObject(int & idx)
  {
    constexpr auto tokens = Tokenize<S>();

    ExpectType(tokens[idx], TokenType::Comma);
    ExpectVal(tokens[++idx], R"("properties")");
    ExpectType(tokens[++idx], TokenType::Colon);
    ExpectType(tokens[++idx], TokenType::ObjectBegin);
    idx++; // skip the '{' of "properties"

    std::vector<std::pair<std::string_view, Val>> members;
    std::vector<bool> required;
    while (tokens[idx].type != TokenType::ObjectEnd)
    {
      ExpectType(tokens[idx], TokenType::String);
      auto name = Unquote(tokens[idx++].text);
      ExpectType(tokens[idx++], TokenType::Colon);
      ExpectType(tokens[idx++], TokenType::ObjectBegin);
      members.emplace_back(name, GetSchemaObj<S>(idx));
      required.emplace_back(false);
      ExpectType(tokens[idx++], TokenType::ObjectEnd);
      if (tokens[idx].type == TokenType::Comma)
        idx++;
    }
    ExpectType(tokens[idx++],
               TokenType::ObjectEnd); // consume '}' of "properties"

    if (tokens[idx].type == TokenType::Comma)
    {
      ExpectType(tokens[idx++], TokenType::Comma);
      ExpectVal(tokens[idx++], R"("required")");
      ExpectType(tokens[idx++], TokenType::Colon);
      ExpectType(tokens[idx++], TokenType::ArrayBegin);
      while (tokens[idx].type != TokenType::ArrayEnd)
      {
        ExpectType(tokens[idx], TokenType::String);
        for (int i = 0; i < required.size(); i++)
          if (Unquote(tokens[idx].text) == members[i].first)
          {
            required[i] = true;
            break;
          }
        idx++;
        if (tokens[idx].type == TokenType::Comma)
          idx++;
      }
      ExpectType(tokens[idx++], TokenType::ArrayEnd);
    }
    ExpectType(tokens[idx], TokenType::ObjectEnd);

    std::vector<std::meta::info> specs;
    for (int i = 0; i < required.size(); i++)
    {
      if (!required[i])
      {
        MayAbsent a{};
        members[i].second.ann.push_back(^^a);
      }
      std::meta::info t = members[i].second.type;
      specs.emplace_back(
          std::meta::reflect_constant(std::meta::data_member_spec(
              t, {.name = members[i].first,
                  .annotations = members[i].second.ann})));
    }

    return std::meta::substitute(^^SchemaTypeInner, specs);
  }

  /**
   * @brief Parses a type descriptor @c {"type": <T> [,...]} into a Val.
   *
   * Called with @a idx on the "type" key. Handles a plain type string, an
   * array of types (only "null" is accepted as a second element), the field
   * annotations of scalar types and the object structure of "object" types.
   * Leaves @a idx on the '}' closing the type descriptor.
   *
   * @tparam S constant-string reflection of the whole schema
   * @param idx current token index (updated in place)
   * @return the parsed Val
   */
  template <std::meta::info S> static consteval Val GetSchemaObj(int & idx)
  {
    constexpr auto tokens = Tokenize<S>();

    ExpectVal(tokens[idx++], "\"type\"");
    ExpectType(tokens[idx++], TokenType::Colon);

    // Parsing of array of types
    bool is_null = false;
    std::string_view type;
    if (tokens[idx].type == TokenType::ArrayBegin)
    {
      idx++; // consume '['
      while (tokens[idx].type != TokenType::ArrayEnd)
      {
        if (tokens[idx].text == R"("null")")
          is_null = true;
        else if (!type.empty())
          throw std::invalid_argument("Variadic types are not supported");
        else
          type = tokens[idx].text;
        idx++; // consume the type token
        if (tokens[idx].type == TokenType::Comma)
          idx++; // consume ','
      }
      idx++; // consume ']'
    }
    else
    {
      ExpectType(tokens[idx], TokenType::String);
      type = tokens[idx].text;
      idx++; // consume the type value
    }

    std::meta::info base_t;
    std::vector<std::meta::info> anns;
    if (type == R"("object")")
    {
      base_t = GetSchemaObject<S>(idx);
    }
    else if (type == R"("string")")
      base_t = ^^std::string_view;
    else if (type == R"("number")")
      base_t = ^^double;
    else if (type == R"("integer")")
      base_t = ^^int;
    else if (type == R"("boolean")")
      base_t = ^^bool;
    else if (type == R"("array")")
      base_t = ^^std::string_view;
    else
      throw std::invalid_argument(R"(Unexpected type)");
    anns = GetSchemaAnnotations<S>(idx);

    if (is_null)
      base_t = std::meta::substitute(^^std::optional, {
                                                          base_t});

    return {base_t, anns};
  }
};

} // namespace detail

template <std::meta::info S> consteval std::meta::info ParseSchema()
{
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

  return detail::SchemaParser::GetSchemaObj<S>(idx).type;
}

} // namespace yjson
