#include "StringUtils.h"
#include <gtest/gtest.h>

using namespace StringUtils;

// -------------------------------------------------------------
// 1. Tests for IsOnlyPunctuationOrDigits
// -------------------------------------------------------------
TEST(StringUtils, OnlyDigitsOrPunct_Positive) {
    EXPECT_TRUE(isOnlyPunctuationOrDigits("123"));
    EXPECT_TRUE(isOnlyPunctuationOrDigits("!!!"));
    EXPECT_TRUE(isOnlyPunctuationOrDigits("123?!.,"));
}

TEST(StringUtils, OnlyDigitsOrPunct_Negative) {
    EXPECT_FALSE(isOnlyPunctuationOrDigits("abc"));
    EXPECT_FALSE(isOnlyPunctuationOrDigits("!@abc"));
    EXPECT_FALSE(isOnlyPunctuationOrDigits("12а"));
    EXPECT_FALSE(isOnlyPunctuationOrDigits("тест"));
    EXPECT_FALSE(isOnlyPunctuationOrDigits("Hello!"));
}

// -------------------------------------------------------------
// 2. Tests for HasNonCyrillicOrSpecialUnicode
// -------------------------------------------------------------
TEST(StringUtils, HasNonCyrillicOrSpecialUnicode_Positive) {
    EXPECT_TRUE(hasNonCyrillicOrSpecialUnicode("123"));
    EXPECT_TRUE(hasNonCyrillicOrSpecialUnicode("🙂"));
    EXPECT_TRUE(hasNonCyrillicOrSpecialUnicode("你好"));
    EXPECT_TRUE(hasNonCyrillicOrSpecialUnicode("مرحبا"));
    EXPECT_TRUE(hasNonCyrillicOrSpecialUnicode("देवनागरी"));
    EXPECT_TRUE(hasNonCyrillicOrSpecialUnicode("x+y=z"));
}

TEST(StringUtils, HasNonCyrillicOrSpecialUnicode_Negative) {
    EXPECT_FALSE(hasNonCyrillicOrSpecialUnicode("Привет мир"));
    EXPECT_FALSE(hasNonCyrillicOrSpecialUnicode("анализ данных"));
}

// -------------------------------------------------------------
// 3. Tests for ShouldBeFiltered
// -------------------------------------------------------------
TEST(StringUtils, ShouldBeFiltered_SymbolBased) {
    EXPECT_TRUE(shouldBeFiltered("%%%"));
    EXPECT_TRUE(shouldBeFiltered("текст_with_underscores"));
    EXPECT_TRUE(shouldBeFiltered("тест#$"));
}

TEST(StringUtils, ShouldBeFiltered_LongLatin) {
    std::string longLatin(30, 'a'); // 30 symbols of 'a'
    EXPECT_TRUE(shouldBeFiltered(longLatin));
}

TEST(StringUtils, ShouldBeFiltered_EmojiOrDigits) {
    EXPECT_TRUE(shouldBeFiltered("анализ 🙂"));
    EXPECT_TRUE(shouldBeFiltered("анализ123"));
}

TEST(StringUtils, ShouldBeFiltered_OnlyPunct) {
    EXPECT_TRUE(shouldBeFiltered("!!!"));
    EXPECT_TRUE(shouldBeFiltered("...,,,"));
}

TEST(StringUtils, ShouldBeFiltered_CyrillicClean) {
    EXPECT_FALSE(shouldBeFiltered("анализ текста"));
    EXPECT_FALSE(shouldBeFiltered("пример данных"));
}

TEST(StringUtils, ShouldBeFiltered_Empty) {
    EXPECT_TRUE(shouldBeFiltered(""));
}
