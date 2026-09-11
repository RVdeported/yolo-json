//========================================================//
// Parser.hpp                                             //
//========================================================//
#pragma once

#include "json_parser.hpp"
#include "utils.hpp"
#include <cassert>
#include <cstring>
#include <memory>
#include <meta>

//! Reflection-driven JSON parser for annotated structs.
namespace yjson
{
//--------------------------------------------------------//
// GetOrderedField                                        //
//--------------------------------------------------------//
/**
 * @brief Computes the parse order of the fields of the annotated struct @a T.
 *
 * Builds a field index permutation describing the order in which the JSON
 * object's values are expected to appear. Fields carrying an explicit
 * Position annotation are pinned to that slot; the remaining fields are
 * placed in the remaining slots in declaration order (or alphabetically when
 * the Alphabetical struct annotation is present).
 *
 * @tparam T the struct type to order
 * @return an @c std::array mapping each parse position to the corresponding
 *         member index
 */
template <class T> consteval auto GetOrderedField()
{
  //--------------------------------------------------------//
  // Collect annotations of the struct                      //
  //--------------------------------------------------------//
  constexpr StructAnnots strAnnots = StructAnnots::MkStrAnnots<^^T>();
  constexpr auto fldsAnnots = FieldAnnots::MkFldAnnots<^^T>();
  constexpr auto fields = GetRelFields<^^T>();
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

//--------------------------------------------------------//
// ParseBase                                              //
//--------------------------------------------------------//
/**
 * @brief Parses a single base (scalar) value of type @a T from the JSON
 *        buffer.
 *
 * Handles integral, floating-point and string values. When Ignore is set the
 * value is skipped rather than materialized. A two-delimiter mode
 * (Delim1 != Delim2) supports parsing up to a known closing delimiter.
 *
 * @tparam T reflected type of the base value
 * @tparam Delim1 primary delimiter following the value
 * @tparam Ignore when true, skip the value instead of reading it
 * @tparam MinSz minimum number of characters the value is guaranteed to occupy
 * @tparam FxSz fixed length of the value when known (-1 otherwise)
 * @tparam Delim2 secondary (closing) delimiter, defaulting to Delim1
 * @param curr pointer to the first character of the value
 * @param end one-past-the-end pointer of the input buffer
 * @return a pair of the pointer just past the value and the parsed value
 */
template <std::meta::info T, char Delim1 = ',', bool Ignore = false,
          int MinSz = 0, int FxSz = -1, char Delim2 = Delim1>
std::pair<char *, typename[:T:]> ParseBase(char * curr, char * end)
{
  // std::cout << curr << '\n';
  constexpr int AddLen = MinSz > FxSz ? MinSz : FxSz;
  constexpr bool integral = std::meta::is_integral_type(T);
  constexpr bool floating = std::meta::is_floating_point_type(T);
  constexpr bool str = !integral && !floating;
  constexpr bool precEnd = Delim1 != Delim2 && !str;

  char Delim = Delim1;
  if constexpr (precEnd)
  {
    char * tmp = curr;
    do
    {
      tmp++;
    } while (*tmp != Delim1 && *tmp != Delim2);
    Delim = *tmp;
    end = tmp;
  }

  if constexpr (Ignore)
  {
    char * after;
    if constexpr (precEnd)
    {
      after = JSONParser::SkipVal<typename[:T:]>(curr, end, AddLen);
    }
    else
    {
      after = JSONParser::SkipVal<typename[:T:]>(curr, Delim, AddLen);
    }
    assert(*after == Delim);
    return {after, typename[:T:]{}};
  }
  else
  {
    if constexpr (integral || floating)
    {
      char * after;
      typename[:T:] val;
      if constexpr (precEnd)
      {
        std::tie(val, after) =
            JSONParser::ReadNumber<typename[:T:]>(curr, end, AddLen);
      }
      else
      {
        std::tie(val, after) =
            JSONParser::ReadNumber<typename[:T:]>(curr, Delim, AddLen);
      }

      assert(*after == Delim);
      return {after, val};
    }
    else
    {
      GET_STR(Out);
      // GET_STR NUL-terminates the field in place; `curr` now points one past
      // the terminator, so the string length is `curr - Out - 1`. Building from
      // (ptr, len) avoids a second strlen scan and, for std::string_view, copies
      // nothing at all (the view just references the input buffer).
      const std::size_t len = static_cast<std::size_t>(curr - Out - 1);
      return {curr, typename[:T:](Out, len)};
    }
  }
  std::unreachable();
}

//! Implementation details of the reflection-driven JSON parser.
namespace detail
{
//--------------------------------------------------------//
// ObjectParser                                           //
//--------------------------------------------------------//
/**
 * @brief The mutually recursive object parser.
 *
 * Member functions may call each other regardless of declaration order, which
 * avoids redeclaring function templates whose return type contains a
 * reflection splice.
 */
struct ObjectParser
{
  /**
   * @brief Parses a JSON object into a value of the annotated struct @a T.
   *
   * @tparam T reflected type of the object to parse
   * @param curr pointer to the opening '{' of the object
   * @param end one-past-the-end pointer of the input buffer
   * @return a pair of the pointer just past the object and the parsed value
   */
  template <std::meta::info T>
  static std::pair<char *, typename[:T:]> ParseJson(char * curr, char * end)
  {
    using T_ = typename[:T:];
    constexpr StructAnnots strAnnots = StructAnnots::MkStrAnnots<T>();

    if constexpr (strAnnots.m_random_order)
    {
      return ParseJsonRandomOrder<T>(curr, end);
    }
    else
    {
      T_ out{};
      assert(curr);
      assert(*curr == '{');
      constexpr auto flds = GetRelFields<T>();
      constexpr auto flds_ord = GetOrderedField<T_>();
      constexpr auto sz = flds.size();

      template for (constexpr auto idx : std::views::indices(sz))
      {
        curr++;
        constexpr auto curr_fld = flds[flds_ord[idx]];
        constexpr FieldAnnots curr_ann = FieldAnnots::MkFieldAnnots<curr_fld>();

        constexpr char delim = idx + 1 == sz ? '}' : ',';
        auto [after, v] = ParseObj<curr_fld, curr_ann.m_sz, curr_ann.m_min_sz,
                                   strAnnots.m_compressed, delim>(curr, end);
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
  }

  /**
   * @brief Parses a JSON object whose fields may appear in any order.
   *
   * Every key in the JSON is matched against the not-yet-parsed fields of the
   * struct (by identifier or DisplayName); MayAbsent fields simply stay absent
   * when their key never appears.
   *
   * @tparam T reflected type of the object to parse
   * @param curr pointer to the opening '{' of the object
   * @param end one-past-the-end pointer of the input buffer
   * @return a pair of the pointer just past the object and the parsed value
   */
  template <std::meta::info T>
  static std::pair<char *, typename[:T:]> ParseJsonRandomOrder(char * curr,
                                                               char * end)
  {
    using T_ = typename[:T:];
    T_ out{};
    assert(curr);
    assert(*curr == '{');
    constexpr auto flds = GetRelFields<T>();
    constexpr auto flds_ord = GetOrderedField<T_>();
    constexpr auto sz = flds.size();
    constexpr StructAnnots strAnnots = StructAnnots::MkStrAnnots<T>();
    constexpr auto fldAnnots = FieldAnnots::MkFldAnnots<T>();

    std::array<bool, sz> parsed{};
    curr++;

    while (true)
    {
      if constexpr (!strAnnots.m_compressed)
        SKP_SPC();

      if (*curr == '}')
      {
        curr++;
        break;
      }

      bool matched = false;
      template for (constexpr auto i : std::views::indices(sz))
      {
        if (!matched && !parsed[i])
        {
          constexpr auto curr_fld = flds[flds_ord[i]];
          constexpr FieldAnnots curr_ann =
              FieldAnnots::MkFieldAnnots<curr_fld>();
          constexpr std::string_view ident = std::meta::identifier_of(curr_fld);
          std::string_view disp_name =
              std::string_view(curr_ann.m_disp_name.begin());
          std::string_view name =
              curr_ann.m_disp_name[0] == '\0' ? ident : disp_name;

          char * after = curr;
          SKP_IF_SV_G(name);
          if (after == curr)
            continue;

          matched = true;
          parsed[i] = true;

          if constexpr (!strAnnots.m_compressed)
            SKP_SPC();
          assert(*curr == ':');
          curr++;
          if constexpr (!strAnnots.m_compressed)
            SKP_SPC();

          constexpr auto t = std::meta::type_of(curr_fld);
          std::tie(curr, out.[:curr_fld:]) =
              ParseVal<t, curr_ann.m_sz, curr_ann.m_min_sz,
                       curr_ann.m_static_sz, strAnnots.m_compressed, ',',
                       curr_ann.m_ignore, '}'>(curr, end);
        }
      }
      assert(matched);

      if (*curr == ',')
        curr++;
      else
        assert(*curr == '}');
    }

    template for (constexpr auto i : std::views::indices(sz))
    {
      if constexpr (!fldAnnots[i].m_may_absent)
        assert(parsed[i]);
    }

    if constexpr (!strAnnots.m_compressed)
      SKP_SPC();
    return {curr, out};
  }

  /**
   * @brief Parses a JSON array into a std::tuple / std::pair value.
   *
   * @tparam T reflected tuple/pair type
   * @tparam Compressed when false, whitespace is tolerated between tokens
   * @param curr pointer to the opening '[' of the array
   * @param end one-past-the-end pointer of the input buffer
   * @return a pair of the pointer just past the array and the parsed value
   */
  template <std::meta::info T, bool Compressed = true>
  static std::pair<char *, typename[:T:]> ParseTuple(char * curr, char * end)
  {
    static_assert(IsTuple(T));
    assert(curr && end);
    assert(*curr == '[');
    constexpr auto types =
        std::define_static_array(std::meta::template_arguments_of(T));
    constexpr auto sz = types.size();

    typename[:T:] out;
    template for (constexpr auto idx : std::views::indices(sz))
    {
      constexpr auto tt = types[idx];
      if constexpr (idx > 0)
        assert(*curr == ',');

      curr++;
      if constexpr (!Compressed)
        SKP_SPC();

      auto [after, v] =
          ParseVal<tt, 0, -1, -1, Compressed, idx == sz - 1 ? ']' : ','>(curr,
                                                                         end);

      std::get<idx>(out) = v;
      curr = after;
    }
    curr++;

    return {curr, out};
  }

  //--------------------------------------------------------//
  // ParseContainer                                         //
  //--------------------------------------------------------//
  /**
   * @brief Parses a JSON array into a dynamic or fixed-size container.
   *
   * Supports any container detected by IsContainer (std::array, std::vector,
   * std::deque, raw arrays, ...), growing dynamic containers element by
   * element.
   *
   * @tparam T reflected container type
   * @tparam Compressed when false, whitespace is tolerated between tokens
   * @param curr pointer to the opening '[' of the array
   * @param end one-past-the-end pointer of the input buffer
   * @return a pair of the pointer just past the array and the parsed value
   */
  template <std::meta::info T, bool Compressed = true>
  static std::pair<char *, typename[:T:]> ParseContainer(char * curr,
                                                         char * end)
  {
    static_assert(IsContainer<T>());
    assert(curr && end);
    assert(*curr == '[');
    constexpr auto tt = std::meta::template_arguments_of(T)[0];
    constexpr bool fixed = std::meta::template_of(T) == ^^std::array;

    typename[:T:] out;

    // Empty container: skip '[' and any surrounding whitespace (NotCompressed
    // emits spaces inside an empty container), then expect ']'.
    {
      char * probe = curr + 1;
      if constexpr (!Compressed)
        while (isspace(*probe) || *probe == '\n' || *probe == '\t')
          ++probe;
      if (*probe == ']')
        return {probe + 1, out};
    }

    for (auto idx : std::views::indices(
             fixed ? int(out.size()) : std::numeric_limits<int>::max()))
    {
      assert(idx == 0 || *curr == ',');
      curr++;

      if constexpr (!Compressed)
        SKP_SPC();

      auto [after, v] =
          ParseVal<tt, 0, -1, -1, Compressed, ',', false, ']'>(curr, end);

      if constexpr (fixed)
        out[idx] = v;
      else
        out.emplace_back(v);
      curr = after;
      if constexpr (!Compressed)
        SKP_SPC();

      if (*curr == ']')
        break;
    }
    curr++;

    return {curr, out};
  }

  //--------------------------------------------------------//
  // ParseContainerStatic (fixed element count)             //
  //--------------------------------------------------------//
  /**
   * @brief Parses a container holding exactly @a N elements (StaticSize).
   *
   * The element count is known at compile time (via the StaticSize
   * annotation), so the loop is fully unrolled (`template for`), the delimiter
   * after every element is a compile-time constant, and dynamic containers
   * allocate their final storage once up front — no per-element growth checks,
   * no delimiter probing and no runtime break test remain.
   *
   * @tparam T reflected container type
   * @tparam N the exact number of elements to parse
   * @tparam Compressed when false, whitespace is tolerated between tokens
   * @param curr pointer to the opening '[' of the array
   * @param end one-past-the-end pointer of the input buffer
   * @return a pair of the pointer just past the array and the parsed value
   */
  template <std::meta::info T, int N, bool Compressed = true>
  static std::pair<char *, typename[:T:]> ParseContainerStatic(char * curr,
                                                               char * end)
  {
    static_assert(IsContainer<T>());
    static_assert(N >= 0);
    assert(curr && end);
    assert(*curr == '[');
    constexpr auto tt = std::meta::template_arguments_of(T)[0];
    constexpr bool fixed = std::meta::template_of(T) == ^^std::array;

    if constexpr (fixed)
      static_assert(N == static_cast<int>(std::tuple_size_v<typename[:T:]>),
                    "StaticSize must match the std::array element count");

    typename[:T:] out;

    if constexpr (N == 0)
    {
      // Empty container: consume "[]" (whitespace tolerant when !Compressed).
      char * probe = curr + 1;
      if constexpr (!Compressed)
        while (isspace(*probe) || *probe == '\n' || *probe == '\t')
          ++probe;
      assert(*probe == ']');
      return {probe + 1, out};
    }

    // Dynamic containers: size their storage once up front so element parsing
    // never reallocates. std::deque lacks reserve(), hence the requires-guard.
    if constexpr (!fixed)
    {
      if constexpr (requires { out.reserve(N); })
        out.reserve(N);
    }

    template for (constexpr auto idx : std::views::indices(N))
    {
      if constexpr (idx > 0)
        assert(*curr == ',');
      curr++; // skip '[' (first element) or ',' (subsequent elements)

      if constexpr (!Compressed)
        SKP_SPC();

      // The delimiter after this element is a compile-time constant: ',' for
      // every element except the last, which is followed by ']'.
      constexpr char delim = idx == N - 1 ? ']' : ',';
      auto [after, v] =
          ParseVal<tt, 0, -1, -1, Compressed, delim, false, delim>(curr, end);

      if constexpr (fixed)
        out[idx] = v;
      else
        out.emplace_back(v);
      curr = after;

      if constexpr (!Compressed)
        SKP_SPC();
    }

    assert(*curr == ']');
    curr++;
    return {curr, out};
  }

  /**
   * @brief Parses a single field of the object being parsed.
   *
   * @a curr points at the field's key; the returned pointer points at the
   * delimiter following the value (or at the re-wound key when an optional
   * field is absent).
   *
   * @tparam curr_fld reflection of the field to parse
   * @tparam sz the Size annotation value for this field
   * @tparam min_sz the MinSize annotation value for this field
   * @tparam compressed when false, whitespace is tolerated between tokens
   * @tparam Delim delimiter expected after the field's value
   * @param curr pointer to the field's key
   * @param end one-past-the-end pointer of the input buffer
   * @return a pair of the pointer just past the value and the parsed value
   */
  template <std::meta::info curr_fld, int sz, int min_sz, bool compressed,
            char Delim>
  static std::pair<char *, typename[:std::meta::type_of(curr_fld):]>
  ParseObj(char * curr, char * end)
  {
    constexpr auto t = std::meta::type_of(curr_fld);
    constexpr FieldAnnots curr_ann = FieldAnnots::MkFieldAnnots<curr_fld>();
    constexpr std::string_view ident = std::meta::identifier_of(curr_fld);

    typename[:t:] out;
    std::string_view disp_name = std::string_view(curr_ann.m_disp_name.begin());
    std::string_view name = curr_ann.m_disp_name[0] == '\0' ? ident : disp_name;

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

    auto [after, v] = ParseVal<t, sz, min_sz, curr_ann.m_static_sz, compressed,
                               Delim, curr_ann.m_ignore>(curr, end);

    return {after, v};
  }

  //--------------------------------------------------------//
  // ParseVal                                               //
  //--------------------------------------------------------//
  /**
   * @brief Parses a value of arbitrary reflected type @a T, dispatching to the
   *        appropriate base/tuple/container/object parser.
   *
   * @tparam T reflected type of the value
   * @tparam sz the Size annotation value (-1 when unset)
   * @tparam min_sz the MinSize annotation value (-1 when unset)
   * @tparam static_sz the StaticSize annotation value (-1 when unset)
   * @tparam compressed when false, whitespace is tolerated between tokens
   * @tparam Delim1 primary delimiter following the value
   * @tparam ignore when true, skip the value instead of reading it
   * @tparam Delim2 secondary (closing) delimiter, defaulting to Delim1
   * @param curr pointer to the first character of the value
   * @param end one-past-the-end pointer of the input buffer
   * @return a pair of the pointer just past the value and the parsed value
   */
  template <std::meta::info T, int sz, int min_sz, int static_sz,
            bool compressed, char Delim1, bool ignore = false,
            char Delim2 = Delim1>
  static std::pair<char *, typename[:T:]> ParseVal(char * curr, char * end)
  {
    static_assert(IsSupported<T>());
    typename[:T:] out;
    // constexpr auto base_cls = std::meta::has_template_arguments(T)
    //                               ? std::meta::template_arguments_of(T)[0]
    //                               : T;
    constexpr auto base_cls =
        IsOption<T>() ? std::meta::template_arguments_of(T)[0] : T;
    static_assert(!std::meta::is_array_type(T));
    static_assert(!std::meta::is_array_type(base_cls));

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
      auto [after, v] = ParseBase<base_cls == ^^char ? T : base_cls, Delim1,
                                  ignore, min_sz, sz, Delim2>(curr, end);
      curr = after;
      out = v;

      if constexpr (!compressed)
        SKP_SPC();
    }
    else if constexpr (IsContainer<base_cls>())
    {
      if constexpr (static_sz >= 0)
      {
        auto [after, v] =
            ParseContainerStatic<base_cls, static_sz, compressed>(curr, end);
        curr = after;
        out = v;
      }
      else
      {
        auto [after, v] = ParseContainer<base_cls, compressed>(curr, end);
        curr = after;
        out = v;
      }
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
}; // ObjectParser

} // namespace detail

/**
 * @brief Public entry point: parses a JSON object from the given buffer into a
 *        value of the annotated struct @a T.
 *
 * @tparam T reflected type of the object to parse
 * @param curr pointer to the opening '{' of the object
 * @param end one-past-the-end pointer of the input buffer
 * @return a pair of the pointer just past the object and the parsed value
 */
template <std::meta::info T>
typename[:T:] ParseJson(char * curr, char * end)
{
  return detail::ObjectParser::ParseJson<T>(curr, end).second;
}

template <std::meta::info T>
typename[:T:] ParseJson(char * curr)
{
  return detail::ObjectParser::ParseJson<T>(curr, curr + strlen(curr)).second;
}

template <std::meta::info T>
typename[:T:] ParseJson(std::string & a_in)
{
  char * curr = (char*) a_in.c_str();
  return detail::ObjectParser::ParseJson<T>(curr, curr + a_in.size()).second;
}

template <std::meta::info T>
typename[:T:] ParseJson(std::string_view a_in)
{
  return detail::ObjectParser::ParseJson<T>((char *) a_in.begin(), (char *) a_in.end()).second;
}

} // namespace yjson
