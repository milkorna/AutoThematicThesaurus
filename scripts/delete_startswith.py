import json

# Путь к файлу
file_path = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/final_relations_manual_check.json"

i=1

# Функция для загрузки JSON
def load_json(path):
    with open(path, "r", encoding="utf-8") as file:
        return json.load(file)

# Функция для сохранения JSON
def save_json(path, data):
    with open(path, "w", encoding="utf-8") as file:
        json.dump(data, file, ensure_ascii=False, indent=4)

# Фильтрация данных
def filter_data(data):
    filtered_data = []
    for entry in data:
        if entry["key"].startswith("танцующий "):
            continue

        filtered_phrases = [p for p in entry["phrases"] if not p["phrase"].startswith("танцующий ")]
        entry["phrases"] = filtered_phrases

        filtered_data.append(entry)
    return filtered_data

if __name__ == "__main__":
    data = load_json(file_path)
    filtered_data = filter_data(data)
    save_json(file_path, filtered_data)
    print("Файл успешно обновлен.")

# вызов
#   остановилась на {
        # "key": "база данный для извлечение информация",
        # "phrases": [

модель классификация для данный задача
наглядный
план

            {
                "phrase": "подробный классификация метод",
                "is_synonym": false,
                "is_usage_variant": false,
                "is_related": true,
                "is_hypernym": false,
                "is_hyponym": false
            },
изощренный
предлагаемый

            {
                "phrase": "предлагаемый метод",
                "is_synonym": false,
                "is_usage_variant": false,
                "is_related": true,
                "is_hypernym": false,
                "is_hyponym": false
            },
            {
                "phrase": "привычный метод",
                "is_synonym": false,
                "is_usage_variant": false,
                "is_related": true,
                "is_hypernym": false,
                "is_hyponym": false
            },
            {
                "phrase": "проверенный метод",
                "is_synonym": false,
                "is_usage_variant": false,
                "is_related": true,
                "is_hypernym": false,
                "is_hyponym": false
            },
прежний
принимающий
вектор в результирующий вектор
получение
рассматриваемый
{
                "phrase": "результирующий vektor для word",
                "is_synonym": false,
                "is_usage_variant": false,
                "is_related": false,
                "is_hypernym": false,
                "is_hyponym": false
            },
            {
                "phrase": "результирующий vektor word",
                "is_synonym": false,
                "is_usage_variant": false,
                "is_related": false,
                "is_hypernym": false,
                "is_hyponym": false
            },
одноименный
желаемый
мерный?
получившийся
предоставленный
представляющий