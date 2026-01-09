#pragma once

#include "ModelComponent.h"
#include "PhraseMatchStatus.h"
#include "WordComplex.h"

#include <vector>

class PhraseMatchStatus;

/**
 * @brief Encapsulates all validation logic for tokens and components
 */
class PhraseValidator {
  public:
    explicit PhraseValidator(const std::vector<X::WordFormPtr>& sentence) {
    }

    // Фаза 1: Базовая валидация токена (стоп-слова, пунктуация, ошибки анализа)
    static bool isTokenValid(const X::WordFormPtr& token);

    // Фаза 2: Проверка морфологии против условия
    static bool validateMorphology(const X::WordFormPtr& token, const Condition& condition, PhraseMatchStatus& status);

    // Фаза 3: Проверка компонентов модели
    static bool validateWordComponents(const std::vector<X::WordFormPtr>& sentence, const WordComplexPtr& phrase,
                                       const std::shared_ptr<ModelComp>& modelComp, PhraseMatchStatus& status);

    // Фаза 4: Проверка что фраза готова к сохранению
    static bool isPhraseComplete(const PhraseMatchStatus& status);
};
