#include "Document.h"

// ────────────────────────────────────────────────────────────────────────
// ПОЛУЧЕНИЕ ИНФОРМАЦИИ
// ────────────────────────────────────────────────────────────────────────

std::string Document::getDocId() const {
    return doc_id;
}

std::string Document::getFilename() const {
    return filename;
}

std::string Document::getProcessingTimestamp() const {
    return processing_timestamp;
}

std::string Document::getTitle() const {
    return title;
}

std::string Document::getText(bool mergeWithTitle) const {
    if (mergeWithTitle) {
        if (title.empty()) {
            return text;
        }
        if (text.empty()) {
            return title;
        }
        return title + "\n" + text;
    } else {
        return text;
    }
}

// ────────────────────────────────────────────────────────────────────────
// ПОЛУЧЕНИЕ СТАТИСТИКИ
// ────────────────────────────────────────────────────────────────────────

size_t Document::getSentenceCount() const {
    return stats.sentence_count;
}

size_t Document::getWordCount() const {
    return stats.word_count;
}

size_t Document::getUniqueLemmasCount() const {
    return stats.unique_lemmas;
}

size_t Document::getCharacterCount() const {
    return stats.character_count;
}

const std::unordered_map<std::string, size_t>& Document::getWordFrequency() const {
    return word_frequency_local;
}

const std::unordered_set<std::string>& Document::getUniqueLemmas() const {
    return document_lemmas;
}

// ────────────────────────────────────────────────────────────────────────
// ОБНОВЛЕНИЕ СТАТИСТИКИ
// ────────────────────────────────────────────────────────────────────────

void Document::incrementSentenceCount(size_t count) {
    stats.sentence_count += count;
}

void Document::incrementWordCount(size_t count) {
    stats.word_count += count;
}

void Document::incrementWordFrequency(const std::string& lemma, size_t count) {
    word_frequency_local[lemma] += count;
}

void Document::addUniqueLemma(const std::string& lemma) {
    auto [it, inserted] = document_lemmas.insert(lemma);
    if (inserted) {
        stats.unique_lemmas++;
    }
}

void Document::addUniqueLemmasFromSentence(const std::unordered_set<std::string>& lemmas) {
    for (const auto& lemma : lemmas) {
        addUniqueLemma(lemma);
    }
}

void Document::setCharacterCount(size_t count) {
    stats.character_count = count;
}