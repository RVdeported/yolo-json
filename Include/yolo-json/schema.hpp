#pragma once
#include "tokenizer.hpp"
#include "utils.hpp"
#include <array>
#include <charconv>
#include <cstdlib>
#include <format>
#include <meta>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace yjson
{

//========================================================//
// Helper functions                                       //
//========================================================//

// TODO: make the traceback
consteval void ExpectType(const Token a_in, const TokenType a_t)
{
  if (a_in.type != a_t)
    throw std::invalid_argument("Schema encountered unexpected token");
}

// TODO: make the traceback (I think there in utxx there was something
// useful)
consteval void ExpectVal(const Token a_in, const std::string_view a_v)
{
  if (a_in.text != a_v)
    throw std::invalid_argument("Schema have unexpected val");
}

//========================================================//
// Internal schema parser implementation                  //
//========================================================//
namespace detail
{

//--------------------------------------------------------//
// Val                                                    //
//--------------------------------------------------------//
//! @class Val 
//! @brief Holds result of schema parcing
struct Val
{
  //! @brief Type of the object
  std::meta::info type;             
  //! @brief List of annotations extracted
  std::vector<std::meta::info> ann; 
};

//--------------------------------------------------------//
// ParseRawNum                                            //
//--------------------------------------------------------//
//! @brief Number concept
template <typename T>
concept Number = std::integral<T> || std::floating_point<T>;

//! @brief Consteval parser of numbers
//! @param a_sv string to parse
//! @tparam T type of a number
//! @return Parsed number
template <typename T>
consteval T ParseRawNum(std::string_view a_sv)
  requires Number<T>
{
  T out{};
  // XXX: Do we care if exception?
  (void)std::from_chars(a_sv.data(), a_sv.data() + a_sv.size(), out);

  return out;
}


//--------------------------------------------------------//
// Unquote                                                //
//--------------------------------------------------------//
//! @brief Strips the enclosing double quotes from a JSON string lexeme.
//! @param s the lexeme (e.g. @c "\"name\"")
//! @return the inner text (@c "name"), or @a s unchanged when not quoted
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
//! the scope of the type being defined, so a function-local
//! `struct` cannot be completed by it. Instead the aggregate is materialized
//! as `Inner`, a nested class of a class template keyed by the member specs;
//! instantiating `SchemaType<Ms...>` runs the `consteval` block and completes
//! `Inner`.
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
        anns.push_back(std::meta::reflect_constant(
            MinSize{ParseRawNum<int>(tokens[++idx].text)}));
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
        members[i].second.ann.push_back(
            std::meta::reflect_constant(MayAbsent{}));
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
   * @brief Parses the "prefixItems" / "items" / "minItems" / "maxItems"
   *        sections of an array type and selects the matching container type.
   *
   * Called with @a idx on the ',' right after the "array" type value. Leaves
   * @a idx on the '}' closing the type descriptor.
   *
   * The resulting type is:
   *  - @c std::vector<T> for a homogeneous "items" schema of unbounded length;
   *  - @c std::array<T, N> for a homogeneous "items" schema whose "minItems"
   *    equals "maxItems";
   *  - @c std::tuple<Ts...> for "prefixItems" (or a draft-07 "items" array),
   *    with a trailing @c std::vector<T> when "items" is also present.
   *
   * When "minItems" and "maxItems" are both given and equal, the returned
   * @c Val also carries a @c StaticSize annotation, so the field is parsed by
   * the fixed-count unrolled parser.
   *
   * @tparam S constant-string reflection of the whole schema
   * @param idx current token index (updated in place)
   * @return the selected container type together with its field annotations
   */
  template <std::meta::info S> static consteval Val GetSchemaArray(int & idx)
  {
    constexpr auto tokens = Tokenize<S>();

    std::vector<std::meta::info> prefix; // element types of the tuple part
    std::meta::info items{};             // homogeneous element type
    bool has_items = false;
    int min_items = -1;
    int max_items = -1;

    while (tokens[idx].type == TokenType::Comma)
    {
      idx++;                              // consume ','
      ExpectType(tokens[idx], TokenType::String); // key
      std::string_view key = tokens[idx].text;
      idx++;                              // consume key
      ExpectType(tokens[idx], TokenType::Colon);
      idx++;                              // consume ':'

      if (key == R"("prefixItems")")
      {
        ExpectType(tokens[idx], TokenType::ArrayBegin);
        idx++;                            // consume '['
        while (tokens[idx].type != TokenType::ArrayEnd)
        {
          ExpectType(tokens[idx], TokenType::ObjectBegin);
          idx++;                          // consume '{'
          Val v = GetSchemaObj<S>(idx);
          ExpectType(tokens[idx], TokenType::ObjectEnd);
          idx++;                          // consume '}'
          prefix.emplace_back(v.type);
          if (tokens[idx].type == TokenType::Comma)
            idx++;
        }
        ExpectType(tokens[idx], TokenType::ArrayEnd);
        idx++;                            // consume ']'
      }
      else if (key == R"("items")")
      {
        if (tokens[idx].type == TokenType::ArrayBegin)
        {
          // Draft-07 style tuple: "items" given as an array of schemas.
          idx++;                          // consume '['
          while (tokens[idx].type != TokenType::ArrayEnd)
          {
            ExpectType(tokens[idx], TokenType::ObjectBegin);
            idx++;                        // consume '{'
            Val v = GetSchemaObj<S>(idx);
            ExpectType(tokens[idx], TokenType::ObjectEnd);
            idx++;                        // consume '}'
            prefix.emplace_back(v.type);
            if (tokens[idx].type == TokenType::Comma)
              idx++;
          }
          ExpectType(tokens[idx], TokenType::ArrayEnd);
          idx++;                          // consume ']'
        }
        else
        {
          ExpectType(tokens[idx], TokenType::ObjectBegin);
          idx++;                          // consume '{'
          Val v = GetSchemaObj<S>(idx);
          ExpectType(tokens[idx], TokenType::ObjectEnd);
          idx++;                          // consume '}'
          items = v.type;
          has_items = true;
        }
      }
      else if (key == R"("minItems")")
      {
        ExpectType(tokens[idx], TokenType::Number);
        min_items = ParseRawNum<int>(tokens[idx].text);
        idx++;                            // consume the number
      }
      else if (key == R"("maxItems")")
      {
        ExpectType(tokens[idx], TokenType::Number);
        max_items = ParseRawNum<int>(tokens[idx].text);
        idx++;                            // consume the number
      }
      else
      {
        idx++; // skip the unsupported keyword's scalar value
      }
    }

    ExpectType(tokens[idx], TokenType::ObjectEnd); // leave idx on '}'

    // A matching "minItems"/"maxItems" pair fixes the element count, so tag
    // the container with a StaticSize annotation for the unrolled parser.
    std::vector<std::meta::info> anns;
    if (min_items >= 0 && min_items == max_items)
      anns.push_back(std::meta::reflect_constant(StaticSize{min_items}));

    std::meta::info ret;
    if (!prefix.empty())
    {
      if (has_items)
        prefix.emplace_back(std::meta::substitute(^^std::vector, {items}));
      ret = std::meta::substitute(^^std::tuple, prefix);
    }
    else if (has_items)
    {
      if (max_items > 0)
        ret = std::meta::substitute(
            ^^std::array, {items, std::meta::reflect_constant(max_items)});
      else
        ret = std::meta::substitute(^^std::vector, {items});
    }
    else
    {
      throw std::invalid_argument(
          R"(Array schema has neither "items" nor "prefixItems")");
    }

    return {ret, anns};
  }

  /**
   * @brief Parses a type descriptor @c {"type": <T> [,...]} into a Val.
   *
   * Called with @a idx on the "type" key. Handles a plain type string, an
   * array of types (only "null" is accepted as a second element), the field
   * annotations of scalar types, the object structure of "object" types and
   * the array structure of "array" types.
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
    {
      Val v = GetSchemaArray<S>(idx);
      base_t = v.type;
      anns.insert(anns.end(), v.ann.begin(), v.ann.end());
    }
    else
      throw std::invalid_argument(R"(Unexpected type)");
    
    // XXX: We actually need to get annotations only for basic types.
    // The other cases should be handled within their own parsers
    std::vector<std::meta::info> field_anns = GetSchemaAnnotations<S>(idx);
    anns.insert(anns.end(), field_anns.begin(), field_anns.end());

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
