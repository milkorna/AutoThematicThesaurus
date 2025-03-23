import json

def remove_self_referencing_phrases(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    cleaned_data = []
    for entry in data:
        key = entry["key"]
        filtered_phrases = [
            p for p in entry["phrases"]
            if p["phrase"] != key
        ]
        if filtered_phrases:
            cleaned_data.append({
                "key": key,
                "phrases": filtered_phrases
            })

    with open(file_path, 'w', encoding='utf-8') as f:
        json.dump(cleaned_data, f, ensure_ascii=False, indent=4)

if __name__ == "__main__":
    file_path = "marked_relations_manual.json"
    remove_self_referencing_phrases(file_path)
