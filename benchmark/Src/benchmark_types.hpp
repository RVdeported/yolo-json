//===========================================================================//
//                    "benchmark_types.hpp":                                  //
//    Benchmark test-case types for yolo-json (parser + serializer).         //
//===========================================================================//
// One section per benchmark scenario. Each defines an annotated struct plus a
// Make*() factory that fills a representative "large" value. Benchmarks should
// (de)serialize via yjson::ParseJson<^^T> / yjson::SerializeJson<^^T> with a
// FIXED seed so output is deterministic.
//
// Notes for the benchmark driver:
//   * Ignore and RandomOrder are PARSE-side annotations. The serializer emits
//     ignored fields normally and always writes fields in definition order
//     (RandomOrder is not honoured on serialize). To benchmark random-order
//     PARSING you must feed a scrambled JSON buffer, not serializer output.
//   * NotCompressed emits 0-3 random spaces per token (seeded); parse-side it
//     toggles whitespace tolerance.
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>
#include <utils.hpp>

namespace benchmark_types
{

// A small nested record reused by several sections.
struct Point
{
  double x;
  double y;
  double z;
};

//---------------------------------------------------------------------------//
// (1) Large JSON with dynamic arrays                                        //
//---------------------------------------------------------------------------//
struct LargeDynamic
{
  std::array<int, 100> ints;
  std::array<double, 100> doubles;
  std::array<std::string_view, 100> strings;
  std::array<Point, 100> points;              // vector of nested objects
  std::array<std::array<int, 10>, 10> matrix; // nested dynamic arrays
  int id;
  std::string_view label;
};
//---------------------------------------------------------------------------//
// Fixed arrays (StaticSize-annotated std::array)                            //
//---------------------------------------------------------------------------//
// Four single-field benchmarks, one per element type. Each struct holds
// exactly one fixed array and nothing else, so each measurement isolates the
// unrolled StaticSize parser path for that element type.
struct FixedDoubles
{
  [[= yjson::StaticSize{100}]] std::array<double, 100> doubles;
};

struct FixedStrings
{
  [[= yjson::StaticSize{100}]] std::array<std::string_view, 100> strings;
};

struct FixedPoints
{
  [[= yjson::StaticSize{100}]] std::array<Point, 100> points;
};

struct FixedInts
{
  [[= yjson::StaticSize{100}]] std::array<int, 100> ints;
};
//---------------------------------------------------------------------------//
// Dynamic containers (StaticSize-annotated std::vector)                     //
//---------------------------------------------------------------------------//
// Dynamic (std::vector) counterparts of the fixed-array benchmarks above:
// StaticSize still gives the parser a compile-time element count, so the
// vector is reserved once and parsed by the same unrolled loop.
struct DynamicInts
{
  [[= yjson::StaticSize{100}]] std::vector<int> ints;
};

struct DynamicPoints
{
  [[= yjson::StaticSize{100}]] std::vector<Point> points;
};
//---------------------------------------------------------------------------//
// (2) Large JSON, NotCompressed (whitespace emitted / tolerated)            //
//---------------------------------------------------------------------------//
struct[[= yjson::NotCompressed{}]] LargeNotCompressed
{
  int i0;
  int i1;
  int i2;
  int i3;
  double d0;
  double d1;
  double d2;
  double d3;
  bool b0;
  bool b1;
  Point origin;
};

//---------------------------------------------------------------------------//
// (3) JSON with fixed-size types (std::array + Size/MinSize)                //
//---------------------------------------------------------------------------//
struct LargeFixed
{
  [[= yjson::Size{8}]] int zero_padded1;
  [[= yjson::Size{8}]] int zero_padded2;
  [[= yjson::Size{8}]] int zero_padded3;
  [[= yjson::Size{8}]] int zero_padded4;
  [[= yjson::Size{8}]] int zero_padded5;
  [[= yjson::Size{8}]] int zero_padded6;
  [[= yjson::Size{8}]] int zero_padded7;
  [[= yjson::MinSize{4}]] int min_width;
  int tail;
};

//---------------------------------------------------------------------------//
// (4) JSON with many fields to be ignored (Ignore is parse-side only)       //
//---------------------------------------------------------------------------//
struct LargeIgnored
{
  [[= yjson::Ignore{}]] int skip_00;
  [[= yjson::Ignore{}]] int skip_01;
  [[= yjson::Ignore{}]] int skip_02;
  [[= yjson::Ignore{}]] int skip_03;
  [[= yjson::Ignore{}]] int skip_04;
  [[= yjson::Ignore{}]] int skip_05;
  [[= yjson::Ignore{}]] int skip_06;
  [[= yjson::Ignore{}]] int skip_07;
  [[= yjson::Ignore{}]] int skip_08;
  [[= yjson::Ignore{}]] int skip_09;
  [[= yjson::Ignore{}]] int skip_10;
  [[= yjson::Ignore{}]] int skip_11;
  [[= yjson::Ignore{}]] int skip_12;
  [[= yjson::Ignore{}]] int skip_13;
  [[= yjson::Ignore{}]] int skip_14;
  [[= yjson::Ignore{}]] int skip_15;
  [[= yjson::Ignore{}]] int skip_16;
  [[= yjson::Ignore{}]] int skip_17;
  [[= yjson::Ignore{}]] int skip_18;
  [[= yjson::Ignore{}]] int skip_19;
  int id;
  std::array<double, 10> nums;
  double value;
  Point origin;
};

struct LargeJust
{
  int skip_00;
  int skip_01;
  int skip_02;
  int skip_03;
  int skip_04;
  int skip_05;
  int skip_06;
  int skip_07;
  int skip_08;
  int skip_09;
  int skip_10;
  int skip_11;
  int skip_12;
  int skip_13;
  int skip_14;
  int skip_15;
  int skip_16;
  int skip_17;
  int skip_18;
  int skip_19;
  int id;
  double value;
  Point origin;
};

//---------------------------------------------------------------------------//
// (5) Large JSON with random order of fields (RandomOrder is parse-only)    //
//---------------------------------------------------------------------------//
struct[[= yjson::RandomOrder{}]] LargeRandomOrder
{
  int f00;
  int f01;
  int f02;
  int f03;
  int f04;
  int f05;
  int f06;
  int f07;
  int f08;
  int f09;
  int f10;
  int f11;
  int f12;
  int f13;
  int f14;
  int f15;
  int f16;
  int f17;
  int f18;
  int f19;
  bool flag;
  double ratio;
};

//===========================================================================//
// Factories                                                                  //
//===========================================================================//

inline LargeDynamic MakeLargeDynamic(int n)
{
  LargeDynamic v;
  n = std::min(100, n);
  for (std::size_t i = 0; i < n; ++i)
  {
    v.ints[i] = static_cast<int>(i % 1000);
    v.doubles[i] = static_cast<double>(i) * 0.5;
    v.strings[i] = "item_" + std::to_string(i);
    v.points[i] = Point{static_cast<double>(i), static_cast<double>(i) + 0.5,
                        static_cast<double>(i) + 1.0};
  }

  int rows = n / 10 + 1;
  for (std::size_t r = 0; r < rows; ++r)
  {
    for (std::size_t c = 0; c < rows; ++c)
      v.matrix[r][c] = static_cast<int>(r * n + c);
  }
  v.id = 12345;
  v.label = "large_dynamic";
  return v;
}

inline FixedDoubles MakeFixedDoubles()
{
  FixedDoubles v{};
  for (std::size_t i = 0; i < v.doubles.size(); ++i)
    v.doubles[i] = static_cast<double>(i) * 0.5;
  return v;
}

inline FixedStrings MakeFixedStrings()
{
  FixedStrings v{};
  for (std::size_t i = 0; i < v.strings.size(); ++i)
  {
    std::string * str = new std::string("item_" + std::to_string(i));
    v.strings[i] = *str;
  }
  return v;
}

inline FixedPoints MakeFixedPoints()
{
  FixedPoints v{};
  for (std::size_t i = 0; i < v.points.size(); ++i)
    v.points[i] = Point{static_cast<double>(i), static_cast<double>(i) + 0.5,
                        static_cast<double>(i) + 1.0};
  return v;
}

inline FixedInts MakeFixedInts()
{
  FixedInts v{};
  for (std::size_t i = 0; i < v.ints.size(); ++i)
    v.ints[i] = static_cast<int>(i % 1000);
  return v;
}

inline DynamicInts MakeDynamicInts()
{
  DynamicInts v{};
  v.ints.resize(100);
  for (std::size_t i = 0; i < v.ints.size(); ++i)
    v.ints[i] = static_cast<int>(i % 1000);
  return v;
}

inline DynamicPoints MakeDynamicPoints()
{
  DynamicPoints v{};
  v.points.resize(100);
  for (std::size_t i = 0; i < v.points.size(); ++i)
    v.points[i] = Point{static_cast<double>(i), static_cast<double>(i) + 0.5,
                        static_cast<double>(i) + 1.0};
  return v;
}

inline LargeNotCompressed MakeLargeNotCompressed(std::size_t n)
{
  LargeNotCompressed v;
  v.i0 = 1;
  v.i1 = 2;
  v.i2 = 3;
  v.i3 = 4;
  v.d0 = 0.5;
  v.d1 = 1.5;
  v.d2 = 2.5;
  v.d3 = 3.5;
  v.b0 = true;
  v.b1 = false;
  const std::size_t m = n / 10;
  // v.pts.resize(m);
  // for (std::size_t i = 0; i < n; ++i)
  //   v.nums[i] = static_cast<int>(i);
  // for (std::size_t i = 0; i < m; ++i)
  //   v.pts[i] = Point{static_cast<double>(i), -static_cast<double>(i), 0.0};
  v.origin = Point{1.0, 2.0, 3.0};
  return v;
}

inline LargeFixed MakeLargeFixed()
{
  LargeFixed v{};
  // REFLECTIOOONN
  constexpr auto flds = yjson::GetRelFields<LargeFixed>();
  template for (constexpr auto i : std::views::indices(flds.size()))
  {
    constexpr auto fld = flds[i];
    constexpr std::string_view name = std::meta::identifier_of(fld);
    if constexpr ((std::meta::type_of(fld) ==
                   ^^int)&&name.starts_with("zero_padded"))
      v.[:fld:] = 42000000;
  }
  v.min_width = 7;
  v.tail = 99;
  return v;
}

inline LargeIgnored MakeLargeIgnored()
{
  LargeIgnored v{};
  // Fill the ignored fields too: the serializer emits them, the parser skips
  // them (they come back default-initialized).
  v.skip_00 = 100;
  v.skip_01 = 101;
  v.skip_02 = 102;
  v.skip_03 = 103;
  v.skip_04 = 104;
  v.skip_05 = 105;
  v.skip_06 = 106;
  v.skip_07 = 107;
  v.skip_08 = 108;
  v.skip_09 = 109;
  v.skip_10 = 110;
  v.skip_11 = 111;
  v.skip_12 = 112;
  v.skip_13 = 113;
  v.skip_14 = 114;
  v.skip_15 = 115;
  v.skip_16 = 116;
  v.skip_17 = 117;
  v.skip_18 = 118;
  v.skip_19 = 119;
  v.id = 4242;
  v.value = 6.25;
  for (std::size_t i = 0; i < 10; ++i)
    v.nums[i] = static_cast<int>(i % 64);
  v.origin = Point{9.0, 8.0, 7.0};
  return v;
}

inline LargeJust MakeLargeJust()
{
  LargeJust v{};
  // Fill the ignored fields too: the serializer emits them, the parser skips
  // them (they come back default-initialized).
  v.skip_00 = 100;
  v.skip_01 = 101;
  v.skip_02 = 102;
  v.skip_03 = 103;
  v.skip_04 = 104;
  v.skip_05 = 105;
  v.skip_06 = 106;
  v.skip_07 = 107;
  v.skip_08 = 108;
  v.skip_09 = 109;
  v.skip_10 = 110;
  v.skip_11 = 111;
  v.skip_12 = 112;
  v.skip_13 = 113;
  v.skip_14 = 114;
  v.skip_15 = 115;
  v.skip_16 = 116;
  v.skip_17 = 117;
  v.skip_18 = 118;
  v.skip_19 = 119;
  v.id = 4242;
  v.value = 6.25;
  v.origin = Point{9.0, 8.0, 7.0};
  return v;
}

inline LargeRandomOrder MakeLargeRandomOrder(std::size_t n)
{
  LargeRandomOrder v{};
  v.f00 = 0;
  v.f01 = 1;
  v.f02 = 2;
  v.f03 = 3;
  v.f04 = 4;
  v.f05 = 5;
  v.f06 = 6;
  v.f07 = 7;
  v.f08 = 8;
  v.f09 = 9;
  v.f10 = 10;
  v.f11 = 11;
  v.f12 = 12;
  v.f13 = 13;
  v.f14 = 14;
  v.f15 = 15;
  v.f16 = 16;
  v.f17 = 17;
  v.f18 = 18;
  v.f19 = 19;
  v.flag = true;
  v.ratio = 3.75;
  // v.nums.resize(n);
  // for (std::size_t i = 0; i < n; ++i)
  //   v.nums[i] = static_cast<int>(i);
  return v;
}

} // namespace benchmark_types
