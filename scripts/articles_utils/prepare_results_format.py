import os
from scripts.core.paths import CORPUS_DIR

directory_path = CORPUS_DIR / 'results'

def process_json_files(directory):
    for filename in os.listdir(directory):
        if filename.endswith(".txt"):  # Process only files with the .txt extension
            filepath = os.path.join(directory, filename)

            if os.path.getsize(filepath) == 0:
                os.remove(filepath)
                print(f"Deleted empty file: {filename}")
                continue

            with open(filepath, 'r', encoding='utf-8') as file:
                content = file.read().strip()   # Read the file content and strip any extra whitespace

            # Split the content into individual JSON objects based on lines containing "{"
            json_objects = content.split('}\n{')

            # Reconstruct the correct JSON object format
            for i in range(len(json_objects)):
                if i != 0:
                    json_objects[i] = '{' + json_objects[i]
                if i != len(json_objects) - 1:
                    json_objects[i] += '}'

            # Convert them into a JSON array, adding square brackets and commas between objects
            result = '[\n' + ',\n'.join(json_objects) + '\n]'

            # Determine the new path to save the file with the .json extension
            new_filepath = os.path.join(directory, f"{os.path.splitext(filename)[0]}.json")
            with open(new_filepath, 'w', encoding='utf-8') as output_file:
                output_file.write(result)

            # Remove the original .txt file
            os.remove(filepath)

            print(f"Converted {filename} to {os.path.basename(new_filepath)} and deleted the original .txt file.")

process_json_files(directory_path)
