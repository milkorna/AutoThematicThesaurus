import json
from transformers import AutoTokenizer, AutoModelForMaskedLM
import torch
import pymorphy2

# Initialize the morphological analyzer for the Russian language
morph = pymorphy2.MorphAnalyzer()

# Load the model and tokenizer
model_name = "DeepPavlov/rubert-base-cased"  # You can replace this with another model
tokenizer = AutoTokenizer.from_pretrained(model_name)
model = AutoModelForMaskedLM.from_pretrained(model_name)

# Function to determine the part of speech using pymorphy2
def get_pos_tag(word):
    parsed_word = morph.parse(word)[0]
    return parsed_word.tag.POS

# Function to lemmatize a word
def lemmatize_word(word):
    parsed_word = morph.parse(word)[0]
    return parsed_word.normal_form

# Function to generate new phrases by replacing words one by one with synonyms
def find_synonyms_in_set(phrase, phrases_set, top_n=20):
    words = phrase.split()  # Split the phrase into individual words
    generated_phrases = set()  # Set to store generated phrases

    # Determine the parts of speech of all words in the original phrase
    pos_tags = [get_pos_tag(word) for word in words]

    # Iterate over each word in the phrase
    for i, (word, original_pos) in enumerate(zip(words, pos_tags)):
        # Create a masked input sentence with the current word replaced by [MASK]
        masked_sentence = words.copy()
        masked_sentence[i] = "[MASK]"
        input_text = f"{' '.join(masked_sentence)} может быть заменено на [MASK]."
        input_ids = tokenizer.encode(input_text, return_tensors="pt")
        mask_token_index = torch.where(input_ids == tokenizer.mask_token_id)[1]

        # Get model predictions for the masked token
        with torch.no_grad():
            output = model(input_ids)
            logits = output.logits

        # Find the top N predicted tokens as synonyms
        mask_token_logits = logits[0, mask_token_index, :]
        top_tokens = torch.topk(mask_token_logits, top_n, dim=1).indices[0].tolist()

        # Decode the tokens to words
        synonyms = [lemmatize_word(tokenizer.decode([token]).strip().lower()) for token in top_tokens]

        # Generate new phrases with each synonym replacing the masked word
        for synonym in synonyms:
            synonym_pos = get_pos_tag(synonym)
            # Check if the part of speech matches the original word's part of speech
            if synonym_pos == original_pos:
                new_phrase = words.copy()
                new_phrase[i] = synonym
                generated_phrase = " ".join(new_phrase)
                # Ensure the generated phrase is not identical to the original phrase
                if generated_phrase != phrase:
                    generated_phrases.add(generated_phrase)

    # Find matches in the phrases set
    matched_phrases = [phrase for phrase in generated_phrases if phrase in phrases_set]

    # Log the results
    if not matched_phrases:
        with open("script_logs.txt", "a", encoding="utf-8") as log_file:
            log_file.write(f"No exact matches found in the set for '{phrase}'. Generated phrases: {generated_phrases}\n")
            
    return matched_phrases

# Load data from your JSON file
file_path = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/total_results_no_sw.json'  # Specify the path to your file
with open(file_path, 'r', encoding='utf-8') as f:
    phrases_data = json.load(f)

# Extract phrase keys from the JSON data to create the set of phrases
phrases_set = set(phrases_data.keys())

# Log the total number of phrases collected
print(f"Total number of phrases collected: {len(phrases_set)}")

# Iterate over each phrase and find synonyms
for phrase in phrases_set:
    # Find synonyms for the current phrase
    synonyms = find_synonyms_in_set(phrase, phrases_set)
    # Add the synonyms to the JSON object under the key "9_synonyms"
    phrases_data[phrase]["9_synonyms"] = synonyms

# Save the updated data to a new JSON file
output_file_path = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/total_results_no_sw_synonyms.json'
with open(output_file_path, 'w', encoding='utf-8') as f:
    json.dump(phrases_data, f, ensure_ascii=False, indent=4)

print(f"Updated data saved to {output_file_path}")