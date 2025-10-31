#include "grammar_patterns/GrammarPatternManager.h"
#include "grammar_patterns/JsonPatternParser.h"
#include <gtest/gtest.h>

using json = nlohmann::json;

struct JsonParserFixture : ::testing::Test {
    void SetUp() override {
        GrammarPatternManager::GetManager()->clear();
    }

    void TearDown() override {
        GrammarPatternManager::GetManager()->clear();
    }
};

// A. базовый разбор
TEST_F(JsonParserFixture, ParsesSimpleAdjPlusNoun) {
    json arr = json::array({{{"name", "Прил + С"},
                             {"body", json::array({
                                          {
                                              {"type", "word"},
                                              {"role", "dependent"},
                                              {"pos", "ADJ"},
                                              {"recursive", true},
                                          },
                                          {
                                              {"type", "word"},
                                              {"role", "head"},
                                              {"pos", "NOUN"},
                                          },
                                      })}}});

    JsonPatternParser parser(arr);
    parser.parseAll();

    auto* manager = GrammarPatternManager::GetManager();
    ASSERT_TRUE(manager->has("Прил + С"));

    auto modelAdjNoun = manager->get("Прил + С");
    const auto& components = modelAdjNoun->getComponents();
    ASSERT_EQ(components.size(), 2u);

    auto adjDependent = std::dynamic_pointer_cast<WordComp>(components[0]);
    auto headNoun = std::dynamic_pointer_cast<WordComp>(components[1]);
    ASSERT_TRUE(adjDependent);
    ASSERT_TRUE(headNoun);

    EXPECT_EQ(adjDependent->condition().getSyntaxRole(), SyntaxRole::Dependent);
    EXPECT_TRUE(adjDependent->isRec());

    EXPECT_EQ(headNoun->condition().getSyntaxRole(), SyntaxRole::Head);
    EXPECT_FALSE(headNoun->isRec());
}

TEST_F(JsonParserFixture, ParsesNounPlusGenitiveNoun_IndependentRecursive) {
    json arr = json::array({{{"name", "С + Срд"},
                             {"body", json::array({
                                          json{
                                              {"type", "word"},
                                              {"role", "head"},
                                              {"pos", "NOUN"},
                                          },
                                          json{
                                              {"type", "word"},
                                              {"role", "independent"},
                                              {"pos", "NOUN"},
                                              {"features", {{"Case", "Gen"}}},
                                              {"recursive", true},
                                          },
                                      })}}});

    JsonPatternParser parser(arr);
    parser.parseAll();

    auto* manager = GrammarPatternManager::GetManager();
    ASSERT_TRUE(manager->has("С + Срд"));

    auto model = manager->get("С + Срд");
    const auto& comps = model->getComponents();
    ASSERT_EQ(comps.size(), 2u);

    auto headNoun = std::dynamic_pointer_cast<WordComp>(comps[0]);
    auto genNounInd = std::dynamic_pointer_cast<WordComp>(comps[1]);
    ASSERT_TRUE(headNoun);
    ASSERT_TRUE(genNounInd);

    EXPECT_EQ(headNoun->condition().getSyntaxRole(), SyntaxRole::Head);
    EXPECT_FALSE(headNoun->isRec());

    EXPECT_EQ(genNounInd->condition().getSyntaxRole(), SyntaxRole::Independent);
    EXPECT_TRUE(genNounInd->isRec());
    EXPECT_NE(genNounInd->condition().getMorphTag().toString().find("Gen"), std::string::npos);
}

TEST_F(JsonParserFixture, ParsesNounPrepositionForNoun_WithExactLexeme) {
    json arr = json::array({{{"name", "С + Предл(Для) + С"},
                             {"body", json::array({
                                          json{
                                              {"type", "word"},
                                              {"role", "head"},
                                              {"pos", "NOUN"},
                                          },
                                          json{
                                              {"type", "word"},
                                              {"role", "independent"},
                                              {"pos", "ADP"},
                                              {"exact_lexeme", "для"},
                                          },
                                          json{
                                              {"type", "word"},
                                              {"role", "independent"},
                                              {"pos", "NOUN"},
                                          },
                                      })}}});

    JsonPatternParser parser(arr);
    parser.parseAll();

    auto* manager = GrammarPatternManager::GetManager();
    ASSERT_TRUE(manager->has("С + Предл(Для) + С"));

    auto model = manager->get("С + Предл(Для) + С");
    const auto& comps = model->getComponents();
    ASSERT_EQ(comps.size(), 3u);

    auto prepFor = std::dynamic_pointer_cast<WordComp>(comps[1]);
    ASSERT_TRUE(prepFor);

    const auto& add = prepFor->condition().getAdditional();
    EXPECT_EQ(add.m_exLex, "для");
}

TEST_F(JsonParserFixture, ParsesComposite_AdjNounHead_PlusGenNoun_AndDividesAsComplex) {
    json arr = json::array({{{"name", "Прил + С"},
                             {"body", json::array({
                                          json{
                                              {"type", "word"},
                                              {"role", "dependent"},
                                              {"pos", "ADJ"},
                                              {"recursive", true},
                                          },
                                          json{
                                              {"type", "word"},
                                              {"role", "head"},
                                              {"pos", "NOUN"},
                                          },
                                      })}},
                            {{"name", "(Прил + С) + Срд"},
                             {"body", json::array({
                                          json{
                                              {"type", "pattern"},
                                              {"role", "head"},
                                              {"pattern", "Прил + С"},
                                          },
                                          json{
                                              {"type", "word"},
                                              {"role", "independent"},
                                              {"pos", "NOUN"},
                                              {"features", {{"Case", "Gen"}}},
                                              {"recursive", true},
                                          },
                                      })}}});

    JsonPatternParser parser(arr);
    parser.parseAll();

    auto* manager = GrammarPatternManager::GetManager();
    ASSERT_TRUE(manager->has("(Прил + С) + Срд"));

    auto composite = manager->get("(Прил + С) + Срд");
    const auto& comps = composite->getComponents();
    ASSERT_EQ(comps.size(), 2u);

    auto headPattern = std::dynamic_pointer_cast<ModelComp>(comps[0]);
    auto genNoun = std::dynamic_pointer_cast<WordComp>(comps[1]);
    ASSERT_TRUE(headPattern);
    ASSERT_TRUE(genNoun);

    EXPECT_EQ(headPattern->condition().getSyntaxRole(), SyntaxRole::Head);
    EXPECT_NE(genNoun->condition().getMorphTag().toString().find("Gen"), std::string::npos);

    // проверим разделение simple/complex
    EXPECT_GT(manager->complexPatternsSize(), 0u);
}

// B. enabled=false
TEST_F(JsonParserFixture, SkipsDisabledPatterns) {
    json arr = json::array({{
        {"name", "X"},
        {"enabled", false},
        {"body", json::array({})},
    }});
    JsonPatternParser parser(arr);
    parser.parseAll();
    EXPECT_FALSE(GrammarPatternManager::GetManager()->has("X"));
}

// C1. top-level не массив
TEST(JsonParserErrors, ThrowsIfTopLevelNotArray) {
    json notArray = {{"oops", 1}};
    EXPECT_THROW(JsonPatternParser parser(notArray), std::runtime_error);
}

TEST_F(JsonParserFixture, WordWithoutPos_MakesPatternInvalid_AndNotAdded) {
    using nlohmann::json;

    json arr = json::array({{{"name", "Падать без POS"},
                             {"body", json::array({
                                          json{
                                              {"type", "word"},
                                              {"role", "head"},
                                          }, // нет pos -> должна быть ошибка
                                          json{
                                              {"type", "word"},
                                              {"role", "dependent"},
                                              {"pos", "NOUN"},
                                          },
                                      })}},
                            {{"name", "Контрольный валидный"},
                             {"body", json::array({
                                          json{
                                              {"type", "word"},
                                              {"role", "head"},
                                              {"pos", "NOUN"},
                                          },
                                      })}}});

    JsonPatternParser parser(arr);
    parser.parseAll(); // ошибки ловятся внутри parseAll(), битый паттерн «Падать без POS» не добавится

    auto* mgr = GrammarPatternManager::GetManager();
    EXPECT_FALSE(mgr->has("Падать без POS"));      // целиком не добавили
    EXPECT_TRUE(mgr->has("Контрольный валидный")); // остальные добавляются как обычно
}

// D. неизвестная роль -> Independent
TEST_F(JsonParserFixture, UnknownRole_MakesPatternInvalid_AndNotAdded) {
    using nlohmann::json;

    json arr = json::array({{{"name", "ПлохаяРоль"},
                             {"body", json::array({json{
                                          {"type", "word"},
                                          {"role", "???"},
                                          {"pos", "NOUN"},
                                      }})}},
                            {{"name", "Контрольный"},
                             {"body", json::array({json{
                                          {"type", "word"},
                                          {"role", "head"},
                                          {"pos", "NOUN"},
                                      }})}}});

    JsonPatternParser parser(arr);
    parser.parseAll(); // parseAll() ловит исключение и просто не добавляет битый паттерн

    auto* mgr = GrammarPatternManager::GetManager();
    EXPECT_FALSE(mgr->has("ПлохаяРоль")); // из-за неизвестной роли весь паттерн отвергнут
    EXPECT_TRUE(mgr->has("Контрольный")); // валидный паттерн добавлен
}
