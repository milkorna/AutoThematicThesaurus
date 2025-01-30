import json

# Specify the path to your file
file_path = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/total_results_no_sw_synonyms.json'

# Function to find phrases with lemmas having TF-IDF below a specified threshold
def find_phrases_with_low_tfidf(file_path, tfidf_threshold):
    # Load data from the JSON file
    with open(file_path, 'r', encoding='utf-8') as f:
        phrases_data = json.load(f)

    # List to store phrases that contain lemmas with low TF-IDF values
    matching_phrases = []

    # Iterate through all phrases and their lemmas in the data
    for phrase, data in phrases_data.items():
        # Check conditions for phrases_count and topic_relevance
        phrases_count = data.get("7_phrases_count", 0)
        topic_relevance = data.get("2_topic_relevance", 0)

        for lemma in data.get("6_lemmas", []):
            # Check if the TF-IDF value is below the specified threshold
            tfidf_value = lemma.get("3_tf-idf", 0)
            if tfidf_value < tfidf_threshold:
                if phrases_count > 1 or topic_relevance > 0.4:
                    matching_phrases.append(f"#{phrase}")
                else:
                    matching_phrases.append(phrase)
                break  # Stop further checks once a low TF-IDF lemma is found in the phrase

    return matching_phrases

# Input for the TF-IDF threshold
tfidf_threshold = float(input("Enter TF-IDF threshold value: "))  # Example input: 0.00001
phrases = find_phrases_with_low_tfidf(file_path, tfidf_threshold)

# Specify the output file path
output_file_path = 'low_tfidf_phrases.txt'

# Save the found phrases to a text file
with open(output_file_path, 'w', encoding='utf-8') as file:
    file.write(f"Phrases with lemmas having TF-IDF below {tfidf_threshold}:\n")
    for phrase in phrases:
        file.write(phrase + '\n')

print(f"Found phrases have been saved to {output_file_path}")