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

from bs4 import BeautifulSoup, Comment

def parse_text(soup):
    ignored_tags = ['script', 'style', 'iframe', 'button', 'input', 'textarea', 'figure', 'img']
    content_body = soup.find('div', id='post-content-body')
    text_blocks = []

    if content_body:
        for element in content_body.find_all(recursive=True):
            if element.name in ignored_tags or isinstance(element, Comment) or 'table' in [parent.name for parent in element.parents]:
                continue

            if isinstance(element, str):
                continue

            if element.name in ['p', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'li'] and not any(parent.name in ['li', 'details'] for parent in element.parents):
                clean_text = element.get_text(separator=' ', strip=True)
                if clean_text and clean_text[-1] not in '.!?:;':
                    clean_text += '.'
                text_blocks.append(clean_text)
    
    return '\n'.join(text_blocks)


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
        text = parse_text(soup)

        # Save title
        title_file_path = os.path.join(output_folder, f'art{article_id}_title.txt')
        save_data(title_file_path, title)
        
        # Save tags
        tags_file_path = os.path.join(output_folder, f'art{article_id}_tags.txt')
        save_data(tags_file_path, '\n'.join(tags_text))

        # Save hubs
        hubs_file_path = os.path.join(output_folder, f'art{article_id}_hubs.txt')
        save_data(hubs_file_path, '\n'.join(hubs_text))

        #Save text
        text_file_path = os.path.join(output_folder, f'art{article_id}_text.txt')
        save_data(text_file_path, text)
        
def main():
    with open(links_file_path, 'r') as file:
        count = 0
        for url in file:
            print(f"Started processing the article {url}")
            parse_article(url.strip())
            count +=1 # for tests
            if count == 20:
                return

current_script_dir = os.path.dirname(os.path.abspath(__file__))
links_file_path = os.path.join(current_script_dir, '..', 'my_data', 'collected_links.txt')
output_folder = os.path.join(current_script_dir, '..', 'my_data', 'texts')

if not os.path.exists(output_folder):
    os.makedirs(output_folder)

main()