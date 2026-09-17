//===========================================================================//
//                    "test_schema.cpp":                                     //
//      Tests for Include/yolo-json/schema.hpp (yjson::ParseSchema)          //
//===========================================================================//
#include <yolo-json/schema.hpp>

#include <gtest/gtest.h>

#include <array>
#include <meta>
#include <optional>
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

//---------------------------------------------------------------------------//
// Scalar type mapping:                                                      //
//---------------------------------------------------------------------------//
TEST(SchemaScalarTest, StringType)
{
  constexpr auto t = PARSE(R"({ "type": "string" })");
  static_assert(std::is_same_v<typename[:t:], std::string_view>);
}

TEST(SchemaScalarTest, NumberType)
{
  constexpr auto t = PARSE(R"({ "type": "number" })");
  static_assert(std::is_same_v<typename[:t:], double>);
}

TEST(SchemaScalarTest, IntegerType)
{
  constexpr auto t = PARSE(R"({ "type": "integer" })");
  static_assert(std::is_same_v<typename[:t:], int>);
}

TEST(SchemaScalarTest, BooleanType)
{
  constexpr auto t = PARSE(R"({ "type": "boolean" })");
  static_assert(std::is_same_v<typename[:t:], bool>);
}

//---------------------------------------------------------------------------//
// Nullable types ("type": [ ..., "null" ]) -> std::optional:                //
//---------------------------------------------------------------------------//
TEST(SchemaNullableTest, NullableInteger)
{
  constexpr auto t = PARSE(R"({ "type": ["integer", "null"] })");
  static_assert(std::is_same_v<typename[:t:], std::optional<int>>);
}

TEST(SchemaNullableTest, NullableString)
{
  constexpr auto t = PARSE(R"({ "type": ["string", "null"] })");
  static_assert(std::is_same_v<typename[:t:], std::optional<std::string_view>>);
}

TEST(SchemaNullableTest, NullableNumber)
{
  constexpr auto t = PARSE(R"({ "type": ["number", "null"] })");
  static_assert(std::is_same_v<typename[:t:], std::optional<double>>);
}

TEST(SchemaNullableTest, NullableBoolean)
{
  constexpr auto t = PARSE(R"({ "type": ["boolean", "null"] })");
  static_assert(std::is_same_v<typename[:t:], std::optional<bool>>);
}

TEST(SchemaNullableTest, NullableArrayField)
{
  constexpr auto t = PARSE(
      R"({
        "type": "object",
        "properties": {
          "list": { "type": ["array", "null"], "items": { "type": "integer" } }
        },
        "required": ["list"]
      })");
  using T = typename[:t:];
  static_assert(
      std::is_same_v<decltype(T{}.list), std::optional<std::vector<int>>>);
}

TEST(SchemaNullableTest, NullableObjectField)
{
  constexpr auto t = PARSE(
      R"({
        "type": "object",
        "properties": {
          "info": { "type": ["object", "null"], "properties": {
                      "x": { "type": "integer" } }, "required": ["x"] }
        },
        "required": ["info"]
      })");
  using T = typename[:t:];
  using Info = std::remove_cvref_t<decltype(T{}.info)>;
  using Nested = std::remove_cvref_t<decltype(T{}.info.value())>;
  static_assert(std::is_same_v<Info, std::optional<Nested>>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(Nested{}.x)>, int>);
}

//---------------------------------------------------------------------------//
// Nested object schemas:                                                    //
//---------------------------------------------------------------------------//
TEST(SchemaNestedTest, ObjectInObject)
{
  constexpr auto t = PARSE(
      R"({
        "type": "object",
        "properties": {
          "id":   { "type": "integer" },
          "meta": { "type": "object", "properties": {
                      "name": { "type": "string" },
                      "flag": { "type": "boolean" }
                    }, "required": ["name", "flag"] }
        },
        "required": ["id", "meta"]
      })");
  using T = typename[:t:];
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.id)>, int>);
  static_assert(
      std::is_same_v<std::remove_cvref_t<decltype(T{}.meta.name)>,
                     std::string_view>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.meta.flag)>,
                               bool>);
}

TEST(SchemaNestedTest, DeeplyNestedObjects)
{
  constexpr auto t = PARSE(
      R"({
        "type": "object",
        "properties": {
          "a": { "type": "object", "properties": {
                   "b": { "type": "object", "properties": {
                            "c": { "type": "integer" }
                          }, "required": ["c"] }
                 }, "required": ["b"] }
        },
        "required": ["a"]
      })");
  using T = typename[:t:];
  static_assert(
      std::is_same_v<std::remove_cvref_t<decltype(T{}.a.b.c)>, int>);
}

TEST(SchemaNestedTest, ArrayOfObjects)
{
  constexpr auto t = PARSE(
      R"({
        "type": "array",
        "items": {
          "type": "object",
          "properties": {
            "x": { "type": "integer" },
            "y": { "type": "string" }
          },
          "required": ["x", "y"]
        }
      })");
  using T = typename[:t:];
  using Elem = typename T::value_type;
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(Elem{}.x)>, int>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(Elem{}.y)>,
                               std::string_view>);
}

TEST(SchemaNestedTest, FixedArrayOfObjects)
{
  constexpr auto t = PARSE(
      R"({
        "type": "array",
        "items": {
          "type": "object",
          "properties": { "x": { "type": "integer" } },
          "required": ["x"]
        },
        "minItems": 3, "maxItems": 3
      })");
  using T = typename[:t:];
  using Elem = typename T::value_type;
  static_assert(std::is_same_v<T, std::array<Elem, 3>>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(Elem{}.x)>, int>);
}

TEST(SchemaNestedTest, TupleOfObjects)
{
  constexpr auto t = PARSE(
      R"({
        "type": "array",
        "prefixItems": [
          { "type": "object", "properties": { "x": { "type": "integer" } },
            "required": ["x"] },
          { "type": "object", "properties": { "y": { "type": "string" } },
            "required": ["y"] }
        ]
      })");
  using T = typename[:t:];
  static_assert(
      std::is_same_v<std::remove_cvref_t<decltype(std::get<0>(T{}).x)>, int>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get<1>(T{}).y)>,
                               std::string_view>);
}

TEST(SchemaNestedTest, Draft07TupleOfObjects)
{
  constexpr auto t = PARSE(
      R"({
        "type": "array",
        "items": [
          { "type": "object", "properties": { "x": { "type": "integer" } },
            "required": ["x"] },
          { "type": "object", "properties": { "y": { "type": "string" } },
            "required": ["y"] }
        ]
      })");
  using T = typename[:t:];
  static_assert(
      std::is_same_v<std::remove_cvref_t<decltype(std::get<0>(T{}).x)>, int>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get<1>(T{}).y)>,
                               std::string_view>);
}

//---------------------------------------------------------------------------//
// Mixed object schema: required, nullable, optional, arrays, nested object: //
//---------------------------------------------------------------------------//
TEST(SchemaMixTest, MixedObjectFields)
{
  constexpr auto t = PARSE(
      R"({
        "type": "object",
        "properties": {
          "id":     { "type": "integer" },
          "name":   { "type": "string" },
          "nick":   { "type": ["string", "null"] },
          "score":  { "type": ["number", "null"] },
          "note":   { "type": ["string", "null"] },
          "tags":   { "type": "array", "items": { "type": "string" } },
          "coords": { "type": "array", "items": { "type": "integer" },
                      "minItems": 2, "maxItems": 2 },
          "meta":   { "type": "object", "properties": {
                        "k": { "type": "integer" } }, "required": ["k"] }
        },
        "required": ["id", "name", "nick", "score", "tags", "coords", "meta"]
      })");
  using T = typename[:t:];
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.id)>, int>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.name)>,
                               std::string_view>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.nick)>,
                               std::optional<std::string_view>>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.score)>,
                               std::optional<double>>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.note)>,
                               std::optional<std::string_view>>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.tags)>,
                               std::vector<std::string_view>>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.coords)>,
                               std::array<int, 2>>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.meta.k)>,
                               int>);

  constexpr auto anns = yjson::FieldAnnots::MkFldAnnots<t>();
  static_assert(anns.size() == 8u);
  // Required fields carry no MayAbsent annotation, even when nullable.
  static_assert(!anns[0].m_may_absent); // id
  static_assert(!anns[1].m_may_absent); // name
  static_assert(!anns[2].m_may_absent); // nick
  static_assert(!anns[3].m_may_absent); // score
  // "note" is absent from "required": nullable + MayAbsent.
  static_assert(anns[4].m_may_absent);
  static_assert(!anns[5].m_may_absent); // tags
  static_assert(!anns[6].m_may_absent); // coords
  static_assert(anns[6].m_static_sz == 2); // fixed-size array
  static_assert(!anns[7].m_may_absent); // meta
}

//---------------------------------------------------------------------------//
// Deeper mix: array of objects whose fields are themselves arrays/tuples:   //
//---------------------------------------------------------------------------//
TEST(SchemaMixTest, ArrayOfObjectsWithArrays)
{
  constexpr auto t = PARSE(
      R"({
        "type": "array",
        "items": {
          "type": "object",
          "properties": {
            "id":     { "type": "integer" },
            "points": { "type": "array", "items": { "type": "number" } },
            "pair":   { "type": "array", "prefixItems": [
                         { "type": "integer" }, { "type": "integer" } ] }
          },
          "required": ["id", "points", "pair"]
        }
      })");
  using T = typename[:t:];
  using Elem = typename T::value_type;
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(Elem{}.id)>, int>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(Elem{}.points)>,
                               std::vector<double>>);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(Elem{}.pair)>,
                               std::tuple<int, int>>);
}

//---------------------------------------------------------------------------//
// minLength keyword -> MinSize annotation:                                  //
//---------------------------------------------------------------------------//
TEST(SchemaMinLengthTest, StringMinLengthGetsMinSize)
{
  constexpr auto t = PARSE(
      R"({
        "type": "object",
        "properties": {
          "code": { "type": "string", "minLength": 5 }
        },
        "required": ["code"]
      })");
  using T = typename[:t:];
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(T{}.code)>,
                               std::string_view>);

  constexpr auto anns = yjson::FieldAnnots::MkFldAnnots<t>();
  static_assert(anns.size() == 1u);
  static_assert(anns[0].m_min_sz == 5);
}

