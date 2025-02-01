import json

# Load data from the updated JSON file with synonyms
file_path = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/total_results_no_sw_synonyms.json'
with open(file_path, 'r', encoding='utf-8') as f:
    phrases_data = json.load(f)

# Extract phrase keys from the JSON data to create the set of phrases
phrases_set = set(phrases_data.keys())

# Function to ensure mutual synonym relationships and remove unwanted words
def ensure_mutual_synonyms_and_cleanup(phrases_data, phrases_set, stop_word="данный"):
    # Iterate over each phrase and its synonyms
    for phrase in phrases_set:
        synonyms = phrases_data[phrase].get("9_synonyms", [])

        # Filter out synonyms containing the stop word
        synonyms = [syn for syn in synonyms if stop_word not in syn]
        phrases_data[phrase]["9_synonyms"] = synonyms

        # Iterate over each remaining synonym of the current phrase
        for synonym in synonyms:
            # Check if the synonym is also a phrase in phrases_set
            if synonym in phrases_set:
                # Get the synonyms list of the "synonym" phrase
                synonym_data = phrases_data[synonym].get("9_synonyms", [])

                # If the original phrase is not in the synonym's list, add it
                if phrase not in synonym_data:
                    synonym_data.append(phrase)
                    # Filter out any unwanted synonyms containing the stop word
                    synonym_data = [syn for syn in synonym_data if stop_word not in syn]
                    phrases_data[synonym]["9_synonyms"] = synonym_data

    return phrases_data

# Ensure mutual synonyms and remove unwanted words
phrases_data = ensure_mutual_synonyms_and_cleanup(phrases_data, phrases_set)

# Save the updated data to a new JSON file
output_file_path = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/total_results_no_sw_synonyms_checked.json'
with open(output_file_path, 'w', encoding='utf-8') as f:
    json.dump(phrases_data, f, ensure_ascii=False, indent=4)

print(f"Updated data with mutual synonyms saved to {output_file_path}")
