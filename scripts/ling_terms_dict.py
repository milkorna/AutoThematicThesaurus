# Import required libraries that will be used for PDF parsing and regular expressions
import fitz  # PyMuPDF
import re
import os

def unique_elements(phrases):
    seen = set()
    unique_phrases = []
    for phrase in phrases:
        if phrase not in seen:
            unique_phrases.append(phrase)
            seen.add(phrase)
    return unique_phrases


def filter_phrases(phrases):
    # Step 1: Filter out short phrases
    filtered = [phrase for phrase in phrases if len(phrase) > 4]

    # Step 2: Special condition filtering
    final_filtered = []
    for i, phrase in enumerate(filtered):
        # Skip the first and last elements to avoid IndexError
        if i > 0 and i < len(filtered) - 1:
            if filtered[i-1][0] == filtered[i+1][0] and phrase[0] != filtered[i-1][0]:
                continue  # This phrase is skipped
        final_filtered.append(phrase)

    return final_filtered

# Function to extract uppercase phrases from a PDF file
def extract_uppercase_phrases_from_pdf(pdf_path):
    uppercase_phrases = []

    with fitz.open(pdf_path) as pdf:
    # Iterate over the specified range of pages in the PDF
        for page_num in range(31, 351):  # start from page 32 (index 31) to page 351 (index 350)
            # Get the text of the current page
            page_text = pdf[page_num].get_text("text")

            # Regular expression pattern to match uppercase words or phrases followed by a period
            # The pattern will match consecutive uppercase words or single uppercase words at the start of a line,
            # potentially followed by a lowercase description ending with a period.
            pattern = re.compile(r'^[А-Я¨]{2,}(?:\s+[А-Я¨]{2,})*', re.MULTILINE)

            # Find all matches on the current page and add them to the list
            matches = pattern.findall(page_text)
            uppercase_phrases.extend(matches)

    filtered_phrases = filter_phrases(unique_elements(uppercase_phrases))

    # Remove any surrounding whitespace and return the list
    return [phrase.strip() for phrase in filtered_phrases]

# Get the current script directory
current_dir = os.path.dirname(os.path.abspath(__file__))

# Construct the file path by joining the directory path with the file name
pdf_path = os.path.join(current_dir, '33612459.pdf')

# Extract the uppercase phrases
phrases = extract_uppercase_phrases_from_pdf(pdf_path)
# Print the extracted phrases
for phrase in phrases:
    print(phrase)

# Save the extracted words to a text file, each word on a new line
output_file_path = os.path.join(current_dir, 'extracted_uppercase_words.txt')
with open(output_file_path, 'w', encoding='utf-8') as file:
    for phrase in phrases:
        file.write(f"{phrase}\n")









#         std::vector<std::string> extractValues(const std::string& line) {
#     std::vector<std::string> values;
#     std::istringstream iss(line);
#     std::string token;

#     while (std::getline(iss, token, ',')) {
#         size_t pos = token.find('=');
#         if (pos != std::string::npos) {
#             values.push_back(token.substr(pos + 1));
#         }
#     }

#     for (auto& val : values) {
#         val.erase(std::remove(val.begin(), val.end(), '['), val.end());
#         val.erase(std::remove(val.begin(), val.end(), ']'), val.end());
#         removeSpaces(val);
#     }

#     return values;
# }