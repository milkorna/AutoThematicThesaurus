import json
from collections import Counter

MAX_KEY = "обучение двухклассовый модель"

with open('/home/milkorna/Documents/AutoThematicThesaurus/thesaurus/relations.json', 'r', encoding='utf-8') as f:
    gold_data = json.load(f)

with open('hypernym_voting_results.json', 'r', encoding='utf-8') as f:
    voting_data = json.load(f)

gold_pairs = set()
for entry in gold_data:
    key = entry["key"]
    if key > MAX_KEY:
        continue
    for phrase in entry["phrases"]:
        if phrase["relation"] == "hypernym":
            gold_pairs.add((phrase["phrase"], key))

pred_pairs = set()
for result in voting_data["results"]:
    hypo = result["query"]
    if hypo > MAX_KEY:
        continue
    for h in result.get("hypernyms", []):
        hyper = h["hypernym"]
        pred_pairs.add((hyper, hypo))

tp = len(pred_pairs & gold_pairs)
fp = len(pred_pairs - gold_pairs)
fn = len(gold_pairs - pred_pairs)

precision = tp / (tp + fp) if (tp + fp) else 0.0
recall = tp / (tp + fn) if (tp + fn) else 0.0
f1 = 2 * precision * recall / (precision + recall) if (precision + recall) else 0.0

print("=== Метрики для hypernym voting (гипер → гипо) ===")
print(f"True Positives:  {tp}")
print(f"False Positives: {fp}")
print(f"False Negatives: {fn}")
print(f"Precision:       {precision:.4f}")
print(f"Recall:          {recall:.4f}")
print(f"F1 Score:        {f1:.4f}")
