import os

def collect_line_counts(data_folder):
    line_count = {}
    
    data_folder = os.path.join(data_folder, 'texts') 
    # Navigate to the data folder
    os.chdir(data_folder)
    
    # Loop through each file in the directory
    for filename in os.listdir():
        if filename.endswith('hubs.txt') or filename.endswith('tags.txt'):
            with open(filename, 'r') as file:
                # Read lines and count occurrences
                for line in file:
                    cleaned_line = line.strip()
                    if cleaned_line:
                        if cleaned_line in line_count:
                            line_count[cleaned_line] += 1
                        else:
                            line_count[cleaned_line] = 1

    # Navigate back to the initial utils folder
    os.chdir('..')
    return line_count

def save_counts_to_file(line_count, output_path):
    # Sort the dictionary by value (number of occurrences) in descending order
    sorted_line_count = sorted(line_count.items(), key=lambda item: item[1], reverse=True)
    
    with open(output_path, 'w') as file:
        for line, count in sorted_line_count:
            file.write(f"{line} {count}\n")

def main():
    utils_folder = os.path.dirname(__file__)  # Get the directory of the current script
    data_folder = os.path.join(utils_folder, '..', 'my_data') 
    
    # Collect line counts
    line_counts = collect_line_counts(data_folder)
    
    # Save counts to a file in my_data
    output_file_path = os.path.join(data_folder, 'tags_and_hubs_line_counts.txt')
    save_counts_to_file(line_counts, output_file_path)

if __name__ == "__main__":
    main()
