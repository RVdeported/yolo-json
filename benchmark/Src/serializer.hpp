//===========================================================================//
//                    "serializer.hpp":                                       //
//      Reflection-driven JSON serializer (yjson::SerializeJson)             //
//===========================================================================//
#pragma once

#include "Include/parser.hpp"
#include "Include/utils.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <meta>
#include <random>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace yjson
{

// Serializes reflection-described structs into JSON, mirroring the parser in
// Include/parser.hpp. It reuses the same compile-time utilities (StructAnnots,
// FieldAnnots, GetOrderedField, ...) so ordering, renaming, skipping and
// sizing annotations are honoured. For the three "random" annotations the
// effect is faked with a random number generator:
//
//   * MayAbsent     -> the field is randomly omitted (may_absent_prob);
//   * NotCompressed -> whitespace is emitted at random (0-3 spaces);
//   * RandomOrder   -> the fields are emitted in a shuffled order.
class JsonSerializer
{
public:
  explicit JsonSerializer(unsigned seed, double may_absent_prob = 0.5)
      : rng_(seed), may_absent_prob_(may_absent_prob)
  {
  }

  template <std::meta::info T>
  std::string Serialize(const typename[:T:] & value)
  {
    std::string out;
    SerializeVal<T>(value, out);
    return out;
  }

private:
  std::mt19937 rng_;
  double may_absent_prob_;

  // Emit a random amount of whitespace when the surrounding object is not
  // compressed; emit nothing for compressed objects.
  template <bool Compressed> void MaybeSpaces(std::string & out)
  {
    if constexpr (!Compressed)
      out.append(std::uniform_int_distribution<int>(0, 3)(rng_), ' ');
  }

  // Serialize a single value: an optional, a bool, a tuple, a base type, a
  // container, or (by default) a nested object.
  template <std::meta::info T, bool Compressed = true>
  void SerializeVal(const typename[:T:] & value, std::string & out,
                                        int width = 0)
  {
    if constexpr (IsOption<T>())
    {
      if (!value.has_value())
      {
        out += "null";
        return;
      }
      SerializeVal<std::meta::template_arguments_of(T)[0], Compressed>(
          value.value(), out, width);
    }
    else if constexpr (IsBool(T))
    {
      out += value ? "true" : "false";
    }
    else if constexpr (IsTuple(T))
    {
      SerializeTuple<T, Compressed>(value, out);
    }
    else if constexpr (IsBase<T>())
    {
      SerializeBase<T>(value, out, width);
    }
    else if constexpr (IsContainer<T>())
    {
      SerializeContainer<T, Compressed>(value, out);
    }
    else
    {
      SerializeObject<T>(value, out);
    }
  }

  // Base types: numbers (optionally zero-padded to @a width to satisfy the
  // Size / MinSize annotations) and strings (emitted verbatim, unescaped,
  // matching the parser's verbatim string handling).
  template <std::meta::info T>
  void SerializeBase(const typename[:T:] & value, std::string & out, int width)
  {
    if constexpr (std::meta::is_floating_point_type(T))
    {
      std::string num = std::format("{}", value);
      PadLeftZero(num, width);
      out += num;
    }
    else if constexpr (std::meta::is_integral_type(T))
    {
      // std::to_string promotes char to int, so char is emitted as an integer
      // (mirroring the parser, which reads char fields as integers).
      std::string num = std::to_string(value);
      PadLeftZero(num, width);
      out += num;
    }
    else
    {
      out += '"';
      out += value;
      out += '"';
    }
  }

  // Right-align a number in a fixed-width field by zero-padding it on the
  // left (keeping a leading '-' sign in front of the padding).
  static void PadLeftZero(std::string & num, int width)
  {
    if (width <= 0 || static_cast<int>(num.size()) >= width)
      return;
    const std::size_t sign = (!num.empty() && num[0] == '-') ? 1 : 0;
    num.insert(sign, static_cast<std::size_t>(width) - num.size(), '0');
  }

  template <std::meta::info T, bool Compressed>
  void SerializeTuple(const typename[:T:] & value, std::string & out)
  {
    constexpr auto types =
        std::define_static_array(std::meta::template_arguments_of(T));
    constexpr auto sz = types.size();

    out += '[';
    MaybeSpaces<Compressed>(out);

    template for (constexpr auto idx : std::views::indices(sz))
    {
      if constexpr (idx > 0)
      {
        out += ',';
        MaybeSpaces<Compressed>(out);
      }
      constexpr auto tt = types[idx];
      SerializeVal<tt, Compressed>(std::get<idx>(value), out);
    }

    MaybeSpaces<Compressed>(out);
    out += ']';
  }

  template <std::meta::info T, bool Compressed>
  void SerializeContainer(const typename[:T:] & value, std::string & out)
  {
    constexpr auto tt = std::meta::template_arguments_of(T)[0];

    out += '[';
    MaybeSpaces<Compressed>(out);

    bool first = true;
    for (const auto & elem : value)
    {
      if (!first)
      {
        out += ',';
        MaybeSpaces<Compressed>(out);
      }
      first = false;
      SerializeVal<tt, Compressed>(elem, out);
    }

    MaybeSpaces<Compressed>(out);
    out += ']';
  }

  // Emit a single object field (key + value) and update @a emitted. MayAbsent
  // fields are randomly dropped by consuming the RNG, mirroring the parser's
  // expectation that they may simply be absent.
  template <std::meta::info T, std::meta::info curr_fld, bool Compressed>
  void EmitField(const typename[:T:] & value, std::string & out, bool & emitted)
  {
    constexpr FieldAnnots curr_ann = FieldAnnots::MkFieldAnnots<curr_fld>();

    // MayAbsent: randomly drop the field to simulate absence.
    if constexpr (curr_ann.m_may_absent)
    {
      if (std::bernoulli_distribution(may_absent_prob_)(rng_))
        return;
    }

    constexpr std::string_view ident = std::meta::identifier_of(curr_fld);
    const std::string_view name =
        curr_ann.m_disp_name[0] == '\0'
            ? ident
            : std::string_view(curr_ann.m_disp_name.data());

    if (emitted)
    {
      out += ',';
      MaybeSpaces<Compressed>(out);
    }
    emitted = true;

    out += '"';
    out += name;
    out += '"';
    MaybeSpaces<Compressed>(out);
    out += ':';
    MaybeSpaces<Compressed>(out);

    constexpr auto t = std::meta::type_of(curr_fld);
    const int width = std::max({0, curr_ann.m_sz, curr_ann.m_min_sz});
    SerializeVal<t, Compressed>(value.[:curr_fld:], out, width);
  }

  template <std::meta::info T>
  void SerializeObject(const typename[:T:] & value, std::string & out)
  {
    using T_ = typename[:T:];
    constexpr StructAnnots strAnnots = StructAnnots::MkStrAnnots<T>();
    constexpr auto flds = GetRelFields<T_>();
    constexpr auto flds_ord = GetOrderedField<T_>();
    constexpr auto sz = flds.size();
    constexpr bool compressed = strAnnots.m_compressed;

    out += '{';
    MaybeSpaces<compressed>(out);

    bool emitted = false;

    if constexpr (strAnnots.m_random_order)
    {
      // RandomOrder: emit fields in a runtime-shuffled order (deterministic
      // for a fixed seed). Each field still needs a compile-time reflection
      // handle, so loop over runtime positions and dispatch to the matching
      // field via a compile-time scan.
      std::array<int, sz> order = flds_ord;
      std::shuffle(order.begin(), order.end(), rng_);

      for (std::size_t pos = 0; pos < sz; ++pos)
      {
        template for (constexpr auto i : std::views::indices(sz))
        {
          if (order[pos] == static_cast<int>(i))
            EmitField<T, flds[i], compressed>(value, out, emitted);
        }
      }
    }
    else
    {
      template for (constexpr auto idx : std::views::indices(sz))
      {
        constexpr auto curr_fld = flds[flds_ord[idx]];
        EmitField<T, curr_fld, compressed>(value, out, emitted);
      }
    }

    MaybeSpaces<compressed>(out);
    out += '}';
  }
};

// Serialize @a value into JSON with a fixed seed (deterministic output).
template <std::meta::info T>
std::string SerializeJson(const typename[:T:] & value, unsigned seed)
{
  return JsonSerializer{seed}.Serialize<T>(value);
}

// Serialize @a value into JSON with a non-deterministic seed.
template <std::meta::info T>
std::string SerializeJson(const typename[:T:] & value)
{
  std::random_device rd;
  return JsonSerializer{rd()}.Serialize<T>(value);
}

} // namespace yjson
