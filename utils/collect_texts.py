import requests
from bs4 import BeautifulSoup
import re
import os

def get_article_id(url):
    match = re.search(r'/article/(\d+)/', url)
    return match.group(1) if match else None

def parse_title(soup):
    title_tag = soup.select_one('h1.tm-title.tm-title_h1 > span')
    return title_tag.get_text(strip=True) if title_tag else 'No title found'

def parse_tags(soup):
    tags_container = soup.find('span', string='Теги:').find_next_sibling('ul', class_='tm-separated-list__list')
    if tags_container:
        return [tag.get_text(strip=True) for tag in tags_container.find_all('a', class_='tm-tags-list__link')]
    return []

def parse_hubs(soup):
    hubs_container = soup.find('span', string='Хабы:').find_next_sibling('ul', class_='tm-separated-list__list')
    if hubs_container:
        return [hub.get_text(strip=True) for hub in hubs_container.find_all('a', class_='tm-hubs-list__link')]
    return []

def save_data(file_path, data):
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(data)

def parse_article(url):
    response = requests.get(url)
    soup = BeautifulSoup(response.text, 'html.parser')

    article_id = get_article_id(url)
    if article_id:
        title = parse_title(soup)
        tags_text = parse_tags(soup)
        hubs_text = parse_hubs(soup)

        # TODO: Implement parse_text function
        # text = parse_text(soup)

        # Save title
        title_file_path = os.path.join(output_folder, f'art{article_id}_title.txt')
        save_data(title_file_path, title)
        
        # Save tags
        tags_file_path = os.path.join(output_folder, f'art{article_id}_tags.txt')
        save_data(tags_file_path, '\n'.join(tags_text))

        # Save hubs
        hubs_file_path = os.path.join(output_folder, f'art{article_id}_hubs.txt')
        save_data(hubs_file_path, '\n'.join(hubs_text))

        # Save text
        # text_file_path = os.path.join(output_folder, f'art{article_id}_text.txt')
        # save_data(text_file_path, text)
        
def main():
    with open('collected_links.txt', 'r') as file:
        count = 0
        for url in file:
            print(f"Started processing the article {url}")
            parse_article(url.strip())
            count +=1 # for tests
            if count == 5:
                return

output_folder = 'text_data'

if not os.path.exists(output_folder):
    os.makedirs(output_folder)

main()