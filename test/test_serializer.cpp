//===========================================================================//
//                    "test_serializer.cpp":                                 //
//      End-to-end tests for Include/serializer.hpp (yjson::SerializeJson)   //
//===========================================================================//
#include "Include/parser.hpp"
#include "Include/serializer.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

//---------------------------------------------------------------------------//
// Test structs:                                                             //
//---------------------------------------------------------------------------//
namespace test_types
{

struct Basic
{
  int i;
  double d;
  std::string s;
};

struct Nested
{
  int x;
  Basic b;
};

struct Positioned
{
  [[= yjson::Position{1}]] int second;
  [[= yjson::Position{0}]] int first;
};

struct DisplayNamed
{
  [[= yjson::DisplayName{"renamed"}]] int original;
  int normal;
};

struct Ignored
{
  [[= yjson::Ignore{}]] int skip;
  int keep;
};

struct TupleFld
{
  std::tuple<int, double, std::string> t;
  int rest;
};

struct ArrFld
{
  std::array<int, 3> a;
  int rest;
};

struct VecFld
{
  std::vector<std::string> v;
  int rest;
};

struct OptFld
{
  std::optional<int> o;
  int rest;
};

struct[[= yjson::NotCompressed{}]] NotComp
{
  int a;
  int b;
  std::string s;
};

struct[[= yjson::RandomOrder{}]] RandOpt
{
  int first;
  [[= yjson::MayAbsent{}]] std::optional<int> maybe;
  int last;
};

struct Sized
{
  [[= yjson::Size{5}]] int fixed;
  int after;
};

struct[[= yjson::RandomOrder{}]] Shuffled
{
  int a;
  int b;
  int c;
  int d;
  int e;
};

} // namespace test_types

//---------------------------------------------------------------------------//
// Helpers:                                                                  //
//---------------------------------------------------------------------------//
namespace
{

template <typename T> T Parse(std::string json)
{
  auto [rest, value] =
      yjson::ParseJson<^^T>(json.data(), json.data() + json.size());
  (void)rest;
  return value;
}

} // namespace

//---------------------------------------------------------------------------//
// Deterministic (seeded) serialization:                                     //
//---------------------------------------------------------------------------//
TEST(SerializerTest, SerializesBasicTypesExactly)
{
  auto json = yjson::SerializeJson<^^test_types::Basic>(
      test_types::Basic{42, 3.14, "hello"}, 1);
  EXPECT_EQ(json, R"({"i":42,"d":3.14,"s":"hello"})");
}

TEST(SerializerTest, SerializesNestedStructs)
{
  auto json = yjson::SerializeJson<^^test_types::Nested>(
      test_types::Nested{1, {42, 3.14, "hi"}}, 1);
  auto v = Parse<test_types::Nested>(json);
  EXPECT_EQ(v.x, 1);
  EXPECT_EQ(v.b.i, 42);
  EXPECT_DOUBLE_EQ(v.b.d, 3.14);
  EXPECT_EQ(v.b.s, "hi");
}

TEST(SerializerTest, HonorsPositionAnnotation)
{
  auto json = yjson::SerializeJson<^^test_types::Positioned>(
      test_types::Positioned{20, 10}, 1);
  EXPECT_EQ(json, R"({"first":10,"second":20})");
}

TEST(SerializerTest, HonorsDisplayNameAnnotation)
{
  auto json = yjson::SerializeJson<^^test_types::DisplayNamed>(
      test_types::DisplayNamed{42, 7}, 1);
  EXPECT_EQ(json, R"({"renamed":42,"normal":7})");
}

TEST(SerializerTest, SerializesIgnoreFieldWithItsValue)
{
  // Ignore is a parse-side hint: the key is still emitted (the parser needs it
  // to skip the value), and the value is serialized normally.
  auto json = yjson::SerializeJson<^^test_types::Ignored>(
      test_types::Ignored{123, 456}, 1);
  EXPECT_EQ(json, R"({"skip":123,"keep":456})");

  auto v = Parse<test_types::Ignored>(json);
  EXPECT_EQ(v.skip, 0); // skipped by the parser -> default-initialized
  EXPECT_EQ(v.keep, 456);
}

//---------------------------------------------------------------------------//
// Round-trips:                                                              //
//---------------------------------------------------------------------------//
TEST(SerializerTest, RoundTripsBasicTypes)
{
  auto json = yjson::SerializeJson<^^test_types::Basic>(
      test_types::Basic{42, 3.14, "hello"}, 7);
  auto v = Parse<test_types::Basic>(json);
  EXPECT_EQ(v.i, 42);
  EXPECT_DOUBLE_EQ(v.d, 3.14);
  EXPECT_EQ(v.s, "hello");
}

TEST(SerializerTest, RoundTripsTupleField)
{
  auto json = yjson::SerializeJson<^^test_types::TupleFld>(
      test_types::TupleFld{{1, 2.5, "hi"}, 9}, 7);
  auto v = Parse<test_types::TupleFld>(json);
  EXPECT_EQ(std::get<0>(v.t), 1);
  EXPECT_DOUBLE_EQ(std::get<1>(v.t), 2.5);
  EXPECT_EQ(std::get<2>(v.t), "hi");
  EXPECT_EQ(v.rest, 9);
}

TEST(SerializerTest, RoundTripsFixedArrayField)
{
  auto json = yjson::SerializeJson<^^test_types::ArrFld>(
      test_types::ArrFld{{1, 2, 3}, 9}, 7);
  auto v = Parse<test_types::ArrFld>(json);
  EXPECT_EQ(v.a, (std::array<int, 3>{1, 2, 3}));
  EXPECT_EQ(v.rest, 9);
}

TEST(SerializerTest, RoundTripsDynamicVectorField)
{
  auto json = yjson::SerializeJson<^^test_types::VecFld>(
      test_types::VecFld{{"a", "b"}, 9}, 7);
  auto v = Parse<test_types::VecFld>(json);
  EXPECT_EQ(v.v, (std::vector<std::string>{"a", "b"}));
  EXPECT_EQ(v.rest, 9);
}

TEST(SerializerTest, RoundTripsOptionalWhenPresent)
{
  auto json =
      yjson::SerializeJson<^^test_types::OptFld>(test_types::OptFld{42, 9}, 7);
  auto v = Parse<test_types::OptFld>(json);
  ASSERT_TRUE(v.o.has_value());
  EXPECT_EQ(v.o.value(), 42);
  EXPECT_EQ(v.rest, 9);
}

TEST(SerializerTest, RoundTripsOptionalWhenNull)
{
  auto json = yjson::SerializeJson<^^test_types::OptFld>(
      test_types::OptFld{std::nullopt, 9}, 7);
  auto v = Parse<test_types::OptFld>(json);
  EXPECT_FALSE(v.o.has_value());
  EXPECT_EQ(v.rest, 9);
}

//---------------------------------------------------------------------------//
// Size annotation (fixed-width zero padding):                               //
//---------------------------------------------------------------------------//
TEST(SerializerTest, ZeroPadsSizedField)
{
  auto json =
      yjson::SerializeJson<^^test_types::Sized>(test_types::Sized{42, 9}, 1);
  EXPECT_EQ(json, R"({"fixed":00042,"after":9})");

  auto v = Parse<test_types::Sized>(json);
  EXPECT_EQ(v.fixed, 42);
  EXPECT_EQ(v.after, 9);
}

//---------------------------------------------------------------------------//
// NotCompressed (random whitespace):                                        //
//---------------------------------------------------------------------------//
TEST(SerializerTest, RoundTripsNotCompressedWhitespace)
{
  auto json = yjson::SerializeJson<^^test_types::NotComp>(
      test_types::NotComp{1, 2, "hi"}, 7);
  auto v = Parse<test_types::NotComp>(json);
  EXPECT_EQ(v.a, 1);
  EXPECT_EQ(v.b, 2);
  EXPECT_EQ(v.s, "hi");
}

//---------------------------------------------------------------------------//
// MayAbsent (random omission):                                              //
//---------------------------------------------------------------------------//
TEST(SerializerTest, MayAbsentNeverOmittedWithZeroProbability)
{
  yjson::JsonSerializer ser{42, 0.0};
  auto json =
      ser.Serialize<^^test_types::RandOpt>(test_types::RandOpt{1, 7, 3});
  auto v = Parse<test_types::RandOpt>(json);
  EXPECT_EQ(v.first, 1);
  ASSERT_TRUE(v.maybe.has_value());
  EXPECT_EQ(v.maybe.value(), 7);
  EXPECT_EQ(v.last, 3);
}

TEST(SerializerTest, MayAbsentAlwaysOmittedWithUnitProbability)
{
  yjson::JsonSerializer ser{42, 1.0};
  auto json =
      ser.Serialize<^^test_types::RandOpt>(test_types::RandOpt{1, 7, 3});
  auto v = Parse<test_types::RandOpt>(json);
  EXPECT_EQ(v.first, 1);
  EXPECT_FALSE(v.maybe.has_value());
  EXPECT_EQ(v.last, 3);
}

TEST(SerializerTest, MayAbsentRoundTripsWithDefaultProbability)
{
  auto json = yjson::SerializeJson<^^test_types::RandOpt>(
      test_types::RandOpt{1, 7, 3}, 42);
  auto v = Parse<test_types::RandOpt>(json);
  EXPECT_EQ(v.first, 1);
  if (v.maybe.has_value())
    EXPECT_EQ(v.maybe.value(), 7);
  EXPECT_EQ(v.last, 3);
}

TEST(SerializerTest, MayAbsentUsesRandomness)
{
  bool saw_present = false;
  bool saw_absent = false;
  for (unsigned seed = 0; seed < 64; ++seed)
  {
    auto json = yjson::SerializeJson<^^test_types::RandOpt>(
        test_types::RandOpt{1, 7, 3}, seed);
    auto v = Parse<test_types::RandOpt>(json);
    if (v.maybe.has_value())
      saw_present = true;
    else
      saw_absent = true;
  }
  EXPECT_TRUE(saw_present);
  EXPECT_TRUE(saw_absent);
}

//---------------------------------------------------------------------------//
// RandomOrder (shuffled field order on serialize):                          //
//---------------------------------------------------------------------------//
TEST(SerializerTest, RandomOrderShufflesFieldOrder)
{
  test_types::Shuffled v{1, 2, 3, 4, 5};
  std::set<std::string> seen;
  for (unsigned seed = 0; seed < 64; ++seed)
  {
    auto json = yjson::SerializeJson<^^test_types::Shuffled>(v, seed);
    seen.insert(json);
    // Regardless of field order, the output must round-trip.
    auto r = Parse<test_types::Shuffled>(json);
    EXPECT_EQ(r.a, 1);
    EXPECT_EQ(r.b, 2);
    EXPECT_EQ(r.c, 3);
    EXPECT_EQ(r.d, 4);
    EXPECT_EQ(r.e, 5);
  }
  EXPECT_GT(seen.size(), 1u); // field order actually varies across seeds
}

TEST(SerializerTest, RandomOrderIsDeterministicForFixedSeed)
{
  auto j1 = yjson::SerializeJson<^^test_types::Shuffled>(
      test_types::Shuffled{1, 2, 3, 4, 5}, 42);
  auto j2 = yjson::SerializeJson<^^test_types::Shuffled>(
      test_types::Shuffled{1, 2, 3, 4, 5}, 42);
  EXPECT_EQ(j1, j2);
}
