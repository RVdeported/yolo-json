//========================================================//
// Json_parser.hpp                                        //
//========================================================//
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
//! Low-level JSON token-parsing macros and helper functions.
namespace JSONParser
{
//-------------------------------------------------------------------------//
// "IsCharPtr":                                                            //
//-------------------------------------------------------------------------//
/**
 * @brief Compile-time trait detecting plain @c char pointer types.
 *
 * Primary template is @c false; explicit specializations enable it for
 * @c char* and @c char const*.
 */
template <typename T> constexpr inline bool IsCharPtr = false;
/** @brief Specialization for mutable @c char*. */
template <> constexpr inline bool IsCharPtr<char *> = true;
/** @brief Specialization for immutable @c char const*. */
template <> constexpr inline bool IsCharPtr<char const *> = true;

//=========================================================================//
// Reading, Skipping and Searching Functions:                              //
//=========================================================================//
//-------------------------------------------------------------------------//
// "ReadDouble":                                                           //
//-------------------------------------------------------------------------//
/**
 * @brief Reads a floating-point number from the buffer range.
 *
 * @tparam F floating-point type to read
 * @tparam CharPtr pointer type (must be char* or char const*)
 * @param a_from start of the number
 * @param a_to one-past-the-end of the number
 * @return the parsed value (quiet NaN when the range is invalid)
 */
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
/**
 * @brief Reads an integral number from the buffer range.
 *
 * @tparam I integral type to read
 * @tparam CharPtr pointer type (must be char* or char const*)
 * @param a_from start of the number
 * @param a_to one-past-the-end of the number
 * @return the parsed value
 */
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
/**
 * @brief Reads a number of type @a T up to the given end pointer.
 *
 * @tparam T numeric type to read
 * @tparam CharPtr pointer type (must be char* or char const*)
 * @param a_from start of the number
 * @param a_to one-past-the-end of the number
 * @param a_min_len minimum number of characters the number is guaranteed to
 *                  occupy
 * @return a pair of the parsed value and the pointer past the number
 */
template <typename T, typename CharPtr>
std::pair<T, char *> ReadNumber(CharPtr a_from, char * a_to, int a_min_len = 0)
{
  static_assert(IsCharPtr<CharPtr>);
  assert(a_from != nullptr && a_to != nullptr && a_from + a_min_len < a_to);

  char * cfrom = a_from + a_min_len;

  T res;
  if constexpr (std::is_floating_point_v<T>)
    res = ReadDouble<T>(a_from, a_to);
  else
    res = ReadInt<T>(a_from, a_to);

  return std::pair{res, a_to};
}

/**
 * @brief Reads a number of type @a T terminated by @a a_delimiter.
 *
 * @tparam T numeric type to read
 * @tparam CharPtr pointer type (must be char* or char const*)
 * @param a_from start of the number
 * @param a_delimiter delimiter character terminating the number
 * @param a_min_len minimum number of characters the number is guaranteed to
 *                  occupy
 * @return a pair of the parsed value and the pointer to the delimiter
 */
template <typename T, typename CharPtr>
std::pair<T, char *> ReadNumber(CharPtr a_from, char a_delimiter,
                                int a_min_len = 0)
{
  static_assert(IsCharPtr<CharPtr>);
  assert(a_from != nullptr);

  char * cfrom = a_from + a_min_len;
  char * number_end =
      cfrom + (std::find(cfrom, cfrom + 1000, a_delimiter) - cfrom);
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
 * @brief Finds the beginning of the value associated with @a a_key.
 *
 * @a a_key must contain the enclosing quotes. When @a InclSep is set, @a a_key
 * already includes the ':' separator and, if required, the opening quote of
 * the value; otherwise the separator is expected right after the key.
 *
 * @tparam N length of @a a_key including its NUL terminator
 * @tparam InclSep when true, @a a_key already includes the separator
 * @tparam CharPtr pointer type (must be char* or char const*)
 * @param a_key the key to search for (including enclosing quotes)
 * @param a_curr a hint where to start searching (assumed 0-terminated eventually)
 * @param a_begin the over-all message beginning (assumed 0-terminated)
 * @return pointer to the value (after the opening quote if present)
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

/**
 * @brief Skips a base value of type @a T up to the given end pointer.
 *
 * @tparam T numeric type of the value to skip
 * @tparam CharPtr pointer type (must be char* or char const*)
 * @param a_from start of the value
 * @param a_to one-past-the-end of the value
 * @param a_min_len minimum number of characters the value is guaranteed to
 *                  occupy
 * @return the pointer past the value (@p a_to)
 */
template <typename T, typename CharPtr>
CharPtr SkipVal(CharPtr a_from, char * a_to, int a_min_len = 0)
{
  char * curr = a_from;
  static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>);
  char const * cfrom = a_from;
  assert(a_from + a_min_len < a_to);
  // char const * end = std::find(cfrom + a_min_len, a_to, a_delimiter);
  return a_to;
}

/**
 * @brief Skips a base value of type @a T terminated by @a a_delimiter.
 *
 * For numeric types the delimiter is located by scanning forward; for string
 * types the (quoted) string is skipped to its terminating quote.
 *
 * @tparam T type of the value to skip
 * @tparam CharPtr pointer type (must be char* or char const*)
 * @param a_from start of the value
 * @param a_delimiter delimiter character terminating the value
 * @param a_min_len minimum number of characters the value is guaranteed to
 *                  occupy
 * @return the pointer past the value
 */
template <typename T, typename CharPtr>
CharPtr SkipVal(CharPtr a_from, char a_delimiter, int a_min_len = 0)
{
  if constexpr (std::is_floating_point_v<T> || std::is_integral_v<T>)
  {
    char * cfrom = a_from;
    char * end = cfrom + (std::find(cfrom + a_min_len, cfrom + a_min_len + 1000,
                                    a_delimiter) -
                          cfrom);
    return end;
  }
  else
  {
    char * curr = a_from;
    assert(*curr == '"');
    assert(a_min_len >= 0);
    curr += a_min_len;
    GET_STR(_);
    // For string we actually do not need the separator - it can
    // be any value
    // assert(*curr == a_delimiter);
    return curr;
  }
}
} // namespace JSONParser
// End namespace JSONParRead
