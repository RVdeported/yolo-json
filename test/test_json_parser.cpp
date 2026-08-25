//===========================================================================//
//                    "test_json_parser.cpp":                                //
//           Tests for Include/json_parser.hpp utilities                     //
//===========================================================================//
#include "Include/json_parser.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

namespace
{

//---------------------------------------------------------------------------//
// "IsCharPtr" type trait:                                                   //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, IsCharPtrTrait)
{
  static_assert(JSONParser::IsCharPtr<char *>);
  static_assert(JSONParser::IsCharPtr<char const *>);
  static_assert(JSONParser::IsCharPtr<const char *>);
  static_assert(!JSONParser::IsCharPtr<char>);
  static_assert(!JSONParser::IsCharPtr<int>);
  static_assert(!JSONParser::IsCharPtr<std::string>);
  SUCCEED();
}

//---------------------------------------------------------------------------//
// "ReadInt":                                                                //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, ReadInt)
{
  char positive[] = "12345234werwdf";
  EXPECT_EQ(JSONParser::ReadInt<int>(positive, positive + 5), 12345);

  char negative[] = "-42rer245";
  EXPECT_EQ(JSONParser::ReadInt<int>(negative, negative + 3), -42);

  char zero[] = "0234er5";
  EXPECT_EQ(JSONParser::ReadInt<long>(zero, zero + 1), 0L);
}

//---------------------------------------------------------------------------//
// "ReadDouble":                                                             //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, ReadDouble)
{
  char decimal[] = "3.1443rtefg";
  EXPECT_DOUBLE_EQ(JSONParser::ReadDouble<double>(decimal, decimal + 4), 3.14);

  char integer[] = "42.0ertg";
  EXPECT_DOUBLE_EQ(JSONParser::ReadDouble<double>(integer, integer + 3), 42.0);

  char negative[] = "-0.554.fgdfg";
  EXPECT_DOUBLE_EQ(JSONParser::ReadDouble<double>(negative, negative + 4),
                   -0.5);
}

//---------------------------------------------------------------------------//
// "ReadNumber":                                                             //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, ReadNumber)
{
  char i[] = "123  ,erte5t3";
  auto [val, next] = JSONParser::ReadNumber<int>(i, i + 7, ',', 3);
  EXPECT_EQ(val, 123);
  EXPECT_EQ(*next, ',');

  char d[] = "3.14  ,45tert";
  EXPECT_DOUBLE_EQ(JSONParser::ReadNumber<double>(d, d + 7, ',', 2).first,
                   3.14);
  //
  char neg[] = "-7|452wtrt";
  EXPECT_EQ(JSONParser::ReadNumber<int>(neg, neg + 7, '|').first, -7);
}

//---------------------------------------------------------------------------//
// "FindVal" (InclSep = true, the default): key carries the ':' separator:   //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, FindValWithSeparator)
{
  char msg[] = R"({"\"price\":333,price:88","price":10})";
  const char * v = JSONParser::FindVal(R"("price":)", msg, msg);
  EXPECT_STREQ(v, "10}");
}

//---------------------------------------------------------------------------//
// "FindVal" (InclSep = false): key is the bare quoted field name:           //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, FindValWithoutSeparator)
{
  char msg[] = R"({"\"price\":333,price:88","price":10})";
  constexpr char key[] = "\"price\"";
  const char * v = JSONParser::FindVal<sizeof(key), false>(key, msg, msg);
  EXPECT_STREQ(v, "10}");
}

//---------------------------------------------------------------------------//
// "FindVal" skips the opening quote of a quoted value (InclSep = false):    //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, FindValQuotedValue)
{
  char msg[] = R"({"name":"foo"})";
  constexpr char key[] = "\"name\"";
  const char * v = JSONParser::FindVal<sizeof(key), false>(key, msg, msg);
  EXPECT_STREQ(v, "foo\"}");
}

//---------------------------------------------------------------------------//
// "FindVal" searches forward from the hint, then wraps to the beginning:    //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, FindValSearchesFromHint)
{
  char msg[] = R"({"a":1,"b":2})";
  char * hint = std::strstr(msg, R"("b")");
  ASSERT_NE(hint, nullptr);
  const char * v = JSONParser::FindVal(R"("a":)", hint, msg);
  EXPECT_STREQ(v, R"(1,"b":2})");
}

//---------------------------------------------------------------------------//
// "FindVal" throws when the key is absent:                                  //
//---------------------------------------------------------------------------//
// TEST(JsonParserTest, FindValNotFoundThrows)
// {
//   char msg[] = R"({"price":10})";
//   EXPECT_THROW(JSONParser::FindVal(R"("missing":)", msg, msg),
//                std::runtime_error);
// }

//---------------------------------------------------------------------------//
// String scanning macros: CMP_STR, SKP_IF_STR, SKP_STR:                     //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, StringScanMacros)
{
  char buf[] = "foobar";
  char * curr = buf;

  EXPECT_TRUE(SKP_IF_STR("foo"));
  EXPECT_STREQ(curr, "bar");

  EXPECT_FALSE(SKP_IF_STR("xyz"));
  EXPECT_STREQ(curr, "bar");

  SKP_STR("bar");
  EXPECT_EQ(curr, buf + 6);

  const char * m = "hello world";
  const char * p = m;
  EXPECT_TRUE(CMP_STR(p, "hello"));
  EXPECT_EQ(p, m + 5);
  EXPECT_FALSE(CMP_STR(p, "xyz"));
  EXPECT_EQ(p, m + 5);
}

//---------------------------------------------------------------------------//
// "GET_BOOL" macro:                                                         //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, GetBoolMacro)
{
  char t[] = "true3455";
  char * curr = t;
  GET_BOOL(b);
  EXPECT_TRUE(b);
  EXPECT_EQ(curr, t + 4);

  char f[] = "falseghfghy";
  curr = f;
  GET_BOOL(b2);
  EXPECT_FALSE(b2);
  EXPECT_EQ(curr, f + 5);
}

//---------------------------------------------------------------------------//
// "GET_STR" macro: 0-terminates the string and advances "curr" past it:     //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, GetStrSimple)
{
  char buf[] = "\"hello\"world";
  char * curr = buf;
  GET_STR(s);
  EXPECT_STREQ(s, "hello");
  EXPECT_EQ(curr, buf + 7);
}

//---------------------------------------------------------------------------//
// "GET_STR" skips over an escaped quote inside the string:                  //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, GetStrWithEscapedQuote)
{
  char buf[] = "\"foo\\\"bar\"tail";
  char * curr = buf;
  GET_STR(s);
  EXPECT_STREQ(s, "foo\\\"bar");
  EXPECT_EQ(curr, buf + 10);
}

//---------------------------------------------------------------------------//
// "GET_STR" handles an escaped backslash inside the string:                 //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, GetStrWithEscapedBackslash)
{
  char buf[] = "\"foo\\\\bar\"tail";
  char * curr = buf;
  GET_STR(s);
  EXPECT_STREQ(s, "foo\\\\bar");
  EXPECT_EQ(curr, buf + 10);
}

//---------------------------------------------------------------------------//
// "GET_STR" handles the empty string:                                       //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, GetStrEmpty)
{
  char buf[] = "\"\"tail";
  char * curr = buf;
  GET_STR(s);
  EXPECT_STREQ(s, "");
  EXPECT_EQ(curr, buf + 2);
}

//---------------------------------------------------------------------------//
// "SkipVal" returns the delimiter pointer for an integral value:            //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, SkipValInt)
{
  char msg[] = "123,45";
  char * v = JSONParser::SkipVal<int>(msg, msg + 6, ',');
  EXPECT_EQ(v, msg + 3);
  EXPECT_EQ(*v, ',');
}

//---------------------------------------------------------------------------//
// "SkipVal" returns the delimiter pointer for a floating-point value:       //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, SkipValDouble)
{
  char msg[] = "3.14}rest";
  char * v = JSONParser::SkipVal<double>(msg, msg + 7, '}');
  EXPECT_EQ(v, msg + 4);
  EXPECT_EQ(*v, '}');
}

//---------------------------------------------------------------------------//
// "SkipVal" handles a negative integral value:                              //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, SkipValNegativeInt)
{
  char msg[] = "-42   |xx";
  char * v = JSONParser::SkipVal<int>(msg, msg + 8, '|');
  EXPECT_EQ(v, msg + 6);
  EXPECT_EQ(*v, '|');
}

//---------------------------------------------------------------------------//
// "SkipVal" skips a string value, returning the pointer past its closing    //
// quote (and 0-terminating the string in place):                            //
//---------------------------------------------------------------------------//
TEST(JsonParserTest, SkipValString)
{
  char msg[] = "\"hello\",42";
  char * v = JSONParser::SkipVal<std::string>(msg, msg + 11, ',');
  EXPECT_EQ(v, msg + 7);
  EXPECT_EQ(*v, ',');
  EXPECT_STREQ(msg + 1, "hello");
}

TEST(JsonParserTest, SkipValStringWithMin)
{
  char msg[] = "\"hello\",42";
  char * v = JSONParser::SkipVal<std::string>(msg, msg + 11, ',', 5);
  EXPECT_EQ(v, msg + 7);
  EXPECT_EQ(*v, ',');
  EXPECT_STREQ(msg + 1, "hello");
}

} // namespace
