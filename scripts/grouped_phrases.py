import pandas as pd
import json
from collections import defaultdict

from scripts.core.paths import PATH_DATA_WITH_OFF

# Load dataset
df = pd.read_excel(PATH_DATA_WITH_OFF)

# Create a dictionary to group phrases
phrase_groups = defaultdict(list)

# Group phrases based on the set of words in them
for _, row in df.iterrows():
    phrase_key = tuple(sorted(row['key'].split())) # Sort words within the phrase
    phrase_groups[phrase_key].append({
        "key": row['key'],
        "is_term_manual": row['is_term_manual'],
        "frequency": row['frequency'],
        "oof_prob_class": row['oof_prob_class']
    })

# Exclude groups containing only one phrase or only phrases with is_term_manual = 0 and oof_prob_class < 0.4
filtered_groups = {}
for key, phrases in phrase_groups.items():
    if len(phrases) > 1:
        if not all(p["is_term_manual"] == 0 and p["oof_prob_class"] < 0.4 for p in phrases):
            filtered_groups[key] = phrases

# Convert results into a list of dictionaries
output_data = [{"group_key": " ".join(key), "phrases": phrases, "merge_group": None} for key, phrases in filtered_groups.items()]

# Save to JSON
output_file = "grouped_phrases.json"
with open(output_file, "w", encoding="utf-8") as f:
    json.dump(output_data, f, ensure_ascii=False, indent=4)

print(f"Grouping completed. Data saved to {output_file}")
