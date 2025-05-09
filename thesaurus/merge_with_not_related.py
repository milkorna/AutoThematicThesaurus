import json

# Load new and old data
with open("relations.json", "r", encoding="utf-8") as f:
    new_data = json.load(f)

with open("old_relations_list.json", "r", encoding="utf-8") as f:
    old_data = json.load(f)

# Convert to dictionaries for fast access
new_dict = {entry["key"]: {p["phrase"]: p["relation"] for p in entry["phrases"]} for entry in new_data}
old_dict = {entry["key"]: {p["phrase"]: p["relation"] for p in entry["phrases"]} for entry in old_data}

# Merge: add phrases from old_dict to new_dict with relation="not_related" if not already present
for key, old_phrases in old_dict.items():
    if key not in new_dict:
        # Add completely new key
        new_dict[key] = {phrase: "not_related" for phrase in old_phrases}
    else:
        for phrase in old_phrases:
            if phrase not in new_dict[key]:
                new_dict[key][phrase] = "not_related"

# Build the result in the desired structure
merged_data = [
    {
        "key": key,
        "phrases": [{"phrase": phrase, "relation": relation} for phrase, relation in sorted(phrases.items())]
    }
    for key, phrases in sorted(new_dict.items())
]

# Save the result
with open("relations_merged_with_not_related.json", "w", encoding="utf-8") as f:
    json.dump(merged_data, f, indent=4, ensure_ascii=False)
