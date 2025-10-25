#include <gtest/gtest.h>
#include "StringFilters.h"
#include <xmorphy/morph/WordForm.h>

using namespace X;
using namespace StringFilters;

// -------------------------------------------------------------
// 1. Tests for IsOnlyPunctuationOrDigits
// -------------------------------------------------------------
TEST(StringFilters, OnlyDigitsOrPunct_Positive)
{
    EXPECT_TRUE(IsOnlyPunctuationOrDigits("123"));
    EXPECT_TRUE(IsOnlyPunctuationOrDigits("!!!"));
    EXPECT_TRUE(IsOnlyPunctuationOrDigits("123?!.,"));
}

TEST(StringFilters, OnlyDigitsOrPunct_Negative)
{
    EXPECT_FALSE(IsOnlyPunctuationOrDigits("abc"));
    EXPECT_FALSE(IsOnlyPunctuationOrDigits("!@abc"));
    EXPECT_FALSE(IsOnlyPunctuationOrDigits("12а"));
    EXPECT_FALSE(IsOnlyPunctuationOrDigits("тест"));
    EXPECT_FALSE(IsOnlyPunctuationOrDigits("Hello!"));
}

// -------------------------------------------------------------
// 2. Tests for HasNonCyrillicOrSpecialUnicode
// -------------------------------------------------------------
TEST(StringFilters, HasNonCyrillicOrSpecialUnicode_Positive)
{
    EXPECT_TRUE(HasNonCyrillicOrSpecialUnicode("123"));
    EXPECT_TRUE(HasNonCyrillicOrSpecialUnicode("🙂"));
    EXPECT_TRUE(HasNonCyrillicOrSpecialUnicode("你好"));
    EXPECT_TRUE(HasNonCyrillicOrSpecialUnicode("مرحبا"));
    EXPECT_TRUE(HasNonCyrillicOrSpecialUnicode("देवनागरी"));
    EXPECT_TRUE(HasNonCyrillicOrSpecialUnicode("x+y=z"));
}

TEST(StringFilters, HasNonCyrillicOrSpecialUnicode_Negative)
{
    EXPECT_FALSE(HasNonCyrillicOrSpecialUnicode("Привет мир"));
    EXPECT_FALSE(HasNonCyrillicOrSpecialUnicode("анализ данных"));
}

// -------------------------------------------------------------
// 3. Tests for ShouldBeFiltered
// -------------------------------------------------------------
TEST(StringFilters, ShouldBeFiltered_SymbolBased)
{
    EXPECT_TRUE(ShouldBeFiltered("%%%"));
    EXPECT_TRUE(ShouldBeFiltered("текст_with_underscores"));
    EXPECT_TRUE(ShouldBeFiltered("тест#$"));
}

TEST(StringFilters, ShouldBeFiltered_LongLatin)
{
    std::string longLatin(30, 'a'); // 30 symbols of 'a'
    EXPECT_TRUE(ShouldBeFiltered(longLatin));
}

TEST(StringFilters, ShouldBeFiltered_EmojiOrDigits)
{
    EXPECT_TRUE(ShouldBeFiltered("анализ 🙂"));
    EXPECT_TRUE(ShouldBeFiltered("анализ123"));
}

TEST(StringFilters, ShouldBeFiltered_OnlyPunct)
{
    EXPECT_TRUE(ShouldBeFiltered("!!!"));
    EXPECT_TRUE(ShouldBeFiltered("...,,,"));
}

TEST(StringFilters, ShouldBeFiltered_CyrillicClean)
{
    EXPECT_FALSE(ShouldBeFiltered("анализ текста"));
    EXPECT_FALSE(ShouldBeFiltered("пример данных"));
}

TEST(StringFilters, ShouldBeFiltered_Empty)
{
    EXPECT_TRUE(ShouldBeFiltered(""));
}
