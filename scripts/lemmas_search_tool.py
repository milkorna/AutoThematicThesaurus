import json

# Specify the path to your file
file_path = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/total_results_no_sw.json'

# Function to extract phrases with a specific lemma
def extract_phrases_with_lemma(lemma_to_find, file_path):
    # Load data from the JSON file
    with open(file_path, 'r', encoding='utf-8') as f:
        phrases_data = json.load(f)

    # List to store phrases containing the desired lemma
    matching_phrases = []

    # Iterate through all phrases and lemmas in the data
    for phrase, data in phrases_data.items():
        for lemma in data.get("6_lemmas", []):
            # Remove index before "_" and check if it matches the desired lemma
            lemma_name = lemma.get("0_lemma", "").split('_', 1)[1]
            if lemma_name == lemma_to_find:
                matching_phrases.append(phrase)
                break  # Stop further checks once a match is found

    return matching_phrases

# Input for the lemma to search
lemma_to_find = input("Enter lemma to search for: ")  # Enter lemma for search, e.g., "model"
phrases = extract_phrases_with_lemma(lemma_to_find, file_path)

# Print all found phrases
print(f"Phrases with lemma '{lemma_to_find}':")
for phrase in phrases:
    print(phrase)
