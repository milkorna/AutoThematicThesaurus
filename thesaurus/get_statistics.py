import json
from collections import Counter, defaultdict

with open("relations_merged_with_not_related.json", "r", encoding="utf-8") as f:
    data = json.load(f)

relation_counter = Counter()
phrases_per_key = []
unique_phrases = set()
phrase_occurrences = defaultdict(int)

for entry in data:
    key = entry["key"]
    phrases = entry["phrases"]
    phrases_per_key.append(len(phrases))
    for p in phrases:
        relation_counter[p["relation"]] += 1
        unique_phrases.add(p["phrase"])
        phrase_occurrences[p["phrase"]] += 1

print("Общая статистика по отношениям:")
for relation, count in relation_counter.most_common():
    print(f"  {relation}: {count}")

print("\nОбщее количество ключей:", len(data))
print("Среднее количество фраз на ключ:", sum(phrases_per_key) / len(phrases_per_key))
print("Максимальное количество фраз на ключ:", max(phrases_per_key))
print("Минимальное количество фраз на ключ:", min(phrases_per_key))
print("Общее количество уникальных фраз:", len(unique_phrases))

top_n = 10
print(f"\nТоп-{top_n} фраз по количеству вхождений:")
for phrase, count in sorted(phrase_occurrences.items(), key=lambda x: -x[1])[:top_n]:
    print(f"  {phrase}: {count}")