import json
from collections import Counter
import matplotlib.pyplot as plt
import seaborn as sns

# Загрузка usage_variants
with open('usage_variants.json', 'r', encoding='utf-8') as f:
    usage_data = json.load(f)

# Загрузка relations.json
with open('/home/milkorna/Documents/AutoThematicThesaurus/thesaurus/relations.json', 'r', encoding='utf-8') as f:
    rel_data = json.load(f)

# --- ЧАСТЬ 1: Базовая статистика и визуализации ---

variant_counts = [len(entry["usage_variants"]) for entry in usage_data]
total_phrases = len(usage_data)
avg_variants = sum(variant_counts) / total_phrases
max_variants = max(variant_counts)
min_variants = min(variant_counts)

print("=== Количественные характеристики ===")
print(f"Общее число базовых фраз: {total_phrases}")
print(f"Среднее число вариантов:  {avg_variants:.2f}")
print(f"Максимум вариантов:       {max_variants}")
print(f"Минимум вариантов:        {min_variants}")

# --- Графики ---
plt.figure(figsize=(8, 5))
sns.histplot(variant_counts, bins=range(1, max(variant_counts)+2), kde=False)
plt.title("Распределение количества usage_variants")
plt.xlabel("Число usage_variants")
plt.ylabel("Частота")
plt.tight_layout()
plt.savefig("usage_variants_histogram.png")
plt.close()

base_probs = [entry["oof_prob_class"] for entry in usage_data]
variant_probs = [var["oof_prob_class"]
                 for entry in usage_data
                 for var in entry["usage_variants"]]

plt.figure(figsize=(8, 5))
sns.boxplot(data=[base_probs, variant_probs])
plt.xticks([0, 1], ["Base phrases", "Usage variants"])
plt.ylabel("oof_prob_class")
plt.title("Сравнение вероятностей терминов (base vs. variants)")
plt.tight_layout()
plt.savefig("oof_prob_comparison_boxplot.png")
plt.close()

# --- ЧАСТЬ 2: Анализ связей по relations.json ---

# Собираем все пары (source, target) → relation
relation_lookup = dict()
for entry in rel_data:
    key = entry["key"]
    for phrase in entry["phrases"]:
        relation_lookup[(key, phrase["phrase"])] = phrase["relation"]

# Проверка связей между base и variants
relation_counts = Counter()
total_checked = 0
related_examples = []

for entry in usage_data:
    base = entry["phrase"]
    for variant in entry["usage_variants"]:
        var = variant["phrase"]
        total_checked += 1
        if (base, var) in relation_lookup:
            rel = relation_lookup[(base, var)]
            relation_counts[rel] += 1
            related_examples.append((base, var, rel))
        elif (var, base) in relation_lookup:
            rel = relation_lookup[(var, base)]
            relation_counts[rel + " (reversed)"] += 1
            related_examples.append((var, base, rel + " (reversed)"))

# --- Вывод статистики по отношениям ---
print("\n=== Связи между base и usage_variant по relations.json ===")
print(f"Всего проверено пар: {total_checked}")
print(f"Найдено связанных пар: {sum(relation_counts.values())}")

for rel, count in relation_counts.items():
    percent = 100 * count / total_checked
    print(f"{rel:15}: {count:4d} ({percent:.2f}%)")

# --- Пример найденных связей ---
print("\nПримеры найденных отношений:")
for base, var, rel in related_examples[:10]:  # первые 10
    print(f"{base} ↔ {var} — {rel}")
