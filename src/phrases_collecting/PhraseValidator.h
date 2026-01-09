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
    explicit PhraseValidator(const std::vector<X::WordFormPtr>& sentence) : m_sentence(sentence) {
    }

    // Фаза 1: Базовая валидация токена (стоп-слова, пунктуация, ошибки анализа)
    bool isTokenValid(const X::WordFormPtr& token) const;

    // Фаза 2: Проверка морфологии против условия
    bool validateMorphology(const X::WordFormPtr& token, const Condition& condition, PhraseMatchStatus& status) const;

    // Фаза 3: Проверка компонентов модели
    bool validateWordComponents(const WordComplexPtr& phrase, const std::shared_ptr<ModelComp>& modelComp,
                                PhraseMatchStatus& status) const;

    // Фаза 4: Проверка что фраза готова к сохранению
    bool isPhraseComplete(const PhraseMatchStatus& status) const;

  private:
    const std::vector<X::WordFormPtr>& m_sentence;
};
