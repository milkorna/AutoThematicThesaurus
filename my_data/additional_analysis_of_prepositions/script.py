# Function to process each line of the file
def process_line(line):
    # Remove characters up to and including the first "||"
    parts = line.split('||', 1)
    if len(parts) > 1:
        line = parts[1]
    else:
        line = parts[0]

    # Remove all characters after the next "||"
    line = line.split('||')[0]

    # Remove leading and trailing whitespace from the line
    line = line.strip()

    return line

# Function to classify the processed line into appropriate lists
def classify_line(line, lists):
    # Keywords to search for in the lines
    keywords = [" на ", " в ", " для ", " до ", " от ", " из ", " при ", " с ", " у ", " к "]
    found = False
    # Check if the line contains any of the keywords and add it to the corresponding list
    for i, keyword in enumerate(keywords):
        if keyword in line:
            lists[i].append(line)
            found = True
            break
    # If the line does not contain any of the keywords, add it to the 'others' list
    if not found:
        lists[-1].append(line)

# Function to read and process the file
def process_file(file_path):
    # Create ten lists for keywords and one for others
    lists = [[] for _ in range(11)] # 10 keyword lists + 1 'others' list

    # Open the file and read it line by line
    with open(file_path, 'r', encoding='utf-8') as file:
        for line in file:
            # Process each line according to the specified rules
            processed_line = process_line(line)
            # Classify the processed line into the appropriate list
            classify_line(processed_line, lists)

    return lists

# Function to write the classified lists to corresponding files
def write_lists_to_files(lists):
    # Write each keyword list to a separate file
    for i in range(10):
        with open(f'list_{i + 1}.txt', 'w', encoding='utf-8') as file:
            for item in lists[i]:
                file.write(item + '\n')

    # Write the 'others' list to a separate file
    with open('others.txt', 'w', encoding='utf-8') as file:
        for item in lists[-1]:
            file.write(item + '\n')

file_path = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/additional analysis/sample.txt'  # Óêàæèòå ïóòü ê âàøåìó ôàéëó
lists = process_file(file_path)
write_lists_to_files(lists)

print("done")
