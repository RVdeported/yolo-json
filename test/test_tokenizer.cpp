//===========================================================================//
//                    "test_tokenizer.cpp":                                  //
//      Tests for Include/tokenizer.hpp (yjson::Tokenize)                    //
//===========================================================================//
#include <yolo-json/tokenizer.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <meta>
#include <string_view>

// Feed a JSON schema literal to yjson::Tokenize as a constant-string
// reflection. This must be a macro: std::meta::reflect_constant_string needs
// the literal directly so it stays a constant expression (a helper function's
// parameter would not be one).
#define TOKENS(schema) \
  yjson::Tokenize<std::meta::reflect_constant_string(schema)>()

//---------------------------------------------------------------------------//
// Compile-time sanity (static_assert):                                      //
//---------------------------------------------------------------------------//

// A minimal schema, tokenised at compile time.
constexpr auto kEmptyTokens = TOKENS(R"()");
static_assert(kEmptyTokens.size() == 0);

constexpr auto kStructTokens = TOKENS(R"({}[]:,)");
static_assert(kStructTokens.size() == 6);
static_assert(kStructTokens[0].type == yjson::TokenType::ObjectBegin);
static_assert(kStructTokens[1].type == yjson::TokenType::ObjectEnd);
static_assert(kStructTokens[2].type == yjson::TokenType::ArrayBegin);
static_assert(kStructTokens[3].type == yjson::TokenType::ArrayEnd);
static_assert(kStructTokens[4].type == yjson::TokenType::Colon);
static_assert(kStructTokens[5].type == yjson::TokenType::Comma);

//---------------------------------------------------------------------------//
// TokenTypeName:                                                            //
//---------------------------------------------------------------------------//
TEST(TokenizerTest, TypeName)
{
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::ObjectBegin), "ObjectBegin");
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::ObjectEnd), "ObjectEnd");
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::ArrayBegin), "ArrayBegin");
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::ArrayEnd), "ArrayEnd");
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::Colon), "Colon");
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::Comma), "Comma");
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::String), "String");
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::Number), "Number");
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::True), "True");
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::False), "False");
  EXPECT_EQ(yjson::TokenTypeName(yjson::TokenType::Null), "Null");
}

//---------------------------------------------------------------------------//
// Structural tokens:                                                        //
//---------------------------------------------------------------------------//
TEST(TokenizerTest, Structural)
{
  constexpr auto toks = TOKENS(R"({}[]:,)");
  ASSERT_EQ(toks.size(), 6u);

  const char * expected_text[] = {"{", "}", "[", "]", ":", ","};
  for (std::size_t i = 0; i < 6; ++i)
    EXPECT_EQ(toks[i].text, std::string_view{expected_text[i]});
}

//---------------------------------------------------------------------------//
// Scalars (true / false / null):                                            //
//---------------------------------------------------------------------------//
TEST(TokenizerTest, Scalars)
{
  constexpr auto toks = TOKENS(R"(true false null)");
  ASSERT_EQ(toks.size(), 3u);

  EXPECT_EQ(toks[0].type, yjson::TokenType::True);
  EXPECT_EQ(toks[0].text, std::string_view{"true"});
  EXPECT_EQ(toks[1].type, yjson::TokenType::False);
  EXPECT_EQ(toks[1].text, std::string_view{"false"});
  EXPECT_EQ(toks[2].type, yjson::TokenType::Null);
  EXPECT_EQ(toks[2].text, std::string_view{"null"});
}

//---------------------------------------------------------------------------//
// Numbers:                                                                  //
//---------------------------------------------------------------------------//
TEST(TokenizerTest, Numbers)
{
  constexpr auto toks = TOKENS(R"(-12 3.14 2e5 -0.5E-3 0)");
  ASSERT_EQ(toks.size(), 5u);

  for (const auto & t : toks)
    EXPECT_EQ(t.type, yjson::TokenType::Number);

  EXPECT_EQ(toks[0].text, std::string_view{"-12"});
  EXPECT_EQ(toks[1].text, std::string_view{"3.14"});
  EXPECT_EQ(toks[2].text, std::string_view{"2e5"});
  EXPECT_EQ(toks[3].text, std::string_view{"-0.5E-3"});
  EXPECT_EQ(toks[4].text, std::string_view{"0"});
}

//---------------------------------------------------------------------------//
// Object with fields (full token sequence):                                 //
//---------------------------------------------------------------------------//
TEST(TokenizerTest, Object)
{
  constexpr auto toks = TOKENS(R"({"name":"John","age":30})");

  // { "name" : "John" , "age" : 30 }
  ASSERT_EQ(toks.size(), 9u);

  const std::array<std::pair<yjson::TokenType, std::string_view>, 9> expected{{
      {yjson::TokenType::ObjectBegin, "{"},
      {yjson::TokenType::String, R"("name")"},
      {yjson::TokenType::Colon, ":"},
      {yjson::TokenType::String, R"("John")"},
      {yjson::TokenType::Comma, ","},
      {yjson::TokenType::String, R"("age")"},
      {yjson::TokenType::Colon, ":"},
      {yjson::TokenType::Number, "30"},
      {yjson::TokenType::ObjectEnd, "}"},
  }};

  for (std::size_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(toks[i].type, expected[i].first) << "index " << i;
    EXPECT_EQ(toks[i].text, expected[i].second) << "index " << i;
  }
}

//---------------------------------------------------------------------------//
// Whitespace tolerance (pretty-printed input):                              //
//---------------------------------------------------------------------------//
TEST(TokenizerTest, WhitespaceTolerant)
{
  constexpr auto toks = TOKENS(R"(
{
  "type" : "object",
  "properties" : {
    "name" : { "type" : "string" }
  }
})");

  // { "type" : "object" , "properties" : { "name" : { "type" : "string" } } }
  ASSERT_EQ(toks.size(), 17u);

  const std::array<std::pair<yjson::TokenType, std::string_view>, 17> expected{{
      {yjson::TokenType::ObjectBegin, "{"},
      {yjson::TokenType::String, R"("type")"},
      {yjson::TokenType::Colon, ":"},
      {yjson::TokenType::String, R"("object")"},
      {yjson::TokenType::Comma, ","},
      {yjson::TokenType::String, R"("properties")"},
      {yjson::TokenType::Colon, ":"},
      {yjson::TokenType::ObjectBegin, "{"},
      {yjson::TokenType::String, R"("name")"},
      {yjson::TokenType::Colon, ":"},
      {yjson::TokenType::ObjectBegin, "{"},
      {yjson::TokenType::String, R"("type")"},
      {yjson::TokenType::Colon, ":"},
      {yjson::TokenType::String, R"("string")"},
      {yjson::TokenType::ObjectEnd, "}"},
      {yjson::TokenType::ObjectEnd, "}"},
      {yjson::TokenType::ObjectEnd, "}"},
  }};

  for (std::size_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(toks[i].type, expected[i].first) << "index " << i;
    EXPECT_EQ(toks[i].text, expected[i].second) << "index " << i;
  }
}

//---------------------------------------------------------------------------//
// Escaped strings:                                                          //
//---------------------------------------------------------------------------//
TEST(TokenizerTest, EscapedString)
{
  // The JSON source contains a backslash-escaped quote and newline.
  constexpr auto toks = TOKENS(R"("a\nb\"c")");
  ASSERT_EQ(toks.size(), 1u);

  EXPECT_EQ(toks[0].type, yjson::TokenType::String);
  EXPECT_EQ(toks[0].text, std::string_view{R"("a\nb\"c")"});
}

//---------------------------------------------------------------------------//
// Nested arrays / objects:                                                  //
//---------------------------------------------------------------------------//
TEST(TokenizerTest, Nested)
{
  constexpr auto toks = TOKENS(R"([[1,2],[3,[4]]])");

  // [ [ 1 , 2 ] , [ 3 , [ 4 ] ] ]
  ASSERT_EQ(toks.size(), 15u);

  const std::array<std::pair<yjson::TokenType, std::string_view>, 15> expected{{
      {yjson::TokenType::ArrayBegin, "["},
      {yjson::TokenType::ArrayBegin, "["},
      {yjson::TokenType::Number, "1"},
      {yjson::TokenType::Comma, ","},
      {yjson::TokenType::Number, "2"},
      {yjson::TokenType::ArrayEnd, "]"},
      {yjson::TokenType::Comma, ","},
      {yjson::TokenType::ArrayBegin, "["},
      {yjson::TokenType::Number, "3"},
      {yjson::TokenType::Comma, ","},
      {yjson::TokenType::ArrayBegin, "["},
      {yjson::TokenType::Number, "4"},
      {yjson::TokenType::ArrayEnd, "]"},
      {yjson::TokenType::ArrayEnd, "]"},
      {yjson::TokenType::ArrayEnd, "]"},
  }};

  for (std::size_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(toks[i].type, expected[i].first) << "index " << i;
    EXPECT_EQ(toks[i].text, expected[i].second) << "index " << i;
  }
}

//---------------------------------------------------------------------------//
// A realistic JSON Schema fragment:                                         //
//---------------------------------------------------------------------------//
TEST(TokenizerTest, SchemaFragment)
{
  constexpr auto toks = TOKENS(R"(
{
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "age":  { "type": "integer", "minimum": 0 }
  },
  "required": ["name", "age"]
})");

  // Spot-check a few tokens; full correctness is covered above.
  EXPECT_EQ(toks[0].type, yjson::TokenType::ObjectBegin);
  EXPECT_EQ(toks[1].text, std::string_view{R"("type")"});
  EXPECT_EQ(toks[3].text, std::string_view{R"("object")"});
  EXPECT_EQ(toks.back().type, yjson::TokenType::ObjectEnd);

  // Every structural token must alternate correctly around the "minimum" key.
  bool saw_minimum = false;
  for (std::size_t i = 0; i + 2 < toks.size(); ++i)
  {
    if (toks[i].text == std::string_view{R"("minimum")"})
    {
      saw_minimum = true;
      EXPECT_EQ(toks[i + 1].type, yjson::TokenType::Colon);
      EXPECT_EQ(toks[i + 2].type, yjson::TokenType::Number);
      EXPECT_EQ(toks[i + 2].text, std::string_view{"0"});
    }
  }
  EXPECT_TRUE(saw_minimum);
}
