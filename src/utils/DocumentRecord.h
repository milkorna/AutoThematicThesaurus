#pragma once

#include <string>

/**
 * @brief Структура для представления документа из JSON
 */
struct DocumentRecord {
    std::string doc_id;
    std::string title;
    std::string text;

    /**
     * @brief Объединяет title и text в один текст для обработки
     * @param mergeWithTitle если true, объединяет title и text с переводом строки
     * @return объединенный текст
     */
    std::string getProcessingText(bool mergeWithTitle = true) const {
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
};