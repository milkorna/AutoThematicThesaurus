import json
from pathlib import Path


def process_ruttermeval_with_stats(input_file, output_file):
    """
    Версия с подробной статистикой, выходной файл в формате JSON (не JSONL)
    Два отдельных поля для word_count: с заголовком и без
    """

    stats = {
        "total": 0,
        "with_keywords": 0,
        "with_labels": 0,
        "empty_text": 0,
        "no_title": 0,
        "errors": 0,
    }

    documents = []  # Собираем все документы в один список

    with open(input_file, "r", encoding="utf-8") as infile:

        for line_num, line in enumerate(infile, 1):
            try:
                original = json.loads(line.strip())
                stats["total"] += 1

                # Разделяем текст
                raw_text = original.get("text", "")
                if not raw_text:
                    stats["empty_text"] += 1
                    continue

                # Проверяем наличие заголовка (текста до \n)
                if "\n" in raw_text:
                    title, main_text = raw_text.split("\n", 1)
                else:
                    title = ""
                    main_text = raw_text
                    stats["no_title"] += 1

                # Подсчет слов
                word_count_without_title = len(main_text.split()) if main_text else 0
                word_count_with_title = len(raw_text.split())

                # Статистика
                if original.get("keywords"):
                    stats["with_keywords"] += 1

                labels = original.get("label", [])
                if labels:
                    stats["with_labels"] += 1

                # Обработка labels
                processed_labels = [
                    {"substring": raw_text[start:end], "span": [start, end]}
                    for start, end in labels
                ]

                output_record = {
                    "doc_id": original["id"],
                    "title": title,
                    "source": "RuTermEval",
                    "word_count": word_count_without_title,
                    "word_count_with_title": word_count_with_title,
                    "text": main_text,
                    "keywords": original.get("keywords", ""),
                    "label": processed_labels,
                }

                documents.append(output_record)

            except Exception as e:
                stats["errors"] += 1
                print(f"Ошибка на строке {line_num}: {e}")
                continue

    # Сохраняем в JSON файл с красивым форматированием
    output_json = {
        "metadata": {
            "total_documents": stats["total"],
            "documents_with_keywords": stats["with_keywords"],
            "documents_with_labels": stats["with_labels"],
            "documents_without_title": stats["no_title"],
            "empty_documents": stats["empty_text"],
            "errors": stats["errors"],
            "successful": stats["total"] - stats["errors"],
        },
        "documents": documents,
    }

    with open(output_file, "w", encoding="utf-8") as outfile:
        json.dump(output_json, outfile, ensure_ascii=False, indent=2)

    # Выводим статистику
    print("\n===== СТАТИСТИКА ОБРАБОТКИ =====")
    print(f"Всего обработано:            {stats['total']}")
    print(f"С ключевыми словами:         {stats['with_keywords']}")
    print(f"С labels:                    {stats['with_labels']}")
    print(f"Без заголовка (no title):    {stats['no_title']}")
    print(f"Пустых текстов:              {stats['empty_text']}")
    print(f"Ошибок:                      {stats['errors']}")
    print(f"Успешно обработано:          {stats['total'] - stats['errors']}")
    print("=" * 45)


# ИСПОЛЬЗОВАНИЕ:

if __name__ == "__main__":
    input_path = "RuTermEval.jsonl"  # укажите ваш файл
    output_path = "RuTermEval_processed.json"  # теперь JSON

    process_ruttermeval_with_stats(input_path, output_path)

    print(f"\nФайл сохранен: {output_path}")
