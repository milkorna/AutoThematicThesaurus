import json
from collections import Counter

MAX_KEY = "обучение двухклассовый модель"

with open('/home/milkorna/Documents/AutoThematicThesaurus/thesaurus/relations.json', 'r', encoding='utf-8') as f:
    gold_data = json.load(f)

with open('final_synonyms.json', 'r', encoding='utf-8') as f:
    pred_data = json.load(f)

gold_rel_counts = Counter()
gold_synonyms = {}

for entry in gold_data:
    key = entry["key"]
    if key > MAX_KEY:
        continue
    for phrase in entry["phrases"]:
        gold_rel_counts[phrase["relation"]] += 1
        if phrase["relation"] == "synonym":
            gold_synonyms.setdefault(key, set()).add(phrase["phrase"])

pred_synonyms = {}
pred_relation_inferred = Counter()

for entry in pred_data:
    key = entry["key"]
    if key > MAX_KEY:
        continue
    phrases = entry.get("synonyms", [])
    pred_relation_inferred["synonym"] += len(phrases)
    pred_synonyms.setdefault(key, set()).update(phrases)

tp = fp = fn = 0
all_keys = set(gold_synonyms.keys()).union(pred_synonyms.keys())

for key in all_keys:
    gold = gold_synonyms.get(key, set())
    pred = pred_synonyms.get(key, set())
    tp += len(gold & pred)
    fp += len(pred - gold)
    fn += len(gold - pred)

precision = tp / (tp + fp) if (tp + fp) else 0.0
recall = tp / (tp + fn) if (tp + fn) else 0.0
f1 = 2 * precision * recall / (precision + recall) if (precision + recall) else 0.0

print("=== Метрики для отношения 'synonym' по final_synonyms.json ===")
print(f"True Positives:  {tp}")
print(f"False Positives: {fp}")
print(f"False Negatives: {fn}")
print(f"Precision:       {precision:.4f}")
print(f"Recall:          {recall:.4f}")
print(f"F1 Score:        {f1:.4f}")

print("\n=== Распределение отношений в эталоне ===")
total_gold = sum(gold_rel_counts.values())
for rel, count in gold_rel_counts.items():
    percent = 100 * count / total_gold
    print(f"{rel:10}: {count:4d} ({percent:.2f}%)")

print("\n=== Распределение отношений в эксперименте (все как 'synonym') ===")
total_pred = sum(pred_relation_inferred.values())
for rel, count in pred_relation_inferred.items():
    percent = 100 * count / total_pred
    print(f"{rel:10}: {count:4d} ({percent:.2f}%)")
