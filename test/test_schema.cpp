//===========================================================================//
//                    "test_schema.cpp":                                     //
//      Tests for Include/yolo-json/schema.hpp (yjson::ParseSchema)          //
//===========================================================================//
#include <yolo-json/schema.hpp>

#include <gtest/gtest.h>

#include <array>
#include <meta>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

// Feed a JSON schema literal to yjson::ParseSchema as a constant-string
// reflection. This must be a macro: std::meta::reflect_constant_string needs
// the literal directly so it stays a constant expression (a helper function's
// parameter would not be one).
#define PARSE(schema) \
  yjson::ParseSchema<std::meta::reflect_constant_string(schema)>()

//---------------------------------------------------------------------------//
// Homogeneous arrays -> std::vector:                                        //
//---------------------------------------------------------------------------//
TEST(SchemaArrayOnlyInt, ObjectWithArrayFields)
{
  constexpr auto kVecInt =
    PARSE(R"({ "type": "array", "items": { "type": "integer" } })");
  static_assert(std::is_same_v<typename[:kVecInt:], std::vector<int>>);
}

TEST(SchemaArrayOnlyStr, ObjectWithArrayFields)
{
  constexpr auto kVecStr =
    PARSE(R"({ "type": "array", "items": { "type": "string" } })");
  static_assert(
    std::is_same_v<typename[:kVecStr:], std::vector<std::string_view>>);
}
// A maximum item count alone does not make the array fixed-size.
TEST(SchemaArrayOnlyBounded, ObjectWithArrayFields)
{
  constexpr auto kVecBounded =
    PARSE(R"({ "type": "array", "items": { "type": "integer" },
               "maxItems": 5 })");
  static_assert(std::is_same_v<typename[:kVecBounded:], std::array<int, 5>>);
}

TEST(SchemaArrayOnlyBool, ObjectWithArrayFields)
{
  constexpr auto kTopArray =
    PARSE(R"({ "type": "array", "items": { "type": "boolean" } })");
  static_assert(std::is_same_v<typename[:kTopArray:], std::vector<bool>>);
}

//---------------------------------------------------------------------------//
// Fixed-size arrays -> std::array:                                          //
//---------------------------------------------------------------------------//
TEST(SchemaArrayOnlyFixed, ObjectWithArrayFields)
{
  constexpr auto kArrNum =
    PARSE(R"({ "type": "array", "items": { "type": "number" },
               "minItems": 4, "maxItems": 4 })");
  static_assert(std::is_same_v<typename[:kArrNum:], std::array<double, 4>>);
}

//---------------------------------------------------------------------------//
// Heterogeneous arrays -> std::tuple:                                       //
//---------------------------------------------------------------------------//
TEST(SchemaArrayOnlyTuple, ObjectWithArrayFields)
{
  constexpr auto kTuple =
    PARSE(R"({ "type": "array",
               "prefixItems": [ { "type": "integer" }, { "type": "string" },
                                { "type": "boolean" } ] })");
  static_assert(std::is_same_v<typename[:kTuple:],
                             std::tuple<int, std::string_view, bool>>);
}

TEST(SchemaArrayOnlyTupVec, ObjectWithArrayFields)
{
  constexpr auto kTupleTail =
    PARSE(R"({ "type": "array",
               "prefixItems": [ { "type": "integer" } ],
               "items": { "type": "string" } })");
  static_assert(
    std::is_same_v<typename[:kTupleTail:],
                   std::tuple<int, std::vector<std::string_view>>>);
}
TEST(SchemaArrayOnlyTupleSame, ObjectWithArrayFields)
{
  constexpr auto kDraft07Tuple =
    PARSE(R"({ "type": "array",
               "items": [ { "type": "integer" }, { "type": "integer" } ] })");
  static_assert(std::is_same_v<typename[:kDraft07Tuple:], std::tuple<int, int>>);
}
//---------------------------------------------------------------------------//
// Arrays nested inside an object schema:                                    //
//---------------------------------------------------------------------------//
TEST(SchemaArrayTest, ObjectWithArrayFields)
{
  constexpr auto t = PARSE(
      R"({
        "type": "object",
        "properties": {
          "scores": { "type": "array", "items": { "type": "integer" } },
          "tags":   { "type": "array", "prefixItems": [
                       { "type": "string" }, { "type": "boolean" } ] }
        },
        "required": ["scores", "tags"]
      })");
  using T = typename[:t:];
  static_assert(std::is_same_v<decltype(T{}.scores), std::vector<int>>);
  static_assert(
      std::is_same_v<decltype(T{}.tags), std::tuple<std::string_view, bool>>);

  T v{};
  v.scores.push_back(7);
  EXPECT_EQ(v.scores.size(), 1u);
  EXPECT_EQ(v.scores[0], 7);
}

//---------------------------------------------------------------------------//
// Fixed-size arrays carry a StaticSize field annotation:                    //
//---------------------------------------------------------------------------//
TEST(SchemaArrayAnnotTest, FixedArrayGetsStaticSize)
{
  constexpr auto t = PARSE(
      R"({
        "type": "object",
        "properties": {
          "values": { "type": "array", "items": { "type": "integer" },
                      "minItems": 4, "maxItems": 4 }
        },
        "required": ["values"]
      })");
  using T = typename[:t:];
  static_assert(std::is_same_v<decltype(T{}.values), std::array<int, 4>>);

  constexpr auto anns = yjson::FieldAnnots::MkFldAnnots<t>();
  static_assert(anns.size() == 1u);
  static_assert(anns[0].m_static_sz == 4);
}

TEST(SchemaArrayAnnotTest, MaxOnlyArrayHasNoStaticSize)
{
  constexpr auto t = PARSE(
      R"({
        "type": "object",
        "properties": {
          "values": { "type": "array", "items": { "type": "integer" },
                      "maxItems": 4 }
        },
        "required": ["values"]
      })");
  using T = typename[:t:];
  static_assert(std::is_same_v<decltype(T{}.values), std::array<int, 4>>);

  constexpr auto anns = yjson::FieldAnnots::MkFldAnnots<t>();
  static_assert(anns.size() == 1u);
  static_assert(anns[0].m_static_sz == -1);
}

