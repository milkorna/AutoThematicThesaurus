#pragma once

#include <string>

/**
 * @brief Represents a document record parsed from JSON source
 * @details Contains document metadata (ID, title) and text content.
 * Provides utility method to combine fields for processing.
 */
struct DocumentRecord {
    /// @brief Unique document identifier
    std::string doc_id;

    /// @brief Document title
    std::string title;

    /// @brief Document body text
    std::string text;

    /**
     * @brief Combines title and text fields for processing
     * @details Returns merged text with configurable inclusion of title.
     * If both title and text are present, they are joined with newline separator.
     * If only one field is present, returns that field.
     * If both are empty, returns empty string.
     *
     * @param mergeWithTitle If true, includes title in output;
     *                       if false, returns text only (default: true)
     * @return Combined text ready for processing
     */
    [[nodiscard]] std::string getProcessingText(bool mergeWithTitle = true) const {
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