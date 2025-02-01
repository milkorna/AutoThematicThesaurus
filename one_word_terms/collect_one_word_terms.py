import json
import pandas as pd
import pymorphy2
import inspect
import re
import math

# Патчим pymorphy2 для совместимости с Python 3.12+
if not hasattr(inspect, "getargspec"):
    def getargspec_patched(func):
        spec = inspect.getfullargspec(func)
        return spec.args, spec.varargs, spec.varkw, spec.defaults  # Совместимый формат
    inspect.getargspec = getargspec_patched

# Пути к файлам
json_path = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/nlp_corpus/filtered_corpus.json"
excel_path = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_data.xlsx"
sentences_path = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/nlp_corpus/sentences.json"
output_path = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_one_word_terms.xlsx"

# Инициализация морфологического анализатора
morph = pymorphy2.MorphAnalyzer()

# Функция для проверки, является ли слово существительным
def is_noun(word):
    parsed = morph.parse(word)[0]  # Берем первый разбор
    return 'NOUN' in parsed.tag

# Загрузка JSON с логированием
try:
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    print("JSON-файл успешно загружен")
except json.JSONDecodeError as e:
    print(f"Ошибка при разборе JSON: {e}")
    exit(1)
except Exception as e:
    print(f"Ошибка при загрузке JSON: {e}")
    exit(1)

# Загрузка предложений
try:
    with open(sentences_path, "r", encoding="utf-8") as f:
        sentences_data = json.load(f)
    print("Sentences JSON-файл успешно загружен")
except json.JSONDecodeError as e:
    print(f"Ошибка при разборе JSON: {e}")
    exit(1)
except Exception as e:
    print(f"Ошибка при загрузке JSON: {e}")
    exit(1)

# Группируем предложения по docNum
documents = {}
for sentence in sentences_data["sentences"]:
    doc_id = sentence["docNum"]
    normalized_text = sentence["normalizedStr"]
    if doc_id not in documents:
        documents[doc_id] = []
    documents[doc_id].append(normalized_text)

# Объединяем предложения в текст документа
doc_texts = {doc_id: " ".join(sentences) for doc_id, sentences in documents.items()}
N = len(doc_texts)  # Общее количество документов

# Извлекаем слова из 4_wordFrequency с частотой > 2
try:
    word_freq = data["4_wordFrequency"]
    nouns = {word for word, freq in word_freq.items() if freq > 2 and is_noun(word) and len(word) > 2}
    print(f"Извлечено {len(nouns)} существительных")
except KeyError as e:
    print(f"Ошибка: отсутствует ключ {e} в JSON")
    exit(1)

# Загрузка данных из filtered_data.xlsx с логированием
try:
    filtered_data = pd.read_excel(excel_path, usecols=["key", "oof_prob_class"])
    print("Excel-файл успешно загружен")
except Exception as e:
    print(f"Ошибка при загрузке Excel-файла: {e}")
    exit(1)

# Извлекаем все нормализованные слова из key
normalized_words = set()
word_counts = {}
word_prob_stats = {}
try:
    for _, row in filtered_data.dropna().iterrows():
        words = re.findall(r'\b\w+\b', str(row["key"]))
        normalized_words.update(words)
        for word in words:
            word_counts[word] = word_counts.get(word, 0) + 1
            if word not in word_prob_stats:
                word_prob_stats[word] = []
            word_prob_stats[word].append(row["oof_prob_class"])
    print(f"Извлечено {len(normalized_words)} нормализованных слов из key")
except Exception as e:
    print(f"Ошибка при обработке key в Excel: {e}")
    exit(1)

# Фильтрация существительных по наличию в key
valid_nouns = nouns & normalized_words

# Подсчет TF-IDF
idf = {word: math.log(N / (1 + sum(1 for doc in doc_texts.values() if word in doc))) for word in valid_nouns}
tf_idf = []
for word in sorted(valid_nouns):
    prob_values = word_prob_stats.get(word, [0])
    prob_mean = sum(prob_values) / len(prob_values)
    prob_max = max(prob_values)
    prob_min = min(prob_values)
    phrase_count = word_counts.get(word, 0)
    if phrase_count > 1:  # Исключаем слова с phrase_count == 1
        tf_idf.append((word, phrase_count, idf[word], prob_mean, prob_max, prob_min))

print(f"Финальный список существительных: {len(tf_idf)} слов")

# Сохранение результата с логированием
try:
    df = pd.DataFrame(tf_idf, columns=["noun", "phrase_count", "tf_idf", "prob_mean", "prob_max", "prob_min"])
    df.to_excel(output_path, index=False)
    print(f"Фильтрованный список существительных сохранен в {output_path}")
except Exception as e:
    print(f"Ошибка при сохранении Excel-файла: {e}")
    exit(1)
