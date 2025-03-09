import pandas as pd
import json
from concurrent.futures import ProcessPoolExecutor

# Path to the source Excel file
excel_path = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_data.xlsx"
df = pd.read_excel(excel_path)

# Select base phrases (phrase_size == 2)
df_two_words = df[df['phrase_size'] == 2]

# Select phrases with more than 2 words (for searching usage_variants)
df_longer = df[df['phrase_size'] > 2]

# Convert the DataFrame with longer phrases into a list of dictionaries for faster access in parallel processes
longer_records = df_longer[['key', 'is_term_manual', 'oof_prob_class']].to_dict('records')

def process_base_phrase(base_row):
    """
    Processes a single base phrase:
      - Searches for occurrences of the base phrase as a substring in longer phrases.
      - If no usage variants are found (usage_variants is empty), returns None.
    """
    base_phrase = str(base_row['key'])
    base_is_term_manual = base_row['is_term_manual']
    base_oof_prob_class = base_row['oof_prob_class']

    usage_variants = []
    for rec in longer_records:
        long_phrase = str(rec['key'])
        if base_phrase in long_phrase:
            usage_variants.append({
                'phrase': long_phrase,
                'is_term_manual': rec['is_term_manual'],
                'oof_prob_class': rec['oof_prob_class']
            })

    # If no usage variants exist, return None (i.e., the record will not be included)
    if not usage_variants:
        return None

    return {
        'phrase': base_phrase,
        'is_term_manual': base_is_term_manual,
        'oof_prob_class': base_oof_prob_class,
        'usage_variants': usage_variants
    }

def main():
    # Convert base phrases into a list of dictionaries for passing to the process pool
    base_records = df_two_words.to_dict('records')

    # Use ProcessPoolExecutor for parallel processing of base phrases
    with ProcessPoolExecutor() as executor:
        results_iter = executor.map(process_base_phrase, base_records)

    # Filter results: exclude records where the function returned None
    results = [record for record in results_iter if record is not None]

    # Save the final result to a JSON file
    output_path = "usage_variants.json"
    with open(output_path, "w", encoding="utf-8") as json_file:
        json.dump(results, json_file, ensure_ascii=False, indent=4)

    print(f"JSON file successfully saved: {output_path}")

if __name__ == "__main__":
    main()
