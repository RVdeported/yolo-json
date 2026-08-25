
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

namespace yjson
{

// Base case: types are different
template <typename T, typename U> struct is_same_template : std::false_type
{
};

// Specialization: matching template template parameters
template <template <typename...> class Template, typename... Args1,
          typename... Args2>
struct is_same_template<Template<Args1...>, Template<Args2...>> : std::true_type
{
};

// Helper variable template for ease of use
template <typename T, typename U>
inline constexpr bool is_same_template_v = is_same_template<T, U>::value;

consteval bool compare_naked_templates(std::meta::info typeA,
                                       std::meta::info typeB)
{
  bool has_args_a = std::meta::has_template_arguments(typeA);
  bool has_args_b = std::meta::has_template_arguments(typeB);

  auto base_a = has_args_a ? std::meta::template_of(typeA) : typeA;
  auto base_b = has_args_b ? std::meta::template_of(typeB) : typeB;

  return base_a == base_b;
}

template <typename T>
consteval auto has_annotation(std::meta::info r, T const & value) -> bool
{
  return std::ranges::contains(annotations_of_with_type(r, ^^T),
                               std::meta::reflect_constant(value),
                               std::meta::constant_of);
}

template <typename T, typename U> consteval auto get_annotations()
{
  constexpr auto ann =
      std::define_static_array(std::meta::annotations_of_with_type(^^U, ^^T));
  return ann;
}

template <typename T, std::meta::info entity> consteval auto get_annotations()
{
  return std::define_static_array(
      std::meta::annotations_of_with_type(entity, ^^T));
}

template <typename T> consteval auto GetRelFields()
{
  constexpr auto flds = std::define_static_array(
      std::meta::members_of(^^T, std::meta::access_context::unchecked()));

  return std::define_static_array(std::views::filter(
      flds, [](auto & v)
      { return std::meta::has_identifier(v) && !std::meta::is_function(v); }));
}

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
struct Alphabetical
{
  bool _Rev;
};

struct NotCompressed
{
};

struct NonStrictOrder
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

struct FieldAnnots
{
  int m_pos = -1;
  int m_sz = -1;
  bool m_ignore = false;
  std::array<char, DisplayName::kMaxLen + 1> m_disp_name{};
  int m_min_sz = -1;
  bool m_may_absent = false;

  template <std::meta::info fld> static consteval FieldAnnots MkFieldAnnots()
  {
    FieldAnnots ann;

    if constexpr (constexpr auto pos = get_annotations<Position, fld>();
                  pos.size() > 0)
    {
      ann.m_pos = std::meta::extract<Position>(pos[0])._Pos;
    }

    if constexpr (constexpr auto sz = get_annotations<Size, fld>();
                  sz.size() > 0)
    {
      ann.m_sz = std::meta::extract<Size>(sz[0])._Sz;
    }

    if constexpr (get_annotations<Ignore, fld>().size() > 0)
    {
      ann.m_ignore = true;
    }

    if constexpr (get_annotations<MayAbsent, fld>().size() > 0)
    {
      ann.m_may_absent = true;
    }

    if constexpr (constexpr auto dn = get_annotations<DisplayName, fld>();
                  dn.size() > 0)
    {
      constexpr auto dname = std::meta::extract<DisplayName>(dn[0]);
      std::copy_n(dname.name, DisplayName::kMaxLen + 1,
                  ann.m_disp_name.begin());
    }

    return ann;
  }

  template <typename T> static consteval auto MkFldAnnots()
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

  template <typename T> static consteval StructAnnots MkStrAnnots()
  {
    StructAnnots out;

    if constexpr (constexpr auto alph = get_annotations<Alphabetical, T>();
                  alph.size() > 0)
    {
      out.m_alphabetical = std::meta::extract<Alphabetical>(alph[0])._Rev;
    }

    if constexpr (constexpr auto not_compressed =
                      get_annotations<NotCompressed, T>();
                  not_compressed.size() > 0)
    {
      out.m_compressed = false;
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
  constexpr StructAnnots strAnnots = StructAnnots::MkStrAnnots<T>();
  constexpr auto fldAnnots = FieldAnnots::MkFldAnnots<T>();
  constexpr auto fields = GetRelFields<T>();
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
  constexpr auto funcs = GetRelFuncs<T>();
  constexpr bool is_base = IsBase<T>();
  constexpr bool has_begin = HasFuncWithName<T>("begin");
  constexpr bool has_end = HasFuncWithName<T>("end");
  constexpr bool has_push_b = HasFuncWithName<T>("push_back");
  constexpr bool has_push = HasFuncWithName<T>("push");

  return !is_base && has_begin && has_end && (has_push_b | has_push);
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
    constexpr bool stringal = T == ^^std::string;
    return integral || floating || stringal;
  }
}
} // namespace yjson
