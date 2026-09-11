//===========================================================================//
//                    "test_parser.cpp":                                     //
//     End-to-end tests for Include/parser.hpp (yjson::detail::ObjectParser::ParseJson)            //
//===========================================================================//
#include <yolo-json/parser.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
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

struct StringViewFld
{
  int i;
  std::string_view s;
};

struct VectorOfStringViews
{
  std::vector<std::string_view> v;
  int rest;
};

struct Signed
{
  int a;
  double b;
};

struct Booly
{
  bool f;
};

struct Leaf
{
  int v;
};

struct Mid
{
  Leaf leaf;
  int m;
};

struct Deep
{
  int top;
  Mid mid;
};

struct Inner
{
  int x;
  std::string tag;
};

struct Outer
{
  int a;
  Inner in;
  std::string name;
};

struct Escaped
{
  std::string s;
};

struct[[= yjson::NotCompressed{}]] Spaced
{
  int a;
  int b;
  std::string s;
};

struct[[= yjson::NotCompressed{}]] StrMiddle
{

  int a;
  std::string b;
  int c;
};

struct Positioned
{
  [[= yjson::Position{1}]] int second;
  [[= yjson::Position{0}]] int first;
};

struct PosThenDef
{
  [[= yjson::Position{1}]] int aa;
  int bb;
  int cc;
  [[= yjson::Position{0}]] int dd;
};

struct[[= yjson::Alphabetical{false}]] AlphaFwd
{
  int banana;
  int apple;
  int cherry;
};

struct[[= yjson::Alphabetical{true}]] AlphaRev
{
  int apple;
  int banana;
  int cherry;
};

struct Opt
{
  [[= yjson::MayAbsent{}]] std::optional<int> opt;
  int rest;
};

struct Ignored
{
  [[= yjson::Ignore{}]] int skip;
  int keep;
};

struct Sized
{
  [[= yjson::Size{5}]] int fixed;
  int after;
};

struct MinSized
{
  [[= yjson::MinSize{4}]] int mn;
  int after;
};

struct Pair
{
  int a;
  int b;
};

struct[[= yjson::NotCompressed{}]] Nested
{
  int x;
  std::string tag;
};

// A struct exercising several annotations at once: field ordering (Position),
// an optional (MayAbsent), a skipped value (Ignore) and a nested object, all
// wrapped in a NotCompressed (whitespace-tolerant) object.
struct[[= yjson::NotCompressed{}]] Convoluted
{
  [[= yjson::Position{1}]] int b;
  [[= yjson::Position{0}]] int a;
  [[= yjson::MayAbsent{}]] std::optional<int> maybe;
  [[= yjson::Ignore{}]] int skip;
  int tail;
  Nested nested;
};

// DisplayName overrides the JSON key used to read a field.
struct DisplayNamed
{
  [[= yjson::DisplayName{"renamed"}]] int original;
  int normal;
};

// DisplayName combined with MayAbsent on an optional field.
struct DisplayNamedOpt
{
  [[ = yjson::MayAbsent{}, = yjson::DisplayName{"val"} ]] std::optional<int>
      opt;
  int rest;
};

//---------------------------------------------------------------------------//
// RandomOrder (fields may appear in any order):                             //
//---------------------------------------------------------------------------//

// Basic random-order struct: JSON fields are matched by key, not position.
struct[[= yjson::RandomOrder{}]] RandomBasic
{
  int a;
  int b;
  std::string s;
};

// RandomOrder combined with MayAbsent: the optional field may be present at
// any position, or absent entirely.
struct[[= yjson::RandomOrder{}]] RandomOpt
{
  int first;
  [[= yjson::MayAbsent{}]] std::optional<int> maybe;
  int last;
};

// RandomOrder combined with DisplayName and NotCompressed (whitespace).
struct[[ = yjson::RandomOrder{}, = yjson::NotCompressed{} ]] RandomNamed
{
  [[= yjson::DisplayName{"renamed"}]] int original;
  int normal;
  [[= yjson::MayAbsent{}]] std::optional<std::string> maybe;
};

// RandomOrder with a nested object field.
struct[[= yjson::RandomOrder{}]] RandomNested
{
  std::string tag;
  Leaf leaf;
  int x;
};

//---------------------------------------------------------------------------//
// Tuples (std::tuple / std::pair):                                          //
//---------------------------------------------------------------------------//

// A plain tuple field (no annotations).
struct TupleBasic
{
  std::tuple<int, int> t;
};

// A tuple with mixed element types.
struct TupleMixed
{
  std::tuple<int, double, std::string, bool> t;
};

// A std::pair field (parsed like a two-element tuple).
struct TuplePair
{
  std::pair<int, std::string> p;
};

// A nested tuple (a tuple containing another tuple).
struct TupleNested
{
  std::tuple<std::tuple<int, int>, int> t;
};

// A tuple containing a nested object.
struct TupleOfObjs
{
  std::tuple<Leaf, int> t;
};

// A tuple element that is itself optional (may be null).
struct TupleOfOptional
{
  std::tuple<std::optional<int>, int> t;
};

// A tuple field ordered via Position.
struct TuplePositioned
{
  [[= yjson::Position{1}]] int a;
  [[= yjson::Position{0}]] std::tuple<int, int> t;
};

// A tuple field whose JSON key is renamed via DisplayName.
struct TupleDisplayName
{
  [[= yjson::DisplayName{"vec"}]] std::tuple<int, int> t;
  int rest;
};

// A tuple inside an Alphabetical struct (fields sorted by identifier).
struct[[= yjson::Alphabetical{false}]] TupleAlpha
{
  int banana;
  std::tuple<int, int> apple;
  int cherry;
};

// A tuple inside a NotCompressed (whitespace-tolerant) struct.
struct[[= yjson::NotCompressed{}]] TupleSpaced
{
  std::tuple<int, int> t;
  int x;
};

// Several annotations combined with tuples: Position-ordered tuple fields, an
// optional (MayAbsent), a DisplayName override on a pair, and a nested tuple,
// all wrapped in a NotCompressed object.
struct[[= yjson::NotCompressed{}]] TupleConvoluted
{
  [[= yjson::Position{1}]] int b;
  [[= yjson::Position{0}]] std::tuple<int, int> t;
  [[= yjson::MayAbsent{}]] std::optional<int> maybe;
  [[= yjson::DisplayName{"pair"}]] std::pair<int, std::string> p;
  std::tuple<std::tuple<int, int>, int> nested;
};

// An optional tuple field (std::optional<std::tuple<...>>), which may hold a
// value, be null, or (with MayAbsent) be missing entirely.
struct OptionalTuple
{
  [[= yjson::MayAbsent{}]] std::optional<std::tuple<int, int>> t;
  int rest;
};

// An optional tuple with mixed element types.
struct OptionalTupleMixed
{
  [[= yjson::MayAbsent{}]] std::optional<
      std::tuple<int, double, std::string, bool>>
      t;
  int rest;
};

// An optional std::pair field (parsed like a two-element optional tuple).
struct OptionalPair
{
  [[= yjson::MayAbsent{}]] std::optional<std::pair<int, std::string>> p;
  int rest;
};

// An optional tuple whose first element is itself a nested tuple.
struct OptionalTupleNested
{
  [[= yjson::MayAbsent{}]] std::optional<std::tuple<std::tuple<int, int>, int>>
      t;
  int rest;
};

//---------------------------------------------------------------------------//
// Containers (fixed std::array / dynamic std::vector):                      //
//---------------------------------------------------------------------------//

// A fixed-size container (std::array) field.
struct ArrayOfInts
{
  std::array<int, 3> a;
  int rest;
};

// A fixed-size container of strings.
struct ArrayOfStrings
{
  std::array<std::string, 2> a;
  int rest;
};

// A dynamic container (std::vector) field.
struct VectorOfInts
{
  std::vector<int> v;
  int rest;
};

// A dynamic container of strings.
struct VectorOfStrings
{
  std::vector<std::string> v;
  int rest;
};

// A dynamic container whose elements are nested objects.
struct VectorOfObjs
{
  std::vector<Leaf> v;
  int rest;
};

// Another dynamic container type (std::deque) using the push_back path.
struct DequeOfInts
{
  std::deque<int> d;
  int rest;
};

// A nested container of strings.
struct ArrayMixedNested
{
  std::array<std::vector<std::string>, 2> a;
  int rest;
};

struct ArrayOptional
{
  std::optional<std::array<int, 2>> a;
  int rest;
};

struct VectorOptional
{
  std::optional<std::vector<int>> a;
  int rest;
};

//---------------------------------------------------------------------------//
// StaticSize (fixed element count) containers:                              //
//---------------------------------------------------------------------------//

// A dynamic container whose element count is annotated at compile time.
struct StaticVectorOfInts
{
  [[= yjson::StaticSize{3}]] std::vector<int> v;
  int rest;
};

// StaticSize on a container of strings (exercises the string delimiter path).
struct StaticVectorOfStrings
{
  [[= yjson::StaticSize{2}]] std::vector<std::string> v;
  int rest;
};

// StaticSize on a container of nested objects.
struct StaticVectorOfObjs
{
  [[= yjson::StaticSize{2}]] std::vector<Leaf> v;
  int rest;
};

// StaticSize matching a std::array's own element count (unrolled fixed path).
struct StaticArrayOfInts
{
  [[= yjson::StaticSize{3}]] std::array<int, 3> a;
  int rest;
};

// StaticSize on a std::deque (no reserve() -> emplace_back growth path).
struct StaticDequeOfInts
{
  [[= yjson::StaticSize{2}]] std::deque<int> d;
  int rest;
};

// StaticSize inside a RandomOrder object (annotation must survive key-matching).
struct[[= yjson::RandomOrder{}]] RandomStaticVector
{
  [[= yjson::StaticSize{2}]] std::vector<int> v;
  int rest;
};

} // namespace test_types

//---------------------------------------------------------------------------//
// Helpers:                                                                  //
//---------------------------------------------------------------------------//
namespace
{

// Parse an in-memory JSON buffer (which ParseJson mutates in place) into T and
// return the resulting value.
template <typename T> T Parse(std::string json)
{
  auto [rest, value] =
      yjson::detail::ObjectParser::ParseJson<^^T>(json.data(), json.data() + json.size());
  (void)rest;
  return value;
}

} // namespace

//---------------------------------------------------------------------------//
// Basic JSON values:                                                        //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesBasicTypes)
{
  auto v = Parse<test_types::Basic>(R"({"i":42,"d":3.14,"s":"hello"})");
  EXPECT_EQ(v.i, 42);
  EXPECT_DOUBLE_EQ(v.d, 3.14);
  EXPECT_EQ(v.s, "hello");
}

// std::string_view references the input buffer, so the buffer must stay alive
// across the check (the Parse<T> helper would dangle the view by returning by
// value). Parse directly and keep the JSON string in scope.
TEST(ParseJsonTest, ParsesStringViewField)
{
  std::string json = R"({"i":7,"s":"hello"})";
  auto [rest, v] = yjson::detail::ObjectParser::ParseJson<^^test_types::StringViewFld>(
      json.data(), json.data() + json.size());
  (void)rest;
  EXPECT_EQ(v.i, 7);
  EXPECT_EQ(v.s, "hello");
}

// string_view elements inside a dynamic container (exercises the
// container -> element -> base-string dispatch path).
TEST(ParseJsonTest, ParsesVectorOfStringViews)
{
  std::string json = R"({"v":["a","bb"],"rest":3})";
  auto [rest, v] = yjson::detail::ObjectParser::ParseJson<^^test_types::VectorOfStringViews>(
      json.data(), json.data() + json.size());
  (void)rest;
  ASSERT_EQ(v.v.size(), 2u);
  EXPECT_EQ(v.v[0], "a");
  EXPECT_EQ(v.v[1], "bb");
  EXPECT_EQ(v.rest, 3);
}

TEST(ParseJsonTest, ParsesNegativeNumbers)
{
  auto v = Parse<test_types::Signed>(R"({"a":-7,"b":-2.5})");
  EXPECT_EQ(v.a, -7);
  EXPECT_DOUBLE_EQ(v.b, -2.5);
}

// JSON true/false literals are NOT decoded (bool is read as an integer), so
// only numeric 0/1 round-trip through the current parser.
TEST(ParseJsonTest, ParsesBooleanAsIntegerZeroOrOne)
{
  auto t = Parse<test_types::Booly>(R"({"f":true})");
  EXPECT_TRUE(t.f);

  auto f = Parse<test_types::Booly>(R"({"f":false})");
  EXPECT_FALSE(f.f);
}

TEST(ParseJsonTest, ParsesNestedStructs)
{
  auto v =
      Parse<test_types::Outer>(R"({"a":1,"in":{"x":99,"tag":"T"},"name":"n"})");
  EXPECT_EQ(v.a, 1);
  EXPECT_EQ(v.in.x, 99);
  EXPECT_EQ(v.in.tag, "T");
  EXPECT_EQ(v.name, "n");
}

TEST(ParseJsonTest, ParsesDeeplyNestedStructs)
{
  auto v = Parse<test_types::Deep>(R"({"top":1,"mid":{"leaf":{"v":7},"m":8}})");
  EXPECT_EQ(v.top, 1);
  EXPECT_EQ(v.mid.leaf.v, 7);
  EXPECT_EQ(v.mid.m, 8);
}

// Strings are returned verbatim (no unescaping); an escaped quote and escaped
// backslash are preserved as-is.
TEST(ParseJsonTest, ParsesEscapedStringContent)
{
  auto v = Parse<test_types::Escaped>(R"({"s":"a\"b\\c"})");
  EXPECT_EQ(v.s, std::string(R"(a\"b\\c)"));
}

//---------------------------------------------------------------------------//
// Whitespace handling (NotCompressed):                                      //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesNotCompressedWithWhitespace)
{
  // The trailing string is the last field, so its closing quote must be
  // followed immediately by '}' (no whitespace before the delimiter).
  auto v = Parse<test_types::Spaced>(R"({ "a" : 1 , "b" : 2 , "s" : "hi"})");
  EXPECT_EQ(v.a, 1);
  EXPECT_EQ(v.b, 2);
  EXPECT_EQ(v.s, "hi");
}

//---------------------------------------------------------------------------//
// Field ordering annotations:                                               //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesPositionAnnotation)
{
  auto v = Parse<test_types::Positioned>(R"({"first":10,"second":20})");
  EXPECT_EQ(v.first, 10);
  EXPECT_EQ(v.second, 20);
}

TEST(ParseJsonTest, ParsesPositionThenDefinitionOrder)
{
  // dd (Position 0) and aa (Position 1) are placed first; bb and cc follow in
  // definition order.
  auto v = Parse<test_types::PosThenDef>(R"({"dd":1,"aa":2,"bb":3,"cc":4})");
  EXPECT_EQ(v.dd, 1);
  EXPECT_EQ(v.aa, 2);
  EXPECT_EQ(v.bb, 3);
  EXPECT_EQ(v.cc, 4);
}

TEST(ParseJsonTest, ParsesAlphabeticalForward)
{
  auto v = Parse<test_types::AlphaFwd>(R"({"apple":1,"banana":2,"cherry":3})");
  EXPECT_EQ(v.apple, 1);
  EXPECT_EQ(v.banana, 2);
  EXPECT_EQ(v.cherry, 3);
}

TEST(ParseJsonTest, ParsesAlphabeticalReverse)
{
  auto v = Parse<test_types::AlphaRev>(R"({"cherry":3,"banana":2,"apple":1})");
  EXPECT_EQ(v.apple, 1);
  EXPECT_EQ(v.banana, 2);
  EXPECT_EQ(v.cherry, 3);
}

//---------------------------------------------------------------------------//
// Optional fields (MayAbsent):                                              //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesOptionalWhenPresent)
{
  auto v = Parse<test_types::Opt>(R"({"opt":42,"rest":9})");
  ASSERT_TRUE(v.opt.has_value());
  EXPECT_EQ(v.opt.value(), 42);
  EXPECT_EQ(v.rest, 9);
}

//---------------------------------------------------------------------------//
// Optional fields (null given):                                             //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesOptionalWhenNull)
{
  auto v = Parse<test_types::Opt>(R"({"opt":null,"rest":9})");
  ASSERT_FALSE(v.opt.has_value());
  EXPECT_EQ(v.rest, 9);
}

//---------------------------------------------------------------------------//
// Value-shaping annotations (Ignore / Size / MinSize):                      //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesIgnoreAnnotation)
{
  // The ignored field's key must still be present; its value is skipped and
  // the member keeps its default-initialized value.
  auto v = Parse<test_types::Ignored>(R"({"skip":123,"keep":456})");
  EXPECT_EQ(v.skip, 0);
  EXPECT_EQ(v.keep, 456);
}

TEST(ParseJsonTest, ParsesSizeAnnotation)
{
  auto v = Parse<test_types::Sized>(R"({"fixed":12345,"after":9})");
  EXPECT_EQ(v.fixed, 12345);
  EXPECT_EQ(v.after, 9);
}

TEST(ParseJsonTest, ParsesMinSizeAnnotation)
{
  auto v = Parse<test_types::MinSized>(R"({"mn":9999,"after":9})");
  EXPECT_EQ(v.mn, 9999);
  EXPECT_EQ(v.after, 9);
}

//---------------------------------------------------------------------------//
// Convoluted JSON: several annotations combined:                            //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesCombinedAnnotations)
{
  auto v = Parse<test_types::Convoluted>(
      R"({ "a":1 , "b":2 , "maybe":3 , "skip":999 , "tail":5 , "nested" : { "x":7 , "tag":"T"} })");

  EXPECT_EQ(v.a, 1);
  EXPECT_EQ(v.b, 2);
  ASSERT_TRUE(v.maybe.has_value());
  EXPECT_EQ(v.maybe.value(), 3);
  EXPECT_EQ(v.skip, 0); // ignored
  EXPECT_EQ(v.tail, 5);
  EXPECT_EQ(v.nested.x, 7);
  EXPECT_EQ(v.nested.tag, "T");
}

//---------------------------------------------------------------------------//
// DisplayName annotation:                                                   //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesDisplayNameAnnotation)
{
  auto v = Parse<test_types::DisplayNamed>(R"({"renamed":42,"normal":7})");
  EXPECT_EQ(v.original, 42);
  EXPECT_EQ(v.normal, 7);
}

TEST(ParseJsonTest, ParsesDisplayNameOptionalWhenPresent)
{
  auto v = Parse<test_types::DisplayNamedOpt>(R"({"val":11,"rest":5})");
  ASSERT_TRUE(v.opt.has_value());
  EXPECT_EQ(v.opt.value(), 11);
  EXPECT_EQ(v.rest, 5);
}

TEST(ParseJsonTest, ParsesDisplayNameOptionalWhenAbsent)
{
  auto v = Parse<test_types::DisplayNamedOpt>(R"({"rest":5})");
  EXPECT_FALSE(v.opt.has_value());
  EXPECT_EQ(v.rest, 5);
}

//---------------------------------------------------------------------------//
// RandomOrder annotation:                                                   //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesRandomOrderFields)
{
  auto v = Parse<test_types::RandomBasic>(R"({"s":"hi","a":1,"b":2})");
  EXPECT_EQ(v.a, 1);
  EXPECT_EQ(v.b, 2);
  EXPECT_EQ(v.s, "hi");
}

TEST(ParseJsonTest, ParsesRandomOrderReversed)
{
  auto v = Parse<test_types::RandomBasic>(R"({"b":2,"s":"hi","a":1})");
  EXPECT_EQ(v.a, 1);
  EXPECT_EQ(v.b, 2);
  EXPECT_EQ(v.s, "hi");
}

TEST(ParseJsonTest, ParsesRandomOrderOptionalPresent)
{
  auto v = Parse<test_types::RandomOpt>(R"({"last":3,"maybe":7,"first":1})");
  EXPECT_EQ(v.first, 1);
  ASSERT_TRUE(v.maybe.has_value());
  EXPECT_EQ(v.maybe.value(), 7);
  EXPECT_EQ(v.last, 3);
}

TEST(ParseJsonTest, ParsesRandomOrderOptionalAbsent)
{
  auto v = Parse<test_types::RandomOpt>(R"({"last":3,"first":1})");
  EXPECT_EQ(v.first, 1);
  EXPECT_FALSE(v.maybe.has_value());
  EXPECT_EQ(v.last, 3);
}

TEST(ParseJsonTest, ParsesRandomOrderDisplayNameAndWhitespace)
{
  auto v = Parse<test_types::RandomNamed>(
      R"({ "normal" : 7 , "renamed" : 42 , "maybe" : "hi" })");
  EXPECT_EQ(v.original, 42);
  EXPECT_EQ(v.normal, 7);
  ASSERT_TRUE(v.maybe.has_value());
  EXPECT_EQ(v.maybe.value(), "hi");
}

TEST(ParseJsonTest, ParsesRandomOrderNestedObject)
{
  auto v =
      Parse<test_types::RandomNested>(R"({"leaf":{"v":7},"x":9,"tag":"T"})");
  EXPECT_EQ(v.tag, "T");
  EXPECT_EQ(v.leaf.v, 7);
  EXPECT_EQ(v.x, 9);
}

//---------------------------------------------------------------------------//
// Return value (consumed pointer):                                          //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ReturnsPointerPastClosingBrace)
{
  char buf[] = R"({"a":1,"b":2})";
  auto [after, value] =
      yjson::detail::ObjectParser::ParseJson<^^test_types::Pair>(buf, buf + std::strlen(buf));
  EXPECT_EQ(after, buf + std::strlen(buf));
  EXPECT_EQ(value.a, 1);
  EXPECT_EQ(value.b, 2);
}

//---------------------------------------------------------------------------//
// Optional field absent:                                                    //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, OptionalFieldAbsent)
{
  char buf[] = R"({"rest":9})";
  auto [rest, v] =
      yjson::detail::ObjectParser::ParseJson<^^test_types::Opt>(buf, buf + std::strlen(buf));
  (void)rest;
  EXPECT_FALSE(v.opt.has_value());
  EXPECT_EQ(v.rest, 9);
}

//---------------------------------------------------------------------------//
// Boolean literals:                                                         //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, BooleanLiterals)
{
  char t[] = R"({"f":true})";
  auto [rt, vt] = yjson::detail::ObjectParser::ParseJson<^^test_types::Booly>(t, t + std::strlen(t));
  (void)rt;
  EXPECT_TRUE(vt.f);

  char f[] = R"({"f":false})";
  auto [rf, vf] = yjson::detail::ObjectParser::ParseJson<^^test_types::Booly>(f, f + std::strlen(f));
  (void)rf;
  EXPECT_FALSE(vf.f);
}

//---------------------------------------------------------------------------//
// Optional field absent:                                                    //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, StringInMiddle)
{
  char buf[] = R"({"a":3,  "b" : "333"  ,   "c": 56})";
  auto [rest, v] =
      yjson::detail::ObjectParser::ParseJson<^^test_types::StrMiddle>(buf, buf + std::strlen(buf));
  (void)rest;
  EXPECT_EQ(v.a, 3);
  EXPECT_EQ(v.b, "333");
  EXPECT_EQ(v.c, 56);
}

//---------------------------------------------------------------------------//
// Tuples (std::tuple / std::pair):                                          //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesTupleField)
{
  auto v = Parse<test_types::TupleBasic>(R"({"t":[1,2]})");
  EXPECT_EQ(v.t, (std::tuple<int, int>{1, 2}));
}

TEST(ParseJsonTest, ParsesTupleMixedTypes)
{
  auto v = Parse<test_types::TupleMixed>(R"({"t":[1,2.5,"hi",true]})");
  EXPECT_EQ(std::get<0>(v.t), 1);
  EXPECT_DOUBLE_EQ(std::get<1>(v.t), 2.5);
  EXPECT_EQ(std::get<2>(v.t), "hi");
  EXPECT_TRUE(std::get<3>(v.t));
}

TEST(ParseJsonTest, ParsesPairField)
{
  auto v = Parse<test_types::TuplePair>(R"({"p":[7,"hi"]})");
  EXPECT_EQ(v.p.first, 7);
  EXPECT_EQ(v.p.second, "hi");
}

TEST(ParseJsonTest, ParsesNestedTuple)
{
  auto v = Parse<test_types::TupleNested>(R"({"t":[[1,2],3]})");
  EXPECT_EQ(std::get<0>(std::get<0>(v.t)), 1);
  EXPECT_EQ(std::get<1>(std::get<0>(v.t)), 2);
  EXPECT_EQ(std::get<1>(v.t), 3);
}

TEST(ParseJsonTest, ParsesTupleOfObjects)
{
  auto v = Parse<test_types::TupleOfObjs>(R"({"t":[{"v":7},9]})");
  EXPECT_EQ(std::get<0>(v.t).v, 7);
  EXPECT_EQ(std::get<1>(v.t), 9);
}

// A tuple element that is std::optional may hold a value or be null.
TEST(ParseJsonTest, ParsesTupleOfOptionalElement)
{
  auto v = Parse<test_types::TupleOfOptional>(R"({"t":[7,9]})");
  ASSERT_TRUE(std::get<0>(v.t).has_value());
  EXPECT_EQ(std::get<0>(v.t).value(), 7);
  EXPECT_EQ(std::get<1>(v.t), 9);
}

TEST(ParseJsonTest, ParsesTupleOfOptionalElementNull)
{
  auto v = Parse<test_types::TupleOfOptional>(R"({"t":[null,9]})");
  EXPECT_FALSE(std::get<0>(v.t).has_value());
  EXPECT_EQ(std::get<1>(v.t), 9);
}

//---------------------------------------------------------------------------//
// Tuples combined with annotations:                                         //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesTupleWithPosition)
{
  auto v = Parse<test_types::TuplePositioned>(R"({"t":[1,2],"a":3})");
  EXPECT_EQ(v.t, (std::tuple<int, int>{1, 2}));
  EXPECT_EQ(v.a, 3);
}

TEST(ParseJsonTest, ParsesTupleWithDisplayName)
{
  auto v = Parse<test_types::TupleDisplayName>(R"({"vec":[1,2],"rest":5})");
  EXPECT_EQ(v.t, (std::tuple<int, int>{1, 2}));
  EXPECT_EQ(v.rest, 5);
}

TEST(ParseJsonTest, ParsesTupleInAlphabeticalStruct)
{
  auto v =
      Parse<test_types::TupleAlpha>(R"({"apple":[1,2],"banana":3,"cherry":4})");
  EXPECT_EQ(v.apple, (std::tuple<int, int>{1, 2}));
  EXPECT_EQ(v.banana, 3);
  EXPECT_EQ(v.cherry, 4);
}

// A tuple field in the middle of a NotCompressed struct, with whitespace on
// both sides of every delimiter.
TEST(ParseJsonTest, ParsesNotCompressedTuple)
{
  auto v = Parse<test_types::TupleSpaced>(R"({ "t" : [ 1 , 2 ] , "x" : 3 })");
  EXPECT_EQ(v.t, (std::tuple<int, int>{1, 2}));
  EXPECT_EQ(v.x, 3);
}

TEST(ParseJsonTest, ParsesTupleConvoluted)
{
  auto v = Parse<test_types::TupleConvoluted>(
      R"({ "t" : [1,2] , "b" : 3 , "maybe" : 4 , "pair" : [7,"hi"] , "nested" : [ [5,6] , 8 ] })");

  EXPECT_EQ(v.t, (std::tuple<int, int>{1, 2}));
  EXPECT_EQ(v.b, 3);
  ASSERT_TRUE(v.maybe.has_value());
  EXPECT_EQ(v.maybe.value(), 4);
  EXPECT_EQ(v.p.first, 7);
  EXPECT_EQ(v.p.second, "hi");
  EXPECT_EQ(std::get<0>(std::get<0>(v.nested)), 5);
  EXPECT_EQ(std::get<1>(std::get<0>(v.nested)), 6);
  EXPECT_EQ(std::get<1>(v.nested), 8);
}

//---------------------------------------------------------------------------//
// Optional tuples (std::optional<std::tuple<...>> / std::pair):             //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesOptionalTupleWhenPresent)
{
  auto v = Parse<test_types::OptionalTuple>(R"({"t":[1,2],"rest":9})");
  ASSERT_TRUE(v.t.has_value());
  EXPECT_EQ(v.t.value(), (std::tuple<int, int>{1, 2}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesOptionalTupleWhenNull)
{
  auto v = Parse<test_types::OptionalTuple>(R"({"t":null,"rest":9})");
  EXPECT_FALSE(v.t.has_value());
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesOptionalTupleWhenAbsent)
{
  auto v = Parse<test_types::OptionalTuple>(R"({"rest":9})");
  EXPECT_FALSE(v.t.has_value());
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesOptionalTupleMixedTypes)
{
  auto v = Parse<test_types::OptionalTupleMixed>(
      R"({"t":[1,2.5,"hi",true],"rest":9})");
  ASSERT_TRUE(v.t.has_value());
  EXPECT_EQ(std::get<0>(v.t.value()), 1);
  EXPECT_DOUBLE_EQ(std::get<1>(v.t.value()), 2.5);
  EXPECT_EQ(std::get<2>(v.t.value()), "hi");
  EXPECT_TRUE(std::get<3>(v.t.value()));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesOptionalPairWhenPresent)
{
  auto v = Parse<test_types::OptionalPair>(R"({"p":[7,"hi"],"rest":9})");
  ASSERT_TRUE(v.p.has_value());
  EXPECT_EQ(v.p.value().first, 7);
  EXPECT_EQ(v.p.value().second, "hi");
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesOptionalTupleNested)
{
  auto v =
      Parse<test_types::OptionalTupleNested>(R"({"t":[[1,2],3],"rest":9})");
  ASSERT_TRUE(v.t.has_value());
  EXPECT_EQ(std::get<0>(std::get<0>(v.t.value())), 1);
  EXPECT_EQ(std::get<1>(std::get<0>(v.t.value())), 2);
  EXPECT_EQ(std::get<1>(v.t.value()), 3);
  EXPECT_EQ(v.rest, 9);
}

//---------------------------------------------------------------------------//
// Containers (fixed std::array / dynamic std::vector):                      //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesFixedArrayOfInts)
{
  auto v = Parse<test_types::ArrayOfInts>(R"({"a":[1,2,3],"rest":9})");
  EXPECT_EQ(v.a, (std::array<int, 3>{1, 2, 3}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesFixedArrayOfStrings)
{
  auto v = Parse<test_types::ArrayOfStrings>(R"({"a":["x","yz"],"rest":9})");
  EXPECT_EQ(v.a, (std::array<std::string, 2>{"x", "yz"}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesDynamicVectorOfInts)
{
  auto v = Parse<test_types::VectorOfInts>(R"({"v":[4,5,6],"rest":9})");
  EXPECT_EQ(v.v, (std::vector<int>{4, 5, 6}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesDynamicVectorOfStrings)
{
  auto v = Parse<test_types::VectorOfStrings>(R"({"v":["a","b"],"rest":9})");
  EXPECT_EQ(v.v, (std::vector<std::string>{"a", "b"}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesDynamicVectorOfObjects)
{
  auto v = Parse<test_types::VectorOfObjs>(
      R"({"v":[{"v":1},{"v":2}],"rest":9})");
  ASSERT_EQ(v.v.size(), 2u);
  EXPECT_EQ(v.v[0].v, 1);
  EXPECT_EQ(v.v[1].v, 2);
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesDynamicDequeOfInts)
{
  auto v = Parse<test_types::DequeOfInts>(R"({"d":[7,8],"rest":9})");
  EXPECT_EQ(v.d, (std::deque<int>{7, 8}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesMixedArrayNested)
{
  auto v = Parse<test_types::ArrayMixedNested>(
      R"({"a":[["x","yz"],["aaa","bbb","ccc"]],"rest":9})");
  EXPECT_EQ(v.a, (std::array<std::vector<std::string>, 2>
        {{{"x","yz"},{"aaa","bbb","ccc"}}}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesOptionalArray)
{
  auto v = Parse<test_types::ArrayOptional>(
      R"({"a":[22,33],"rest":9})");
  EXPECT_EQ(v.a.value(), (std::array<int, 2>{22,33}));
  EXPECT_EQ(v.rest, 9);
}


TEST(ParseJsonTest, ParsesOptionalArrayNull)
{
  auto v = Parse<test_types::ArrayOptional>(
      R"({"a":null,"rest":9})");
  EXPECT_FALSE(v.a.has_value());
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesOptionalVector)
{
  auto v = Parse<test_types::VectorOptional>(
      R"({"a":[22,33],"rest":9})");
  EXPECT_EQ(v.a.value(), (std::vector<int>{22,33}));
  EXPECT_EQ(v.rest, 9);
}


TEST(ParseJsonTest, ParsesOptionalVectorNull)
{
  auto v = Parse<test_types::VectorOptional>(
      R"({"a":null,"rest":9})");
  EXPECT_FALSE(v.a.has_value());
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesEmptyContainer)
{
  auto v = Parse<test_types::VectorOfInts>(
      R"({"v":[],"rest":9})");
  EXPECT_TRUE(v.v.empty());
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesNotFullArray)
{
  auto v = Parse<test_types::ArrayOfInts>(
      R"({"a":[1],"rest":9})");
  EXPECT_EQ(v.a, (std::array<int,3>{1,0,0}));
  EXPECT_EQ(v.rest, 9);
}

//---------------------------------------------------------------------------//
// StaticSize (fixed element count) containers:                              //
//---------------------------------------------------------------------------//
TEST(ParseJsonTest, ParsesStaticSizeVectorOfInts)
{
  auto v = Parse<test_types::StaticVectorOfInts>(R"({"v":[1,2,3],"rest":9})");
  EXPECT_EQ(v.v, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesStaticSizeVectorOfStrings)
{
  auto v = Parse<test_types::StaticVectorOfStrings>(R"({"v":["a","b"],"rest":9})");
  EXPECT_EQ(v.v, (std::vector<std::string>{"a", "b"}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesStaticSizeVectorOfObjects)
{
  auto v =
      Parse<test_types::StaticVectorOfObjs>(R"({"v":[{"v":1},{"v":2}],"rest":9})");
  ASSERT_EQ(v.v.size(), 2u);
  EXPECT_EQ(v.v[0].v, 1);
  EXPECT_EQ(v.v[1].v, 2);
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesStaticSizeArrayOfInts)
{
  auto v = Parse<test_types::StaticArrayOfInts>(R"({"a":[4,5,6],"rest":9})");
  EXPECT_EQ(v.a, (std::array<int, 3>{4, 5, 6}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesStaticSizeDequeOfInts)
{
  auto v = Parse<test_types::StaticDequeOfInts>(R"({"d":[7,8],"rest":9})");
  EXPECT_EQ(v.d, (std::deque<int>{7, 8}));
  EXPECT_EQ(v.rest, 9);
}

TEST(ParseJsonTest, ParsesStaticSizeInRandomOrderObject)
{
  auto v =
      Parse<test_types::RandomStaticVector>(R"({"rest":9,"v":[7,8]})");
  EXPECT_EQ(v.v, (std::vector<int>{7, 8}));
  EXPECT_EQ(v.rest, 9);
}

//===========================================================================//
// Known limitations (documented; disabled until fixed):                     //
//===========================================================================//
//
// The reflection parser is a work in progress. The following behaviors are
// known to be missing or broken in the current state and are disabled here so
// they can be enabled once implemented:
//
//  * char fields: treated as an integer, so quoted character values do not
//    round-trip.
//  * std::variant: not supported.
