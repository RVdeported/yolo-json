#include "Include/annotations.hpp"
#include "Include/json_parser.hpp"
#include "Include/utils.hpp"
#include <cassert>
#include <iostream>
#include <meta>
#include <print>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace yjson
{
template <class T> consteval auto GetOrderedField()
{
  //--------------------------------------------------------//
  // Collect annotations of the struct                      //
  //--------------------------------------------------------//
  constexpr StructAnnots strAnnots = StructAnnots::MkStrAnnots<T>();
  constexpr auto fldsAnnots = FieldAnnots::MkFldAnnots<T>();
  constexpr auto fields = GetRelFields<T>();
  constexpr auto sz = fldsAnnots.size();
  std::array<int, sz> out{};
  constexpr auto sorted = SortFieldsAlphabetically<T>();
  static_assert(fldsAnnots.size() == fields.size());
  static_assert(fldsAnnots.size() == sorted.size());

  template for (constexpr auto idx : std::views::indices(fields.size()))
  {
    if (out[idx] != 0)
      continue;
    // search for pos num
    template for (constexpr auto fldNum : std::views::indices(fields.size()))
    {
      if constexpr (fldsAnnots[fldNum].m_pos == idx)
      {
        out[idx] = fldNum + 1;
      }
    }

    if (out[idx] > 0)
      continue;

    // there is no relevant position - must find positional element
    // search next positional candidate

    template for (constexpr auto fldIdx : std::views::indices(fields.size()))
    {
      constexpr auto fldNum =
          strAnnots.m_alphabetical.has_value() ? sorted[fldIdx] : fldIdx;
      // the field shoud Not have the relevant position
      if constexpr (fldsAnnots[fldNum].m_pos != -1 &&
                    fldsAnnots[fldNum].m_pos < sz)
      {
        continue;
      }

      // the field must not be in the out index already
      bool ok = true;
      template for (constexpr auto idx2 : std::views::indices(idx))
      {
        if (out[idx2] == fldNum + 1)
        {
          ok = false;
        }
      }
      if (!ok)
        continue;

      if (out[idx] == 0)
        out[idx] = fldNum + 1;
    }
  }

  template for (constexpr auto idx : std::views::indices(fields.size()))
  {
    out[idx] -= 1;
  }

  return out;
}

template <std::meta::info T, char Delim = ',', bool Ignore = false,
          int MinSz = 0, int FxSz = -1>
std::pair<char *, typename[:T:]> ParseBase(char * curr, char const * end)
{
  static_assert(IsBase<T>());
  constexpr int AddLen = MinSz > FxSz ? MinSz : FxSz;
  if constexpr (Ignore)
  {
    char * after = JSONParser::SkipVal<typename[:T:]>(curr, end, Delim, AddLen);
    assert(*after == Delim);
    return {after, typename[:T:]{}};
  }
  else
  {
    if constexpr (T == ^^std::string)
    {
      GET_STR(Out);
      assert(*curr == Delim);
      return {curr, typename[:T:](Out)};
    }
    else
    {
      auto [val, after] =
          JSONParser::ReadNumber<typename[:T:]>(curr, end, Delim);
      assert(*after == Delim);

      return {after, val};
    }
  }
  std::unreachable();
}

template <std::meta::info T> char * ParseJson(char * curr)
{
  assert(curr);
  char * start = curr;

  return curr;
}

} // namespace yjson
