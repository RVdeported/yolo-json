//========================================================//
// Tokenizer.hpp                                          //
//========================================================//
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <meta>
#include <string_view>

//! Reflection-driven JSON schema tooling.
namespace yjson
{

//--------------------------------------------------------//
// TokenType                                              //
//--------------------------------------------------------//
/**
 * @brief The kind of a JSON token produced by the tokenizer.
 */
enum class TokenType : std::uint8_t
{
  ObjectBegin, //!< '{'
  ObjectEnd,   //!< '}'
  ArrayBegin,  //!< '['
  ArrayEnd,    //!< ']'
  Colon,       //!< ':'
  Comma,       //!< ','
  String,      //!< "..." (including the enclosing quotes)
  Number,      //!< numeric literal
  True,        //!< true
  False,       //!< false
  Null,        //!< null
};

/**
 * @brief Returns a human-readable name of a token kind.
 * @param t the token kind
 * @return its name as a string literal
 */
constexpr std::string_view TokenTypeName(TokenType t)
{
  switch (t)
  {
  case TokenType::ObjectBegin:
    return "ObjectBegin";
  case TokenType::ObjectEnd:
    return "ObjectEnd";
  case TokenType::ArrayBegin:
    return "ArrayBegin";
  case TokenType::ArrayEnd:
    return "ArrayEnd";
  case TokenType::Colon:
    return "Colon";
  case TokenType::Comma:
    return "Comma";
  case TokenType::String:
    return "String";
  case TokenType::Number:
    return "Number";
  case TokenType::True:
    return "True";
  case TokenType::False:
    return "False";
  case TokenType::Null:
    return "Null";
  }
  return "<unknown>";
}

//--------------------------------------------------------//
// Token                                                  //
//--------------------------------------------------------//
/**
 * @brief A single JSON token: its kind plus the raw lexeme text.
 *
 * @c text is a view into the static string reflected by the schema @c info;
 * for string tokens it includes the enclosing quotes (e.g. @c "\"name\"").
 */
struct Token
{
  /** @brief Token kind. */
  TokenType type;
  /** @brief Raw lexeme, a view into the schema's static storage. */
  std::string_view text;
};

//--------------------------------------------------------//
// Tokenize (implementation)                              //
//--------------------------------------------------------//
namespace detail
{

/**
 * @brief Checks whether @a c is JSON whitespace (space, tab, LF, CR).
 */
constexpr bool IsSpace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/**
 * @brief Recovers the NUL-terminated string reflected by @a S as a view.
 *
 * The extent of the reflected array includes the trailing NUL, so the view
 * length is one less. The returned view references static storage and remains
 * valid at runtime.
 *
 * @tparam S a constant-string reflection (@c std::meta::reflect_constant_string)
 * @return a @c std::string_view over the string (excluding the NUL)
 */
template <std::meta::info S> consteval std::string_view AsStringView()
{
  constexpr const char *data = std::meta::extract<const char *>(S);
  constexpr std::size_t len = std::meta::extent(std::meta::type_of(S), 0) - 1;
  return std::string_view{data, len};
}

/**
 * @brief Checks whether the source matches the literal @a lit at offset @a i.
 *
 * @tparam N length of @a lit including its NUL terminator
 */
template <std::size_t N>
consteval bool Matches(std::string_view sv, std::size_t i, const char (&lit)[N])
{
  if (i + (N - 1) > sv.size())
    return false;
  for (std::size_t k = 0; k < N - 1; ++k)
    if (sv[i + k] != lit[k])
      return false;
  return true;
}

/**
 * @brief Length of a quoted string starting at @a i (where @c sv[i] == '"'),
 *        including both enclosing quotes.
 *
 * @return the token length, or 0 when the string is unterminated
 */
consteval std::size_t StringLen(std::string_view sv, std::size_t i)
{
  std::size_t j = i + 1;
  while (j < sv.size())
  {
    if (sv[j] == '\\')
    {
      j += 2; // skip the escaped character
      continue;
    }
    if (sv[j] == '"')
      return j - i + 1;
    ++j;
  }
  return 0; // unterminated string
}

/**
 * @brief Length of a numeric literal starting at @a i.
 *
 * Recognises the JSON number grammar: an optional leading '-', an integer
 * part, an optional fraction and an optional exponent.
 *
 * @return the token length, or 0 when the literal is malformed
 */
consteval std::size_t NumberLen(std::string_view sv, std::size_t i)
{
  std::size_t j = i;
  if (sv[j] == '-')
    ++j;

  std::size_t digits = 0;
  while (j < sv.size() && sv[j] >= '0' && sv[j] <= '9')
  {
    ++j;
    ++digits;
  }
  if (digits == 0)
    return 0; // an integer part is required

  if (j < sv.size() && sv[j] == '.')
  {
    ++j;
    std::size_t frac = 0;
    while (j < sv.size() && sv[j] >= '0' && sv[j] <= '9')
    {
      ++j;
      ++frac;
    }
    if (frac == 0)
      return 0; // digits are required after '.'
  }

  if (j < sv.size() && (sv[j] == 'e' || sv[j] == 'E'))
  {
    ++j;
    if (j < sv.size() && (sv[j] == '+' || sv[j] == '-'))
      ++j;
    std::size_t exp = 0;
    while (j < sv.size() && sv[j] >= '0' && sv[j] <= '9')
    {
      ++j;
      ++exp;
    }
    if (exp == 0)
      return 0; // digits are required in the exponent
  }

  return j - i;
}

/**
 * @brief Length of the token starting at @a i (@c sv[i] must be non-space).
 *
 * @return the token length, or 0 when the token is malformed
 */
consteval std::size_t TokenLen(std::string_view sv, std::size_t i)
{
  switch (sv[i])
  {
  case '{':
  case '}':
  case '[':
  case ']':
  case ':':
  case ',':
    return 1;
  case '"':
    return StringLen(sv, i);
  case 't':
    return Matches(sv, i, "true") ? 4 : 0;
  case 'f':
    return Matches(sv, i, "false") ? 5 : 0;
  case 'n':
    return Matches(sv, i, "null") ? 4 : 0;
  default:
    return NumberLen(sv, i);
  }
}

/**
 * @brief Kind of the token starting at @a i (@c sv[i] must be non-space).
 */
consteval TokenType TokenTypeOf(std::string_view sv, std::size_t i)
{
  switch (sv[i])
  {
  case '{':
    return TokenType::ObjectBegin;
  case '}':
    return TokenType::ObjectEnd;
  case '[':
    return TokenType::ArrayBegin;
  case ']':
    return TokenType::ArrayEnd;
  case ':':
    return TokenType::Colon;
  case ',':
    return TokenType::Comma;
  case '"':
    return TokenType::String;
  case 't':
    return TokenType::True;
  case 'f':
    return TokenType::False;
  case 'n':
    return TokenType::Null;
  default:
    return TokenType::Number;
  }
}

/**
 * @brief Number of tokens in the source, or @c std::size_t(-1) when the
 *        source is lexically malformed.
 */
consteval std::size_t CountTokens(std::string_view sv)
{
  constexpr std::size_t npos = std::size_t(-1);
  std::size_t n = 0;
  std::size_t i = 0;
  while (i < sv.size())
  {
    if (IsSpace(sv[i]))
    {
      ++i;
      continue;
    }
    const std::size_t adv = TokenLen(sv, i);
    if (adv == 0)
      return npos;
    i += adv;
    ++n;
  }
  return n;
}

} // namespace detail

//--------------------------------------------------------//
// Tokenize                                               //
//--------------------------------------------------------//
/**
 * @brief Decomposes a compile-time JSON schema into an array of tokens.
 *
 * Runs entirely at compile time: the schema is supplied as a constant-string
 * reflection (@c std::meta::reflect_constant_string over a range of chars) and
 * the returned @c std::array holds one @c Token per lexeme, in source order.
 * Whitespace is skipped; string escapes are honoured so a quote inside an
 * escaped string does not terminate it early. Malformed input (unterminated
 * string, invalid number, stray token) is rejected with a @c static_assert.
 *
 * Every token lexeme is a view into the schema's static storage (recovered via
 * @c std::meta::extract), so the returned @c std::string_views remain valid at
 * runtime.
 *
 * @tparam S a constant-string reflection of the JSON schema
 * @return an @c std::array of Token, one per lexeme
 */
template <std::meta::info S> consteval auto Tokenize()
{
  constexpr std::string_view sv = detail::AsStringView<S>();
  constexpr std::size_t n = detail::CountTokens(sv);
  static_assert(n != std::size_t(-1),
                "yjson::Tokenize: malformed JSON schema (unterminated string, "
                "invalid number, or stray token)");

  std::array<Token, n> out{};
  std::size_t idx = 0;
  std::size_t i = 0;
  while (i < sv.size())
  {
    if (detail::IsSpace(sv[i]))
    {
      ++i;
      continue;
    }
    const std::size_t start = i;
    const std::size_t adv = detail::TokenLen(sv, i);
    out[idx++] = Token{detail::TokenTypeOf(sv, start), sv.substr(start, adv)};
    i += adv;
  }
  return out;
}

} // namespace yjson
