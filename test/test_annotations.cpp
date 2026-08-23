//===========================================================================//
//                    "test_annotations.cpp":                                 //
//      Tests for struct/field annotation extraction and field ordering       //
//===========================================================================//
#include "Include/parser.hpp"
#include "Include/utils.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <string_view>

//---------------------------------------------------------------------------//
// Test structs:                                                             //
//---------------------------------------------------------------------------//
namespace test_types
{

// No annotations: every struct/field annotation falls back to its default.
struct Plain
{
  int a;
  int b;
};

struct[[= yjson::Alphabetical{false}]] AlphaFwd
{
  int a;
  int b;
};

struct[[= yjson::Alphabetical{true}]] AlphaRev
{
  int apple;
  int banana;
  int cherry;
};

struct[[ = yjson::NotCompressed{}, = yjson::Alphabetical{false} ]] NotComp
{
  int a;
};

// One field per supported field-annotation, plus a plain fallback field.
struct AnnotatedFields
{
  [[= yjson::Position{3}]] int pos_field;
  [[= yjson::Size{5}]] int size_field;
  [[= yjson::Ignore{}]] int ignored_field;
  [[= yjson::DisplayName{"Renamed"}]] int named_field;
  int plain_field;
};

// Position annotations only: exact slots, no gaps.
struct PosOnly
{
  [[= yjson::Position{2}]] int a;
  [[= yjson::Position{0}]] int b;
  [[= yjson::Position{1}]] int c;
};

// Position annotations + fall back to structure-definition order.
struct PosThenDef
{
  [[= yjson::Position{1}]] int aa;
  int bb;
  int cc;
  [[= yjson::Position{0}]] int dd;
};

// Position annotations + fall back to alphabetical order.
struct[[= yjson::Alphabetical{false}]] PosThenAlpha
{
  [[= yjson::Position{0}]] int mmm;
  int aaa;
  int ccc;
  int bbb;
};

// Alphabetical only (no Position annotations).
struct[[= yjson::Alphabetical{false}]] AlphaOnly
{
  int banana;
  int apple;
  int cherry;
};

// DisplayName drives the alphabetical sort key when present.
struct[[= yjson::Alphabetical{false}]] DispNameSort
{
  [[= yjson::DisplayName{"zzz"}]] int a;
  int b;
  [[= yjson::DisplayName{"aaa"}]] int c;
};

// A Position outside the field range is ignored -> definition order.
struct OutOfRange
{
  [[= yjson::Position{10}]] int a;
  int b;
  int c;
};

} // namespace test_types

//---------------------------------------------------------------------------//
// Struct annotation extraction:                                             //
//---------------------------------------------------------------------------//
TEST(StructAnnotationsTest, Defaults)
{
  constexpr auto a = yjson::StructAnnots::MkStrAnnots<test_types::Plain>();
  EXPECT_FALSE(a.m_alphabetical.has_value());
  EXPECT_TRUE(a.m_compressed);
}

TEST(StructAnnotationsTest, AlphabeticalForward)
{
  constexpr auto a = yjson::StructAnnots::MkStrAnnots<test_types::AlphaFwd>();
  ASSERT_TRUE(a.m_alphabetical.has_value());
  EXPECT_FALSE(a.m_alphabetical.value());
  EXPECT_TRUE(a.m_compressed);
}

TEST(StructAnnotationsTest, AlphabeticalReverse)
{
  constexpr auto a = yjson::StructAnnots::MkStrAnnots<test_types::AlphaRev>();
  ASSERT_TRUE(a.m_alphabetical.has_value());
  EXPECT_TRUE(a.m_alphabetical.value());
}

TEST(StructAnnotationsTest, NotCompressed)
{
  constexpr auto a = yjson::StructAnnots::MkStrAnnots<test_types::NotComp>();
  EXPECT_TRUE(a.m_alphabetical.has_value());
  EXPECT_FALSE(a.m_alphabetical.value());
  EXPECT_FALSE(a.m_compressed);
}

//---------------------------------------------------------------------------//
// Field annotation extraction:                                              //
//---------------------------------------------------------------------------//
TEST(FieldAnnotationsTest, Extraction)
{
  constexpr auto f =
      yjson::FieldAnnots::MkFldAnnots<test_types::AnnotatedFields>();
  ASSERT_EQ(f.size(), 5u);

  // [[= Position{3}]] int pos_field;
  EXPECT_EQ(f[0].m_pos, 3);
  EXPECT_EQ(f[0].m_sz, -1);
  EXPECT_FALSE(f[0].m_ignore);
  EXPECT_EQ(f[0].m_disp_name[0], '\0');

  // [[= Size{5}]] int size_field;
  EXPECT_EQ(f[1].m_pos, -1);
  EXPECT_EQ(f[1].m_sz, 5);
  EXPECT_FALSE(f[1].m_ignore);
  EXPECT_EQ(f[1].m_disp_name[0], '\0');

  // [[= Ignore{}]] int ignored_field;
  EXPECT_EQ(f[2].m_pos, -1);
  EXPECT_EQ(f[2].m_sz, -1);
  EXPECT_TRUE(f[2].m_ignore);
  EXPECT_EQ(f[2].m_disp_name[0], '\0');

  // [[= DisplayName{"Renamed"}]] int named_field;
  EXPECT_EQ(f[3].m_pos, -1);
  EXPECT_EQ(f[3].m_sz, -1);
  EXPECT_FALSE(f[3].m_ignore);
  EXPECT_STREQ(f[3].m_disp_name.data(), "Renamed");

  // int plain_field;  (no annotations)
  EXPECT_EQ(f[4].m_pos, -1);
  EXPECT_EQ(f[4].m_sz, -1);
  EXPECT_FALSE(f[4].m_ignore);
  EXPECT_EQ(f[4].m_disp_name[0], '\0');
}

//---------------------------------------------------------------------------//
// Field ordering:                                                           //
//---------------------------------------------------------------------------//
TEST(FieldSortingTest, PositionOnly)
{
  constexpr auto order = yjson::GetOrderedField<test_types::PosOnly>();
  constexpr std::array<int, 3> expected{1, 2, 0};
  static_assert(order == expected);
  EXPECT_EQ(order, expected);
}

TEST(FieldSortingTest, PositionThenDefinitionOrder)
{
  constexpr auto order = yjson::GetOrderedField<test_types::PosThenDef>();
  constexpr std::array<int, 4> expected{3, 0, 1, 2};
  static_assert(order == expected);
  EXPECT_EQ(order, expected);
}

TEST(FieldSortingTest, PositionThenAlphabetical)
{
  constexpr auto order = yjson::GetOrderedField<test_types::PosThenAlpha>();
  constexpr std::array<int, 4> expected{0, 1, 3, 2};
  static_assert(order == expected);
  EXPECT_EQ(order, expected);
}

TEST(FieldSortingTest, AlphabeticalOnly)
{
  constexpr auto order = yjson::GetOrderedField<test_types::AlphaOnly>();
  constexpr std::array<int, 3> expected{1, 0, 2};
  static_assert(order == expected);
  EXPECT_EQ(order, expected);
}

TEST(FieldSortingTest, AlphabeticalReverse)
{
  constexpr auto order = yjson::GetOrderedField<test_types::AlphaRev>();
  constexpr std::array<int, 3> expected{2, 1, 0};
  static_assert(order == expected);
  EXPECT_EQ(order, expected);
}

TEST(FieldSortingTest, DisplayNameDrivesOrder)
{
  constexpr auto order = yjson::GetOrderedField<test_types::DispNameSort>();
  constexpr std::array<int, 3> expected{2, 1, 0};
  static_assert(order == expected);
  EXPECT_EQ(order, expected);

  // SortFieldsAlphabetically must agree with the fallback used by
  // GetOrderedField for the same struct.
  constexpr auto sorted =
      yjson::SortFieldsAlphabetically<test_types::DispNameSort>();
  static_assert(sorted == expected);
  EXPECT_EQ(sorted, expected);
}

TEST(FieldSortingTest, OutOfRangePositionFallsBackToDefinitionOrder)
{
  constexpr auto order = yjson::GetOrderedField<test_types::OutOfRange>();
  constexpr std::array<int, 3> expected{0, 1, 2};
  static_assert(order == expected);
  EXPECT_EQ(order, expected);
}
