
//========================================================//
// Utils.hpp                                              //
//========================================================//
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <meta>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <contracts>

namespace yjson
{


//--------------------------------------------------------//
// get_annotations                                        //
//--------------------------------------------------------//
//@brief returns the annotation object of given 'ann' type
//within the 'entity' type
//@param ann annotation type to search for
//@param entity an object to find the annotations in
//@return a static range of captured annotation reflections
template <std::meta::info ann, std::meta::info entity> consteval auto get_annotations()
{
  static_assert(std::meta::is_type(ann));
  return std::define_static_array(
      std::meta::annotations_of_with_type(entity, ann));
}

//--------------------------------------------------------//
// GetRelFields                                           //
//--------------------------------------------------------//
// @brief provides the Relevant fields from a struct / class
// @param T class type reflection
// @return range of non static, non-function members of a class
template <std::meta::info T> consteval auto GetRelFields()
{
  static_assert(std::meta::is_class_type(T));

  constexpr auto flds = std::define_static_array(
      std::meta::members_of(T, std::meta::access_context::unchecked()));

  return std::define_static_array(std::views::filter(
      flds, [](auto & v)
      { return std::meta::has_identifier(v) && !std::meta::is_function(v)
        && !std::meta::is_static_member(v); }));
}


//--------------------------------------------------------//
// GetRelFuncs                                            //
//--------------------------------------------------------//
@brief provides the Relevant functions from a struct / class
@param T class type reflection
@return range of function members of T
template <std::meta::info T> consteval auto GetRelFuncs()
{
  constexpr auto flds = std::define_static_array(
      std::meta::members_of(T, std::meta::access_context::unchecked()));

  return std::define_static_array(std::views::filter(
      flds, [](auto & v)
      { return std::meta::has_identifier(v) && std::meta::is_function(v); }));
}

//----------------------//
// Struct annotations   //
//----------------------//

/**
 * @class Alphabetical
 * @brief struct tag for marking the struct beign on alphabetically ordered in JSON
 */
struct Alphabetical
{
  // @brief whether the order is reversed
  bool _Rev;
};


struct NotCompressed
{
};

struct NonStrictOrder
{
};

struct RandomOrder
{
};
//----------------------//
// Field annotations    //
//----------------------//
struct Position
{
  int _Pos;
};

struct Size
{
  int _Sz;
};

struct Ignore
{
};

struct MayAbsent
{
};

struct DisplayName
{
  static constexpr std::size_t kMaxLen = 63;
  char name[kMaxLen + 1]{};

  template <std::size_t N>
    requires(N <= kMaxLen + 1)
  consteval DisplayName(const char (&str)[N])
  {
    std::copy_n(str, N, name);
  }
};

struct MinSize
{
  int _Sz;
};

// Marks a container as holding a fixed, compile-time-known number of elements.
// Triggers the fully-unrolled ParseContainerStatic parser (template-for loop,
// compile-time delimiters, single up-front allocation).
struct StaticSize
{
  int _Sz;
};

struct FieldAnnots
{
  int m_pos = -1;
  int m_sz = -1;
  bool m_ignore = false;
  std::array<char, DisplayName::kMaxLen + 1> m_disp_name{};
  int m_min_sz = -1;
  int m_static_sz = -1;
  bool m_may_absent = false;

  template <std::meta::info fld> static consteval FieldAnnots MkFieldAnnots()
  {
    FieldAnnots ann;

    if constexpr (constexpr auto pos = get_annotations<^^Position, fld>();
                  pos.size() > 0)
    {
      ann.m_pos = std::meta::extract<Position>(pos[0])._Pos;
    }

    if constexpr (constexpr auto sz = get_annotations<^^Size, fld>();
                  sz.size() > 0)
    {
      ann.m_sz = std::meta::extract<Size>(sz[0])._Sz;
    }

    if constexpr (constexpr auto st_sz = get_annotations<^^StaticSize, fld>();
                  st_sz.size() > 0)
    {
      ann.m_static_sz = std::meta::extract<StaticSize>(st_sz[0])._Sz;
    }

    if constexpr (get_annotations<^^Ignore, fld>().size() > 0)
    {
      ann.m_ignore = true;
    }

    if constexpr (get_annotations<^^MayAbsent, fld>().size() > 0)
    {
      ann.m_may_absent = true;
    }

    if constexpr (constexpr auto dn = get_annotations<^^DisplayName, fld>();
                  dn.size() > 0)
    {
      constexpr auto dname = std::meta::extract<DisplayName>(dn[0]);
      std::copy_n(dname.name, DisplayName::kMaxLen + 1,
                  ann.m_disp_name.begin());
    }

    return ann;
  }

  template <std::meta::info T> static consteval auto MkFldAnnots()
  {
    constexpr auto flds = GetRelFields<T>();
    constexpr auto n = std::ranges::size(flds);
    std::array<FieldAnnots, n> out{};

    template for (constexpr auto idx : std::views::indices(n))
    {
      out[idx] = MkFieldAnnots<flds[idx]>();
    }

    return out;
  }
};

struct StructAnnots
{
  std::optional<bool> m_alphabetical = std::nullopt;
  bool m_compressed = true;
  bool m_random_order = false;

  template <std::meta::info T> static consteval StructAnnots MkStrAnnots()
  {
    StructAnnots out;

    if constexpr (constexpr auto alph = get_annotations<^^Alphabetical, T>();
                  alph.size() > 0)
    {
      out.m_alphabetical = std::meta::extract<Alphabetical>(alph[0])._Rev;
    }

    if constexpr (constexpr auto not_compressed =
                      get_annotations<^^NotCompressed, T>();
                  not_compressed.size() > 0)
    {
      out.m_compressed = false;
    }

    if constexpr (constexpr auto random_order =
                      get_annotations<^^RandomOrder, T>();
                  random_order.size() > 0)
    {
      out.m_random_order = true;
    }

    return out;
  }
};

//--------------------------------------------------------//
// Alphabetical field sorting                             //
//--------------------------------------------------------//
// ASCII-only lowercase helper (enough for identifiers / display names):
consteval char to_lower_ascii(char c)
{
  return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

// Case-insensitive lexicographic comparison of two compile-time strings.
// Returns < 0 when @a a < @a b, 0 when equal, > 0 otherwise.
consteval int compare_str_ci(std::string_view a, std::string_view b)
{
  const auto n = std::min(a.size(), b.size());
  for (std::size_t i = 0; i < n; ++i)
  {
    const char ca = to_lower_ascii(a[i]);
    const char cb = to_lower_ascii(b[i]);
    if (ca != cb)
      return ca < cb ? -1 : 1;
  }
  if (a.size() == b.size())
    return 0;
  return a.size() < b.size() ? -1 : 1;
}

// Sort the fields of the annotated struct @a T alphabetically (case
// insensitively) and return the sorted field indices, ie. the returned
// array maps the sorted position to the original member index.
//
// The sort key of a field is, in priority order:
//   1. the DisplayName annotation, if present;
//   2. the member identifier of the field itself.
//
// The Position annotation is deliberately ignored here. The Alphabetical
// struct annotation (StructAnnots::m_alphabetical) is honoured: when its
// `_Rev` flag is true the resulting order is reversed.
template <typename T> consteval auto SortFieldsAlphabetically()
{
  constexpr StructAnnots strAnnots = StructAnnots::MkStrAnnots<^^T>();
  constexpr auto fldAnnots = FieldAnnots::MkFldAnnots<^^T>();
  constexpr auto fields = GetRelFields<^^T>();
  constexpr auto n = fields.size();

  // Build the case-insensitive sort key of each field:
  std::array<std::string_view, n> keys{};
  template for (constexpr auto i : std::views::indices(n))
  {
    if constexpr (fldAnnots[i].m_disp_name[0] != '\0')
      keys[i] = std::string_view{fldAnnots[i].m_disp_name.data()};
    else
      keys[i] = std::meta::identifier_of(fields[i]);
  }

  // Indices to sort (identity permutation):
  std::array<int, n> order{};
  template for (constexpr auto i : std::views::indices(n)) order[i] =
      static_cast<int>(i);

  std::sort(order.begin(), order.end(),
            [&](int a, int b) { return compare_str_ci(keys[a], keys[b]) < 0; });

  if constexpr (strAnnots.m_alphabetical.value_or(false))
    std::reverse(order.begin(), order.end());

  return order;
}

template <std::meta::info T> consteval bool HasFuncWithName(std::string_view s)
{
  constexpr auto funcs = GetRelFuncs<T>();
  return std::ranges::contains(funcs, s, std::meta::identifier_of);
}

template <std::meta::info T, bool top_lvl = true> consteval bool IsBase();
template <std::meta::info T> consteval bool IsOption();

template <std::meta::info T> consteval bool IsContainer()
{
  if constexpr (IsBase<T>())
  {
    return false;
  }
  else if constexpr (std::meta::has_template_arguments(T) &&
                     std::meta::template_of(T) == ^^std::array)
  {
    // Fixed-size containers (std::array) lack push_back/emplace_back, so the
    // dynamic-container probe below would miss them.
    return true;
  }
  else
  {
    constexpr bool raw_array = std::meta::is_array_type(T);
    constexpr bool has_begin = HasFuncWithName<T>("begin");
    constexpr bool has_end = HasFuncWithName<T>("end");
    constexpr bool has_push_b = HasFuncWithName<T>("push_back");
    constexpr bool has_push = HasFuncWithName<T>("push");

    return raw_array || (has_begin && has_end && (has_push_b || has_push));
  }
}

template <std::meta::info T> consteval bool IsVariant()
{
  try
  {
    return std::meta::template_of(T) == ^^std::variant;
  }
  catch (...)
  {
    return false;
  }
}

template <std::meta::info T> consteval bool IsOption()
{
  try
  {
    return std::meta::template_of(T) == ^^std::optional;
  }
  catch (...)
  {
    return false;
  }
}

consteval bool IsBool(std::meta::info T) { return T == ^^bool; }

consteval bool IsTuple(std::meta::info T)
{
  try
  {
    return std::meta::template_of(T) == ^^std::tuple ||
           std::meta::template_of(T) == ^^std::pair;
  }
  catch (...)
  {
    return false;
  }
}

template <std::meta::info T, bool top_lvl> consteval bool IsBase()
{
  if constexpr (IsOption<T>() && top_lvl)
  {
    return IsBase<std::meta::template_arguments_of(T)[0], false>();
  }
  else
  {
    constexpr bool integral = std::meta::is_integral_type(T);
    constexpr bool floating = std::meta::is_floating_point_type(T);
    constexpr bool boolean = IsBool(T);
    constexpr bool stringal = T == ^^std::string;
    constexpr bool string11 = T == ^^std::__cxx11::basic_string<char>;
    constexpr bool stringview = T == ^^std::string_view;
    constexpr bool stringview11 = T == ^^std::basic_string_view<char>;
    return integral || floating || stringal || string11 || stringview ||
           stringview11 || boolean;
  }
}
template <std::meta::info T> consteval bool IsSupported()
{
  return std::is_copy_assignable<typename[:T:]>();
}

} // namespace yjson
