import json
from collections import Counter

MAX_KEY = "обучение двухклассовый модель"

with open('/home/milkorna/Documents/AutoThematicThesaurus/thesaurus/relations.json', 'r', encoding='utf-8') as f:
    gold_data = json.load(f)

with open('nli_hypernym_relations.json', 'r', encoding='utf-8') as f:
    nli_data = json.load(f)

gold_pairs = set()
for entry in gold_data:
    key = entry["key"]
    if key > MAX_KEY:
        continue
    for phrase in entry["phrases"]:
        if phrase["relation"] == "hypernym":
            gold_pairs.add((phrase["phrase"], key))

pred_pairs = set()
reversed_pairs = set()

for rel in nli_data["relations"]:
    hyper = rel["hyper"]["phrase"]
    hypo = rel["hypo"]["phrase"]
    pair = (hyper, hypo)
    rev_pair = (hypo, hyper)
    pred_pairs.add(pair)

    if rev_pair in gold_pairs and pair not in gold_pairs:
        reversed_pairs.add(pair)

tp = len(gold_pairs & pred_pairs)
fp = len(pred_pairs - gold_pairs)
fn = len(gold_pairs - pred_pairs)
rev = len(reversed_pairs)

precision = tp / (tp + fp) if (tp + fp) else 0.0
recall = tp / (tp + fn) if (tp + fn) else 0.0
f1 = 2 * precision * recall / (precision + recall) if (precision + recall) else 0.0

print("=== Метрики для направленных отношений hypernym → hyponym ===")
print(f"True Positives:        {tp}")
print(f"False Positives:       {fp}")
print(f"False Negatives:       {fn}")
print(f"Reversed Direction:    {rev}")
print(f"Precision:             {precision:.4f}")
print(f"Recall:                {recall:.4f}")
print(f"F1 Score:              {f1:.4f}")
