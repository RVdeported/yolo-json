
//===========================================================================//
// Annotations.hpp                                                           //
//===========================================================================//
#pragma once
#include "const_str.hpp"

namespace yjson
{
//----------------------//
// Struct annotations   //
//----------------------//
template <bool Reversed> struct Alphabetical
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
} // namespace yjson
