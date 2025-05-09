import json
from deep_translator import GoogleTranslator
from transformers import pipeline
from concurrent.futures import ThreadPoolExecutor, as_completed

from core.paths import PATH_TOTAL_RESULTS, PATH_MNLI_CLASSIFIED_PHRASES

# Load the Zero-Shot classification model
classifier = pipeline("zero-shot-classification", model="facebook/bart-large-mnli")

# Open the JSON file with Russian phrases
with open(PATH_TOTAL_RESULTS, 'r', encoding='utf-8') as json_file:
    data = json.load(json_file)

print("Initializing translator (ru -> en)...")
# Create an instance of the translator from Russian to English
translator = GoogleTranslator(source='ru', target='en')

# List of phrases to classify
phrases = list(data.keys())

# Define the ordered labels for classification
candidate_labels_order = [
    "NLP term",
    "machine learning term",
    "scientific term",
    "technical term",
    "colloquial phrase",
    "everyday expression",
    "general phrase"
]

# Define the threshold values for each label
thresholds = {
    "NLP term": 0.68,
    "machine learning term": 0.76,
    "scientific term": 0.76,
    "technical term": 0.76,
    "colloquial phrase": 0.8,
    "everyday expression": 0.8,
    "general phrase": 0.8  # default value for non-matched terms
}

# List to store results in JSON format
classified_phrases = []

# Function to classify a single phrase
def classify_phrase(phrase):
    try:
        # Translate the phrase from Russian to English
        translated_phrase = translator.translate(phrase)
        print(f"Original: {phrase} -> Translated: {translated_phrase}")

        label = "general phrase"  # Default label if none of the labels match

        # Iterate through the candidate labels in order
        for candidate_label in candidate_labels_order:
            result = classifier(translated_phrase, [candidate_label, "general phrase"])
            threshold = thresholds.get(candidate_label, 0.8)  # Get the threshold for the current label
            if result['labels'][0] == candidate_label and result['scores'][0] > threshold:
                label = candidate_label
                break  # Stop further checks if a label is assigned

        # Return the classified phrase as a dictionary
        return {"phrase": phrase, "label": label}

    except Exception as e:
        print(f"Error processing phrase '{phrase}': {e}")
        return {"phrase": phrase, "label": "general phrase"}  # Return as general phrase on error

# Create a ThreadPoolExecutor for concurrent processing
with ThreadPoolExecutor(max_workers=10) as executor:
    # Submit tasks to the executor
    future_to_phrase = {executor.submit(classify_phrase, phrase): phrase for phrase in phrases}

    # Collect results as they complete
    for future in as_completed(future_to_phrase):
        result = future.result()
        classified_phrases.append(result)

# Write the classified phrases to a JSON file
with open(PATH_MNLI_CLASSIFIED_PHRASES, 'w', encoding='utf-8') as output_file:
    json.dump(classified_phrases, output_file, ensure_ascii=False, indent=4)

print("Classified phrases have been saved.")
