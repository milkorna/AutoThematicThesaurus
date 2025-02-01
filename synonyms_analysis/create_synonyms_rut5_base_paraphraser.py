import os
import json
import re
import torch
import inspect
import pandas as pd
import pymorphy2
from collections import Counter

from transformers import T5ForConditionalGeneration, T5Tokenizer

########################################
# 1) Настройки
########################################

PATH_FILTERED = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_data.xlsx"
PATH_BIGDATA = "/home/milkorna/Documents/AutoThematicThesaurus/data_with_oof.xlsx"
PATH_OUT_JSON = "/home/milkorna/Documents/AutoThematicThesaurus/synonyms_analysis/synonyms_rut5_base_paraphraser.json"

MODEL_NAME = "cointegrated/rut5-base-paraphraser"
NUM_BEAMS = 20                # Сколько beam'ов для поиска
NUM_RETURN_SEQUENCES = 20     # Максимальное количество парафраз
TOP_3 = 3                     # Сколько парафраз попадёт в "top_paraphrases"

MAX_LENGTH_FACTOR = 1.5       # Фактор для вычисления max_length (от длины входа)
GRAMS = 4                     # encoder_no_repeat_ngram_size=4 (рекомендуется)

DEVICE = "cuda" if torch.cuda.is_available() else "cpu"

# Патчим pymorphy2 для совместимости с Python 3.12+
if not hasattr(inspect, "getargspec"):
    def getargspec_patched(func):
        spec = inspect.getfullargspec(func)
        return spec.args, spec.varargs, spec.varkw, spec.defaults  # Совместимый формат
    inspect.getargspec = getargspec_patched


########################################
# 2) Инициализируем модель
########################################

def load_paraphraser_model():
    print(f"[INFO] Loading paraphraser model: {MODEL_NAME}")
    model = T5ForConditionalGeneration.from_pretrained(MODEL_NAME).to(DEVICE)
    tokenizer = T5Tokenizer.from_pretrained(MODEL_NAME)
    model.eval()
    return model, tokenizer

model, tokenizer = load_paraphraser_model()
print(f"[INFO] Model loaded on device: {DEVICE}")

morph = pymorphy2.MorphAnalyzer()

########################################
# 3) Функция генерации парафраз
########################################

def generate_paraphrases(text,
                         model,
                         tokenizer,
                         num_beams=NUM_BEAMS,
                         num_return_sequences=NUM_RETURN_SEQUENCES,
                         grams=GRAMS):
    """
    Генерирует до num_return_sequences парафраз фразы 'text'
    с beam search. Возвращает список строк (без дубликатов).
    """
    text = text.strip()
    if not text:
        return []

    # Преобразуем фразу для t5: обычно подход "paraphrase: <text>"
    # Но автор модели cointegrated/rut5-base-paraphraser
    # упоминает, что достаточно просто подать текст напрямую.
    input_ids = tokenizer.encode(text, return_tensors="pt").to(DEVICE)

    # Примерный max_length
    max_size = int(input_ids.shape[1] * MAX_LENGTH_FACTOR + 10)

    # Генерация
    with torch.no_grad():
        outputs = model.generate(
            input_ids,
            encoder_no_repeat_ngram_size=grams,
            num_beams=num_beams,
            num_return_sequences=num_return_sequences,
            max_length=max_size,
            do_sample=False,
            early_stopping=True
        )

    # Декодируем
    paraphrases = []
    for out in outputs:
        par = tokenizer.decode(out, skip_special_tokens=True).strip()
        paraphrases.append(par)

    # Уберём дубликаты
    paraphrases = list(dict.fromkeys(paraphrases))
    return paraphrases

ALLOWED_POS = {"NOUN", "PRTF", "PRTS", "ADJF", "ADJS", "PREP"}

def normalize_phrase(phrase: str) -> str:
    """
    Приводим строку к нижнему регистру,
    удаляем пунктуацию,
    лемматизируем слова,
    убираем лишние пробелы,
    проверяем, что часть речи у каждого токена из ALLOWED_POS.
    Дополнительно убеждаемся, что предлог не является первым или последним словом.
    """
    # 1) нижний регистр
    phrase = phrase.lower()

    # 2) убираем пунктуацию (любые символы кроме букв/цифр/пробелов)
    #    если нужны цифры, оставляем \d, если нет - убираем.
    phrase = re.sub(r"[^\w\s\d]+", "", phrase)

    # 3) разбиваем на слова
    tokens = phrase.split()

    lemmas = []
    pos_tags = []

    # 4) Лемматизируем и проверяем часть речи
    for t in tokens:
        p = morph.parse(t)[0]
        pos = p.tag.POS  # Часть речи
        if pos is None or pos in ALLOWED_POS:  # Если POS не определён — оставляем
            lemmas.append(p.normal_form)
            pos_tags.append(pos if pos else "UNKNOWN")  # Для отладки можно оставлять "UNKNOWN"


    # 5) Доп. проверка: предлог не должен быть первым или последним
    if not pos_tags:
        return ""

    if pos_tags[0] == "PREP" or pos_tags[-1] == "PREP":
        return ""

    # 6) Склеиваем леммы обратно
    normalized = " ".join(lemmas).strip()
    return normalized


########################################
# 4) Основной скрипт
########################################

def main():
    print("[INFO] Reading filtered data from:", PATH_FILTERED)
    df_filtered = pd.read_excel(PATH_FILTERED)

    print("[INFO] Reading big data from:", PATH_BIGDATA)
    df_big = pd.read_excel(PATH_BIGDATA)

    # Проверка наличия нужных колонок
    required_cols = {'key', 'is_term_manual', 'oof_prob_class'}
    if not required_cols.issubset(df_big.columns):
        print(f"[ERROR] df_big missing columns: {required_cols - set(df_big.columns)}")
        return

    # Словарь для быстрого доступа: phrase -> (is_term_manual, oof_prob_class)
    big_dict = {}
    for idx, row in df_big.iterrows():
        ph = str(row['key']).strip()
        is_term = int(row['is_term_manual'])
        prob = float(row['oof_prob_class'])
        big_dict[ph] = (is_term, prob)

    results = {}

    # Обрабатываем каждую строчку из filtered_data
    for idx, row in df_filtered.iterrows():
        # Читаем key, is_term_manual, oof_prob_class
        original_phrase = str(row.get('key', '')).strip()
        if not original_phrase:
            continue

        # Если в big_dict нет такой фразы, пропускаем
        if original_phrase not in big_dict:
            continue

        is_term, prob = big_dict[original_phrase]

        # Генерация до 20 парафраз
        all_paraphrases = generate_paraphrases(original_phrase, model, tokenizer)

        if not all_paraphrases:
            # Если вообще не получилось парафраз => skip
            continue

        cleaned_phrases = []
        for p in all_paraphrases:
            cp = normalize_phrase(p)
            if cp and cp != original_phrase and cp not in cleaned_phrases:
                words = cp.split()
                word_counts = Counter(words)  # Подсчитываем количество каждого слова
                if all(count == 1 for count in word_counts.values()):  # Проверяем, что все слова уникальны
                    cleaned_phrases.append(cp)

        if not cleaned_phrases:
            continue

        # Возьмём первые TOP_3
        top_paraphrases = cleaned_phrases[:TOP_3]

        # Проверим, какие из 20 (или меньше) есть в big_dict
        found_in_data = []
        for par in cleaned_phrases:
            if par in big_dict:
                # берём is_term_manual + prob из big_dict
                st, sp = big_dict[par]
                found_in_data.append({
                    "key": par,
                    "is_term_manual": st,
                    "oof_prob_class": sp
                })

        if not found_in_data:
            # Если ничего не найдено => не включаем в результат
            continue

        results[original_phrase] = {
            "key": original_phrase,
            "is_term_manual": is_term,
            "oof_prob_class": prob,
            "top_paraphrases": top_paraphrases,
            "found_in_data": found_in_data
        }

    # Сохраняем в JSON
    if not results:
        print("[INFO] No paraphrases found in data_with_oof.")
    else:
        print(f"[INFO] {len(results)} phrases had paraphrases found in data_with_oof.")

    print(f"[INFO] Saving final JSON to {PATH_OUT_JSON}")
    with open(PATH_OUT_JSON, 'w', encoding='utf-8') as f:
        json.dump(results, f, ensure_ascii=False, indent=4)

    print("[INFO] Done.")

if __name__ == "__main__":
    main()
