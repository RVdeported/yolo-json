#include "Include/annotations.hpp"
#include "Include/json_parser.hpp"
#include "Include/utils.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <meta>
#include <print>
#include <ranges>
#include <span>
#include <type_traits>
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

template <std::meta::info T, char Delim1 = ',', bool Ignore = false,
          int MinSz = 0, int FxSz = -1, char Delim2 = Delim1>
std::pair<char *, typename[:T:]> ParseBase(char * curr, char const * end)
{
  static_assert(IsBase<T>());
  constexpr int AddLen = MinSz > FxSz ? MinSz : FxSz;
  constexpr bool integral = std::meta::is_integral_type(T);
  constexpr bool floating = std::meta::is_floating_point_type(T);
  
  char Delim = Delim1;
  if constexpr(Delim1 != Delim2)
  {
    char const * from = curr;
    char const * d1 = std::find(from, end, Delim1);
    char const * d2 = std::find(from, end, Delim2);
    Delim = d1 < d2 ? Delim1 : Delim2;
  }

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
      return {curr, typename[:T:](Out)};
    }
  }
  std::unreachable();
}

namespace detail
{

// The mutually recursive object parser. Member functions may call each other
// regardless of declaration order, which avoids redeclaring function templates
// whose return type contains a reflection splice.
struct ObjectParser
{
  template <std::meta::info T>
  static std::pair<char *, typename[:T:]> ParseJson(char * curr, char * end)
  {
    using T_ = typename[:T:];
    T_ out{};
    assert(curr);
    assert(*curr == '{');
    constexpr auto flds = GetRelFields<T_>();
    constexpr auto flds_ord = GetOrderedField<T_>();
    constexpr StructAnnots strAnnots = StructAnnots::MkStrAnnots<T_>();
    constexpr auto sz = flds.size();

    template for (constexpr auto idx : std::views::indices(sz))
    {
      curr++;
      constexpr auto curr_fld = flds[flds_ord[idx]];
      constexpr FieldAnnots curr_ann = FieldAnnots::MkFieldAnnots<curr_fld>();
      
      constexpr char delim = idx + 1 == sz ? '}' : ',';
      auto [after, v] =
          ParseObj<curr_fld, curr_ann.m_sz, curr_ann.m_min_sz,  strAnnots.m_compressed, 
        delim>(curr, end);
      curr = after;

      out.[:curr_fld:] = v;
    }
    if constexpr (!strAnnots.m_compressed)
      SKP_SPC();
    assert(*curr == '}');
    curr++;
    if constexpr (!strAnnots.m_compressed)
      SKP_SPC();
    return {curr, out};
  }

  template<std::meta::info T, bool Compressed = true> 
  static std::pair<char *, typename[:T:]> ParseTuple(char * curr, char * end)
  {
    static_assert(IsTuple(T));
    assert(curr && end);
    assert(*curr == '[');
    constexpr auto types = std::define_static_array(std::meta::template_arguments_of(T));
    constexpr auto sz = types.size(); 
    
    typename[:T:] out;
    template for (constexpr auto idx : std::views::indices(sz))
    {
      constexpr auto tt = types[idx];
      if constexpr(idx > 0)
        assert(*curr == ',');

      curr++;
      if constexpr(!Compressed)
        SKP_SPC();

      auto [after, v] = ParseVal<tt, 0, -1, Compressed, idx == sz - 1 ? ']' : ','>(curr, end);

      std::get<idx>(out) = v;
      curr = after;
    }
    curr++;

    return {curr, out};
  }

  template<std::meta::info T, bool Compressed = true> 
  static std::pair<char *, typename[:T:]> ParseContainer(char * curr, char * end)
  {
    static_assert(IsContainer<T>());
    assert(curr && end);
    assert(*curr == '[');
    constexpr auto tt = std::meta::template_arguments_of(T)[0];
    constexpr bool fixed = std::meta::template_of(T) == ^^std::array;

    typename[:T:] out;

    for (auto idx :
         std::views::indices(fixed ? int(out.size())
                                   : std::numeric_limits<int>::max()))
    {
      assert(idx == 0 || *curr == ',');
      curr++;

      if constexpr(!Compressed)
        SKP_SPC();

      auto [after, v] = ParseVal<tt, 0, -1, Compressed, ',', false, ']'>(curr, end);

      if constexpr (fixed)
        out[idx] = v;
      else
        out.emplace_back(v);
      curr = after;

      if (*curr == ']')
        break;
    }
    curr++;

    return {curr, out};
  }


  // Parse a single field of the object being parsed. `curr` points at the
  // field's key; the returned pointer points at the delimiter following the
  // value (or at the re-wound key when an optional field is absent).
  template <std::meta::info curr_fld, int sz, int min_sz,
            bool compressed, char Delim>
  static std::pair<char *, typename[:std::meta::type_of(curr_fld):]> 
  ParseObj(char * curr, char * end)
  {
    constexpr auto t = std::meta::type_of(curr_fld);
    constexpr FieldAnnots curr_ann = FieldAnnots::MkFieldAnnots<curr_fld>();
    constexpr std::string_view ident = std::meta::identifier_of(curr_fld);
    
    typename [:t:] out;
    std::string_view disp_name =
        std::string_view(curr_ann.m_disp_name.begin());
    std::string_view name =
        curr_ann.m_disp_name[0] == '\0' ? ident : disp_name;

    constexpr auto base_cls = std::meta::has_template_arguments(t)
                                  ? std::meta::template_arguments_of(t)[0]
                                  : t;

    if constexpr (!compressed)
      SKP_SPC();

    if constexpr (curr_ann.m_may_absent)
    {
      static_assert(IsOption<std::meta::type_of(curr_fld)>());
      if (end - curr < name.size() + 2)
        return {curr, out};
      char * old = curr;
      SKP_IF_SV_G(name);
      // field is absent
      if (curr == old)
      {
        out = std::nullopt;
        --curr;
        return {curr, out};
      }
    }
    else
    {
      if constexpr (!compressed)
        SKP_SPC();

      assert(*curr == '"');
      SKP_STR_SV(name);
    }
    if constexpr (!compressed)
      SKP_SPC();

    assert(*curr == ':');
    curr++;

    if constexpr (!compressed)
      SKP_SPC();
    
    auto [after, v] = ParseVal<t, sz, min_sz, compressed, Delim, curr_ann.m_ignore>(curr, end);

    return {after, v};
  }

  template <std::meta::info T, int sz, int min_sz,
            bool compressed, char Delim1, bool ignore = false, char Delim2 = Delim1>
  static std::pair<char *, typename[:T:]> ParseVal(char * curr,
                                                          char * end)
  {
    typename [:T:] out;
    constexpr auto base_cls = std::meta::has_template_arguments(T)
                                  ? std::meta::template_arguments_of(T)[0]
                                  : T;


    if constexpr (IsOption<T>())
    {
      if (*curr == 'n')
      {
        SKP_STR("null")
        out = std::nullopt;
        if constexpr (!compressed)
          SKP_SPC();
        return {curr, out};
      }
    }

    if constexpr (IsBool(base_cls))
    {
      if (*curr == 't')
      {
        SKP_STR("true")
        out = true;
      }
      else
      {
        SKP_STR("false")
        out = false;
      }
      if constexpr (!compressed)
        SKP_SPC();
    }
    else if constexpr (IsTuple(T) || IsTuple(base_cls))
    {
      constexpr auto _T = IsTuple(T) ? T : base_cls;
      auto [after, v] = ParseTuple<_T, compressed>(curr, end);
      curr = after;
      out = v;

      if constexpr (!compressed)
        SKP_SPC();
    }
    else if constexpr (IsBase<T>())
    {
      auto [after, v] =
          ParseBase<base_cls == ^^char ? T : base_cls, Delim1,
                    ignore, min_sz, sz, Delim2>(
              curr, end);
      curr = after;
      out = v;

      if constexpr (!compressed)
        SKP_SPC();
    }
    else if constexpr (IsContainer<T>())
    {
      constexpr auto _T = IsContainer<T>() ? T : base_cls;
      auto [after, v] = ParseContainer<_T, compressed>(curr, end);
      curr = after;
      out = v;
    }
    // should be another object then
    else
    {
      assert(*curr == '{');
      if constexpr (ignore && sz > 0)
      {
        curr += sz;
      }
      else
      {
        auto [after, v] = ParseJson<base_cls>(curr, end);
        curr = after;
        out = v;
      }
    }
    return {curr, out};
  }
};

} // namespace detail

template <std::meta::info T>
std::pair<char *, typename[:T:]> ParseJson(char * curr, char * end)
{
  return detail::ObjectParser::ParseJson<T>(curr, end);
}




} // namespace yjson
