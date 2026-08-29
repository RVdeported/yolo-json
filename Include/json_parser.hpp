//===========================================================================//
//                    "json_parser.hpp":                                     //
//            Macros and Utils for Efficient Parsing of JSON Objs            //
//===========================================================================//
#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstring>
#include <format>
#include <iostream>
#include <limits>
#include <type_traits>
#include <utxx/compiler_hints.hpp>
#include <utxx/convert.hpp>
// #include <utxx/error.hpp>

//===========================================================================//
// JSON Parsing Macros:                                                      //
//===========================================================================//
//---------------------------------------------------------------------------//
// "TO_VAL": Position "curr" at the beginning of Field's Value:              //
//---------------------------------------------------------------------------//
#define TO_VAL                                                                 \
  {                                                                            \
    curr = std::strchr(curr, ':');                                             \
    if (UNLIKELY(curr == nullptr))                                             \
      std::runtime_error("No field value!");                                   \
    ++curr;                                                                    \
    while (*curr == ' ' || *curr == '\"' || *curr == '\'' || *curr == '\\')    \
      ++curr;                                                                  \
  }

//---------------------------------------------------------------------------//
// "SKP_STR": Skip a fixed (known) string:                                   //
//---------------------------------------------------------------------------//
#define SKP_STR(Str)                                                           \
  {                                                                            \
    constexpr size_t strLen = sizeof(Str) - 1;                                 \
    assert(std::strncmp(Str, curr, strLen) == 0);                              \
    curr += strLen;                                                            \
  }

//---------------------------------------------------------------------------//
// "SKP_STR_SV": Skip a std::string_view along with its enclosing quotes:    //
//---------------------------------------------------------------------------//
#define SKP_STR_SV(Sv)                                                         \
  {                                                                            \
    assert(*curr == '"');                                                      \
    curr;                                                                      \
    assert(std::memcmp(Sv.data(), curr + 1, Sv.size()) == 0);                  \
    curr += Sv.size() + 1;                                                     \
    assert(*curr == '"');                                                      \
    ++curr;                                                                    \
  }

//---------------------------------------------------------------------------//
// "SKP_IF_STR": Compare with fixed (known) string and shift "Msg" pointer   //
//               if true                                                     //
//---------------------------------------------------------------------------//
#define SKP_IF_STR_G(Str)                                                      \
  (std::strncmp(curr + 1, Str, sizeof(Str) - 1) == 0 &&                        \
   (curr += sizeof(Str) + 1, true))

#define SKP_IF_STR_U(Str) UNLIKELY(SKP_IF_STR_G(Str))
#define SKP_IF_STR_L(Str) LIKELY(SKP_IF_STR_G(Str))
#define SKP_IF_STR(Str) SKP_IF_STR_L(Str)

#define SKP_IF_SV_G(Sv)                                                        \
  (std::memcmp(Sv.data(), curr + 1, Sv.size()) == 0 &&                         \
   (curr += Sv.size() + 2, true))

#define SKP_IF_SV_U(Sv) UNLIKELY(SKP_IF_SV_G(Sv))
#define SKP_IF_SV_L(Sv) LIKELY(SKP_IF_SV_G(Sv))
#define SKP_IF_SV(Sv) SKP_IF_SV_L(Sv)

//---------------------------------------------------------------------------//
// "SKP_SPC": Skip white space:                                              //
//---------------------------------------------------------------------------//
#define SKP_SPC()                                                              \
  {                                                                            \
    while (isspace(*curr))                                                     \
      ++curr;                                                                  \
  }

//---------------------------------------------------------------------------//
// "GET_STR": "Var" will hold a 0-terminated string:                         //
//---------------------------------------------------------------------------//
#define GET_STR(Var)                                                           \
  char const * Var = ++curr;                                                   \
  while (true)                                                                 \
  {                                                                            \
    short slash_cnt = 0;                                                       \
    curr = std::strchr(curr, '"');                                             \
    assert(curr != nullptr);                                                   \
    while (*(--curr) == '\\')                                                  \
    {                                                                          \
      slash_cnt++;                                                             \
    }                                                                          \
    curr += slash_cnt + 1;                                                     \
    if (slash_cnt % 2 == 0)                                                    \
      break;                                                                   \
    ++curr;                                                                    \
  }                                                                            \
  /* 0-terminate the Var string: */                                            \
  *curr = '\0';                                                                \
  ++curr;

//---------------------------------------------------------------------------//
// "GET_BOOL":                                                               //
//---------------------------------------------------------------------------//
#define GET_BOOL(Var)                                                          \
  bool Var = *curr == 't';                                                     \
  assert((std::strncmp("false", curr, 5) == 0) ||                              \
         (std::strncmp("true", curr, 4) == 0));                                \
  curr += (Var ? 4 : 5);

//---------------------------------------------------------------------------//
// "CMP_STR": Compare with fixed (known) string and shift "Msg" pointer      //
//  if true                                                                  //
//---------------------------------------------------------------------------//
#define CMP_STR(Msg, Str)                                                      \
  (std::strncmp(Msg, Str, sizeof(Str) - 1) == 0 &&                             \
   (Msg += sizeof(Str) - 1, true))

//===========================================================================//
// Utils:                                                                    //
//===========================================================================//
namespace JSONParser
{
//-------------------------------------------------------------------------//
// "IsCharPtr":                                                            //
//-------------------------------------------------------------------------//
template <typename T> constexpr inline bool IsCharPtr = false;
template <> constexpr inline bool IsCharPtr<char *> = true;
template <> constexpr inline bool IsCharPtr<char const *> = true;

//=========================================================================//
// Reading, Skipping and Searching Functions:                              //
//=========================================================================//
//-------------------------------------------------------------------------//
// "ReadDouble":                                                           //
//-------------------------------------------------------------------------//
template <typename F, typename CharPtr>
F ReadDouble(CharPtr a_from, char const * a_to)
{
  static_assert(std::is_floating_point_v<F> && IsCharPtr<CharPtr>);
  assert(a_from != nullptr && a_to != nullptr && a_from < a_to);

  F v = std::numeric_limits<F>::quiet_NaN();
  auto after = utxx::atof<F>(a_from, a_to, v);

  return v;
}

//-------------------------------------------------------------------------//
// "ReadInt":                                                              //
//-------------------------------------------------------------------------//
template <typename I, typename CharPtr>
I ReadInt(CharPtr a_from, char const * a_to)
{
  static_assert(std::is_integral_v<I> && IsCharPtr<CharPtr>);
  assert(a_from != nullptr && a_to != nullptr && a_from < a_to);

  I v = 0;
  auto after = utxx::fast_atoi<I, false>(a_from, a_to, v);

  return v;
}

//-------------------------------------------------------------------------//
// "ReadNumber":                                                           //
//-------------------------------------------------------------------------//
template <typename T, typename CharPtr>
std::pair<T, char *> ReadNumber(CharPtr a_from, char const * a_to,
                                char a_delimiter, int a_min_len = 0)
{
  static_assert(IsCharPtr<CharPtr>);
  assert(a_from != nullptr && a_to != nullptr && a_from + a_min_len < a_to);

  char * cfrom = a_from + a_min_len;
  char * number_end =
      cfrom + (std::find((char const *)cfrom, a_to, a_delimiter) - cfrom);
  assert(number_end < a_to);
  assert(*number_end == a_delimiter);

  T res;
  if constexpr (std::is_floating_point_v<T>)
    res = ReadDouble<T>(a_from, number_end);
  else
    res = ReadInt<T>(a_from, number_end);

  return std::pair{res, number_end};
}

//-------------------------------------------------------------------------//
// "FindVal":                                                              //
//-------------------------------------------------------------------------//
/**
 * Find the beginning of the value after @a_key (which MUST contain enclosing
 * ""s!), ie:
 * @a_key:"*value*"
 * @a_begin  is the over-all msg beginning (assumed to be 0-terminated)
 * @a a_curr is a hint where to start searching
 * If InclSep is set, we assume that @a_key already includes the ':' separator
 * and, if required, the opening quote of the value.
 * Returns the ptr to value (after the opening quote if present):
 */
template <int N, bool InclSep = true, typename CharPtr>
CharPtr FindVal(char const (&a_key)[N],
                CharPtr a_curr, // Assumed to be 0-terminated eventually!
                CharPtr a_begin)
{
  static_assert(IsCharPtr<CharPtr> && N > 0);
  assert(a_curr != nullptr && a_begin != nullptr && a_begin <= a_curr);

  // First, try searching from the "a_curr" fwd; if not found, then from
  // "a_begin":
  CharPtr it = std::strstr(a_curr, a_key);
  if (UNLIKELY(it == nullptr))
    it = std::strstr(a_begin, a_key);

  // If still not found, it is an error:
  if (UNLIKELY(it == nullptr))
    std::runtime_error(std::format("{} not found in {}", a_key, a_begin));

  // Prior to "it", there must be a fld or msg delimiter:
  assert(*(it - 1) == ',' || *(it - 1) == '{');

  // Move to the value (NB: "N" includes the 0-terminator of "a_key"):
  if constexpr (InclSep)
    it += (N - 1);
  else
  {
    it += N;
    assert(*(it - 1) == ':');

    // Skip a possible opening quote:
    if (*it == '"')
      ++it;
  }
  return it;
}

// Skip of the base val
template <typename T, typename CharPtr>
CharPtr SkipVal(CharPtr a_from, char const * a_to, char a_delimiter,
                int a_min_len = 0)
{
  char * curr = a_from;
  if constexpr (std::is_floating_point_v<T> || std::is_integral_v<T>)
  {
    char const * cfrom = a_from;
    assert(a_from + a_min_len < a_to);
    char const * end = std::find(cfrom + a_min_len, a_to, a_delimiter);
    return a_from + (end - cfrom);
  }
  else
  {
    assert(*curr == '"');
    assert(a_min_len >= 0);
    curr += a_min_len;
    GET_STR(_);
    assert(*curr == a_delimiter);
    return curr;
  }
}
} // namespace JSONParser
// End namespace JSONParRead
