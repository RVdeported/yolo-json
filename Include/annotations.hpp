
//===========================================================================//
// Annotations.hpp                                                           //
//===========================================================================//
#pragma once
#include "const_str.hpp"
#include "utils.hpp"
#include <array>
#include <iostream>
#include <meta>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

namespace yjson
{
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

struct DisplayName
{
  std::string name;
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
  std::string_view m_disp_name = "";
  int m_min_sz = -1;

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

    if constexpr (constexpr auto ign = get_annotations<Ignore, fld>();
                  ign.size() > 0)
    {
      ann.m_ignore = true;
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

} // namespace yjson
