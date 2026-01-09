#include "ComplexPhrasesCollector.h"
#include "ModelComponent.h"
#include "Options.h"
#include "PhraseExtender.h"
#include "PhraseMatchStatus.h"

void ComplexPhrasesCollector::validateBoundaries() {
    if (m_collection.empty()) {
        return;
    }

    // A new container to store only the valid elements after validation
    std::vector<WordComplexPtr> validatedCollection;

    for (const auto& it : m_collection) {
        if (it == nullptr) {
            continue;
        }

        bool isNested = false;

        const auto s = it->pos.start;
        const auto e = it->pos.end;

        for (const auto& innerIt : m_collection) {
            if (innerIt == it) {
                continue;
            }

            const auto& innerWC = innerIt;

            // Mark the current element as nested if its start position is the same as the inner element's start, and
            // its end position is less than or equal to the inner element's end position (indicating that the current
            // element is within the inner one)
            if (innerWC->pos.start == s && innerWC->pos.end >= e) {
                isNested = true;
                break;
            }
        }

        // If the current element is not nested within another element, add it to the validated collection
        if (!isNested) {
            validatedCollection.push_back(it);
        }
    }

    // Replace the old collection with the new collection containing only valid elements
    m_collection = std::move(validatedCollection);
}

bool ComplexPhrasesCollector::isSimplePhraseMatchesComponent(const WordComplexPtr& simplePhrase,
                                                             const std::shared_ptr<ModelComp>& modelComp,
                                                             PhraseMatchStatus& status) const {

    // Проверяем морфологию
    if (!PhraseValidator::validateWordComponents(m_sentence, simplePhrase, modelComp, status)) {
        return false;
    }

    // Если head валидирован но не совпадает - фраза не подходит
    if (status.headValidated && !status.headMatched) {
        return false;
    }

    // Проверяем дополнительные условия
    if (modelComp->getCondition().hasExactLexeme()) {
        return false; // Если есть условия и они не empty - не подходит
    }

    return true;
}

bool ComplexPhrasesCollector::processModelForPhrase(const std::shared_ptr<Model>& model,
                                                    const WordComplexPtr& simplePhrase, size_t simplePhraseIndex) {

    auto componentIndex = model->getModelCompIndByForm(simplePhrase->modelName);
    if (!componentIndex) {
        return false;
    }

    PhraseMatchStatus status;

    // Этап 1: Проверяем что простая фраза соответствует компоненту модели
    auto modelComp = model->getModelComponent(*componentIndex);
    if (!isSimplePhraseMatchesComponent(simplePhrase, modelComp, status)) {
        return false;
    }

    // Этап 2: Инициализируем и пытаемся расширить фразу
    return expandPhraseAroundComponent(model, simplePhrase, simplePhraseIndex, *componentIndex, status);
}

bool ComplexPhrasesCollector::expandPhraseAroundComponent(const std::shared_ptr<Model>& model,
                                                          const WordComplexPtr& simplePhrase, size_t simplePhraseIndex,
                                                          size_t componentIndex, PhraseMatchStatus& status) {

    auto wc = initializeWordComplex(simplePhrase, model->getForm());
    status.matchedComponents = 1;

    // Создаем временный extender с текущим контекстом
    PhraseExtender extender(model, m_collection, status, simplePhraseIndex, m_simplePhrases, m_sentence);

    // Пытаемся расширить влево
    if (componentIndex > 0 && simplePhrase->pos.start > 0) {
        if (extender.checkComponent(componentIndex - 1, simplePhrase->pos.start - 1, true, wc)) {
            return true;
        }
    }

    // Пытаемся расширить вправо
    if (componentIndex < model->size() - 1 && simplePhrase->pos.end + 1 < m_sentence.size()) {
        if (extender.checkComponent(componentIndex + 1, simplePhrase->pos.end + 1, false, wc)) {
            return true;
        }
    }

    return false;
}

void ComplexPhrasesCollector::collect(Process& process) {
    const auto& patterns = GrammarPatternManager::GetManager();

    // Обрабатываем каждую простую фразу
    for (size_t simplePhraseIndex = 0; simplePhraseIndex < m_simplePhrases.size(); ++simplePhraseIndex) {
        const auto& simplePhrase = m_simplePhrases[simplePhraseIndex];

        // Пытаемся найти подходящую модель
        for (const auto& [name, model] : patterns.getComplexPatterns()) {
            if (processModelForPhrase(model, simplePhrase, simplePhraseIndex)) {
                break; // Найдена подходящая модель - переходим к следующей фразе
            }
        }
    }

    auto& options = Options::getOptions();
    if (options.validateBoundaries) {
        validateBoundaries();
    }

    process.outputResults(m_collection);
}