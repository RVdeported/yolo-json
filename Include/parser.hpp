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
  constexpr bool integral = std::meta::is_integral_type(T);
  constexpr bool floating = std::meta::is_floating_point_type(T);

  if constexpr (Ignore)
  {
    char * after = JSONParser::SkipVal<typename[:T:]>(curr, end, Delim, AddLen);
    assert(*after == Delim);
    return {after, typename[:T:]{}};
  }
  else
  {
    if constexpr (integral || floating)
    {
      auto [val, after] =
          JSONParser::ReadNumber<typename[:T:]>(curr, end, Delim, AddLen);
      assert(*after == Delim);

      return {after, val};
    }
    else
    {
      GET_STR(Out);
      assert(*curr == Delim);
      return {curr, typename[:T:](Out)};
    }
  }
  std::unreachable();
}
template <std::meta::info T>
std::pair<char *, typename[:T:]> ParseJson(char * curr, char const * end)
{
  using T_ = typename[:T:];
  T_ out{};
  assert(curr);
  char * start = curr;
  assert(*curr == '{');
  constexpr auto flds = GetRelFields<T_>();
  constexpr auto flds_ord = GetOrderedField<T_>();
  constexpr StructAnnots strAnnots = StructAnnots::MkStrAnnots<T_>();
  constexpr auto fieldAnnots = FieldAnnots::MkFldAnnots<T_>();
  constexpr auto sz = flds.size();

  template for (constexpr auto idx : std::views::indices(sz))
  {
    curr++;
    constexpr auto curr_fld = flds[flds_ord[idx]];
    constexpr auto curr_ann = fieldAnnots[flds_ord[idx]];
    constexpr std::string_view ident = std::meta::identifier_of(curr_fld);
    constexpr auto name =
        curr_ann.m_disp_name[0] == '\0' ? ident : curr_ann.m_disp_name.data();
    constexpr auto t = std::meta::type_of(curr_fld);
    constexpr auto base_cls = std::meta::has_template_arguments(t)
                                  ? std::meta::template_arguments_of(t)[0]
                                  : t;
    
    std::cout << curr << '\n';
    if constexpr (!strAnnots.m_compressed)
      SKP_SPC();

    if constexpr (curr_ann.m_may_absent)
    {
      static_assert(IsOption<std::meta::type_of(curr_fld)>());
      if (end - curr < name.size() + 2)
        continue;
      char * old = curr;
      SKP_IF_SV_G(name);
      std::cout << curr << '\n';
      // field is absent
      if (curr == old)
      {
        out.[:curr_fld:] = std::nullopt;
        curr--;
        continue;
      }
    }
    else
    {
      if constexpr (!strAnnots.m_compressed)
        SKP_SPC();

      assert(*curr == '"');
      SKP_STR_SV(name);
    }
    if constexpr (!strAnnots.m_compressed)
      SKP_SPC();

    assert(*curr == ':');
    curr++;

    if constexpr (!strAnnots.m_compressed)
      SKP_SPC();
  
    if constexpr (IsOption<t>())
    {
      if (*curr == 'n')
      {
        SKP_STR("null") 
        out.[:curr_fld:] = std::nullopt;
        if constexpr (!strAnnots.m_compressed)
          SKP_SPC();
        continue;
      }
    }

    if constexpr (IsBool(base_cls))
    {
      if (*curr == 't')
      {
        SKP_STR("true")
        out.[:curr_fld:] = true; 
      }
      else
      {
        SKP_STR("false")
        out.[:curr_fld:] = false; 
      }
      if constexpr (!strAnnots.m_compressed)
        SKP_SPC();
    }
    else
    if constexpr (IsBase<t>())
    {
      constexpr char Delim = idx == sz - 1 ? '}' : ',';
      auto [after, v] =
          ParseBase<base_cls == ^^char ? t : base_cls, Delim, curr_ann.m_ignore,
                    curr_ann.m_min_sz, curr_ann.m_sz>(curr, end);
      curr = after;
      out.[:curr_fld:] = v;
    }
    else if constexpr (IsContainer<t>())
    {
      // not implemented
      static_assert(false);
    }
    // should be aonother object then
    else
    {
      assert(*curr == '{');
      if constexpr(curr_ann.m_ignore && curr_ann.m_sz > 0)
      {
        curr += curr_ann.m_sz;
      }
      else
      {
        auto [after, v] = ParseJson<base_cls>(curr, end);
        curr = after;
        out.[:curr_fld:] = v;
      }
    }
  }
  if constexpr (!strAnnots.m_compressed)
    SKP_SPC();
  assert(*curr == '}');
  curr++;
  if constexpr (!strAnnots.m_compressed)
    SKP_SPC();
  return {curr, out};
}

} // namespace yjson
