
//===========================================================================//
// Annotations.hpp                                                           //
//===========================================================================//
#pragma once
#include "const_str.hpp"
#include <optional>
#include <meta>
#include "utils.hpp"
#include <iostream>

namespace yjson
{
//----------------------//
// Struct annotations   //
//----------------------//
template <bool Reversed=false> struct Alphabetical
{
  constexpr static bool _Rev = Reversed;
};

//----------------------//
// Field annotations    //
//----------------------//
struct MayAbsent
{
};

template <int Pos> struct Position
{
  constexpr static int _Pos = Pos;
};

template <int Sz> struct Size
{
  constexpr static int _Sz = Sz;
};

struct Ignore
{
};

template <CompTimeStr Name> struct DisplayName
{
};

struct Compressed {};

struct StructAnnots
{
  std::optional<bool> m_alphabetical;
  bool                m_compressed = true;
  
  template<typename T>
  static constexpr StructAnnots MkAnnots()
  {
    static constexpr auto annots = std::define_static_array(std::meta::annotations_of(^^T));
    template for (constexpr auto ann : annots)
    {
      constexpr auto t = std::meta::type_of(ann);
      if constexpr(compare_naked_templates(^^Alphabetical<true>, t))
        std::cout << "Found Alphabetical\n";
      if constexpr(compare_naked_templates(^^Size<1>, t))
        std::cout << "Found size\n";

    }

    return StructAnnots{true, true};
  }
};

} // namespace yjson
