
//========================================================//
// Utils.hpp                                              //
//========================================================//
#pragma once
#include <algorithm>
#include <array>
#include <contracts>
#include <cstddef>
#include <iostream>
#include <meta>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

//! Generic record interfaces and implementations
namespace yjson
{

//--------------------------------------------------------//
// get_annotations                                        //
//--------------------------------------------------------//
/**
 * @brief Returns the annotation object of the given type within an entity.
 * @tparam ann annotation type to search for
 * @tparam entity an object to find the annotations in
 * @return A static range of captured annotation reflections
 */
template <std::meta::info ann, std::meta::info entity>
consteval auto get_annotations()
{
  static_assert(std::meta::is_type(ann));
  return std::define_static_array(
      std::meta::annotations_of_with_type(entity, ann));
}

//--------------------------------------------------------//
// GetRelFields                                           //
//--------------------------------------------------------//
/**
 * @brief provides the Relevant fields from a struct / class
 * @tparam T class type reflection
 * @return range of non static, non-function members of a class
 */
template <std::meta::info T> consteval auto GetRelFields()
{
  static_assert(std::meta::is_class_type(T));

  constexpr auto flds = std::define_static_array(
      std::meta::members_of(T, std::meta::access_context::unchecked()));

  return std::define_static_array(std::views::filter(
      flds,
      [](auto & v)
      {
        return std::meta::has_identifier(v) && !std::meta::is_function(v) &&
               !std::meta::is_static_member(v);
      }));
}

//--------------------------------------------------------//
// GetRelFuncs                                            //
//--------------------------------------------------------//
/**
 * @brief provides the Relevant functions from a struct / class
 * @tparam T class type reflection
 * @return range of function members of T
 */
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
 * @brief Struct tag for marking the struct beign on alphabetically ordered in
 * JSON
 */
struct Alphabetical
{
  /**
   * @brief whether the order is reversed
   */
  bool _Rev;
};

/**
 * @class NotCompressed
 * @brief Struct tag to establish the JSON might have garbage
 * simbols like new lines, tabs or spaces
 */
struct NotCompressed
{
};

/**
 * @class RandomOrder
 * @brief Struct tag to mark the JSON do not have determenistic
 * order
 */
struct RandomOrder
{
};
//----------------------//
// Field annotations    //
//----------------------//
/**
 * @class Position
 * @brief Marks position of the field on JSON
 * order
 */
struct Position
{
  /**
   * @brief whether the order is reversed
   */
  int _Pos;
};

/**
 * @class Size
 * @brief Marks the fixed byte size of the field's value in the JSON.
 *
 * Used to skip a value of known length without parsing it.
 */
struct Size
{
  /** @brief Fixed byte size of the field's value in the JSON. */
  int _Sz;
};

/**
 * @class Ignore
 * @brief Field tag marking the field's value to be skipped during parsing.
 */
struct Ignore
{
};

/**
 * @class MayAbsent
 * @brief Field tag allowing the field to be absent from the JSON object.
 *
 * Used together with std::optional: when the key never appears the optional
 * is left empty instead of triggering a parse error.
 */
struct MayAbsent
{
};

/**
 * @class DisplayName
 * @brief Field tag giving the field an alternative JSON key name.
 *
 * The compile-time string is stored in a fixed-capacity buffer; @c kMaxLen
 * is the maximum supported name length.
 */
struct DisplayName
{
  /** @brief Maximum length of a display name (excluding the NUL terminator). */
  static constexpr std::size_t kMaxLen = 63;
  /** @brief NUL-terminated name buffer. */
  char name[kMaxLen + 1]{};

  /**
   * @brief Copy-constructs the name from a compile-time string literal.
   * @tparam N length of @a str including the NUL terminator
   * @param str the string literal to copy
   */
  template <std::size_t N>
    requires(N <= kMaxLen + 1)
  consteval DisplayName(const char (&str)[N])
  {
    std::copy_n(str, N, name);
  }
};

/**
 * @class MinSize
 * @brief Marks the minimum number of characters the field's value is
 * guaranteed to occupy.
 */
struct MinSize
{
  /** @brief Minimum number of characters the field's value occupies. */
  int _Sz;
};

/**
 * @class StaticSize
 * @brief Marks a container as holding a fixed, compile-time-known number of
 * elements.
 *
 * Triggers the fully-unrolled ParseContainerStatic parser (template-for loop,
 * compile-time delimiters, single up-front allocation).
 */
struct StaticSize
{
  /** @brief Exact number of elements the container holds. */
  int _Sz;
};

/**
 * @class FieldAnnots
 * @brief Resolved per-field annotations, computed once at compile time.
 *
 * Aggregates the Position, Size, StaticSize, Ignore, MayAbsent and DisplayName
 * annotations of a single field into plain data members.
 */
struct FieldAnnots
{
  /** @brief Resolved Position (-1 when unset). */
  int m_pos = -1;
  /** @brief Resolved Size (-1 when unset). */
  int m_sz = -1;
  /** @brief Whether the field carries the Ignore tag. */
  bool m_ignore = false;
  /** @brief Resolved DisplayName (empty when unset). */
  std::array<char, DisplayName::kMaxLen + 1> m_disp_name{};
  /** @brief Resolved MinSize (-1 when unset). */
  int m_min_sz = -1;
  /** @brief Resolved StaticSize (-1 when unset). */
  int m_static_sz = -1;
  /** @brief Whether the field carries the MayAbsent tag. */
  bool m_may_absent = false;

  /**
   * @brief Resolves the annotations of a single reflected field.
   * @tparam fld reflection of the field to inspect
   * @return the resolved FieldAnnots
   */
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

  /**
   * @brief Resolves the annotations of every field of the struct @a T.
   * @tparam T reflected struct type to inspect
   * @return an @c std::array of resolved FieldAnnots (one per field)
   */
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

/**
 * @class StructAnnots
 * @brief Resolved struct-level annotations, computed once at compile time.
 *
 * Aggregates the Alphabetical, NotCompressed and RandomOrder annotations of a
 * struct into plain data members.
 */
struct StructAnnots
{
  /** @brief Alphabetical order flag (nullopt when unset); true = reversed. */
  std::optional<bool> m_alphabetical = std::nullopt;
  /** @brief Whether the JSON is compressed (no stray whitespace). */
  bool m_compressed = true;
  /** @brief Whether JSON fields may appear in arbitrary order. */
  bool m_random_order = false;

  /**
   * @brief Resolves the struct-level annotations of the type @a T.
   * @tparam T reflected struct type to inspect
   * @return the resolved StructAnnots
   */
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
/**
 * @brief ASCII-only lowercase helper (enough for identifiers / display names).
 * @param c character to lower-case
 * @return the lower-cased ASCII character (@p c unchanged when not A-Z)
 */
consteval char to_lower_ascii(char c)
{
  return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

/**
 * @brief Case-insensitive lexicographic comparison of two compile-time strings.
 *
 * @param a left string to compare
 * @param b right string to compare
 * @return < 0 when @a a < @a b, 0 when equal, > 0 otherwise
 */
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

/**
 * @brief Sorts the fields of the annotated struct @a T alphabetically (case
 *        insensitively) and returns the sorted field indices.
 *
 * The returned array maps the sorted position to the original member index.
 *
 * The sort key of a field is, in priority order:
 *   1. the DisplayName annotation, if present;
 *   2. the member identifier of the field itself.
 *
 * The Position annotation is deliberately ignored here. The Alphabetical
 * struct annotation (StructAnnots::m_alphabetical) is honoured: when its
 * `_Rev` flag is true the resulting order is reversed.
 *
 * @tparam T the struct type to sort
 * @return an @c std::array of sorted member indices
 */
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

/**
 * @brief Checks whether the type reflected by @a T has a member function named
 *        @a s.
 *
 * @tparam T reflected type to inspect
 * @param s member function name to search for
 * @return true when a member function named @a s exists
 */
template <std::meta::info T> consteval bool HasFuncWithName(std::string_view s)
{
  constexpr auto funcs = GetRelFuncs<T>();
  return std::ranges::contains(funcs, s, std::meta::identifier_of);
}

template <std::meta::info T, bool top_lvl = true> consteval bool IsBase();
template <std::meta::info T> consteval bool IsOption();

/**
 * @brief Classifies the type reflected by @a T as a dynamic or fixed-size
 *        container.
 *
 * A type is a container when it is a raw array, a std::array, or provides
 * begin()/end() together with push_back()/push().
 *
 * @tparam T reflected type to inspect
 * @return true when @a T is a container
 */
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

/**
 * @brief Classifies the type reflected by @a T as a std::variant.
 *
 * @tparam T reflected type to inspect
 * @return true when @a T is a std::variant
 */
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

/**
 * @brief Classifies the type reflected by @a T as a std::optional.
 *
 * @tparam T reflected type to inspect
 * @return true when @a T is a std::optional
 */
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

/**
 * @brief Checks whether the type reflected by @a T is @c bool.
 *
 * @param T reflected type to inspect
 * @return true when @a T is @c bool
 */
consteval bool IsBool(std::meta::info T) { return T == ^^bool; }

/**
 * @brief Classifies the type reflected by @a T as a std::tuple or std::pair.
 *
 * @param T reflected type to inspect
 * @return true when @a T is a std::tuple or std::pair
 */
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

/**
 * @brief Classifies the type reflected by @a T as a base (scalar) value.
 *
 * A base type is integral, floating-point, boolean, std::string,
 * std::string_view, or a std::optional of such a type when @a top_lvl is set.
 *
 * @tparam T reflected type to inspect
 * @tparam top_lvl when true, unwraps a top-level std::optional
 * @return true when @a T is a base value
 */
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
/**
 * @brief Checks whether the type reflected by @a T is copy-assignable (and
 *        therefore supported by the parser/serializer).
 *
 * @tparam T reflected type to inspect
 * @return true when @a T is copy-assignable
 */
template <std::meta::info T> consteval bool IsSupported()
{
  return std::is_copy_assignable<typename[:T:]>();
}

} // namespace yjson
