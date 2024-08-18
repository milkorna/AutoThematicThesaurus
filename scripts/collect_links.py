import requests
from bs4 import BeautifulSoup

MY_KEY_WORD = "nlp"
BASE_URL = "https://habr.com/ru/search/page{}/?q=%5B{}%5D&target_type=posts&order=date"

def check_page_exists(page_number):
    url = BASE_URL.format(page_number, MY_KEY_WORD)
    response = requests.get(url)
    page_exists = response.status_code == 200
    if page_exists:
        print(f"A set of articles number {page_number} is being processed.")
    else:
        print(f"Failled to open {page_number} page, url = {url}.")
    return page_exists

def collect_links(page_number):
    url = BASE_URL.format(page_number, MY_KEY_WORD)
    response = requests.get(url)
    soup = BeautifulSoup(response.text, 'html.parser')
    
    articles = soup.find_all('article', class_='tm-articles-list__item')
    article_ids = [article['id'] for article in articles if 'id' in article.attrs]

    links = []
    for article_id in article_ids:
        article_url = f"https://habr.com/ru/article/{article_id}/"
        article_response = requests.get(article_url)
        
        if article_response.status_code == 200:
            links.append(article_url)
        else:
            print(f"Failled to open {len(links)} article from page number {page_number}, url = {article_url}.")

    print(f"Added {len(links)} articles from page number {page_number}.")
    return links

def main():
    page_number = 1
    print(page_number)
    all_links = []
    with open('collected_links.txt', 'w') as file:
        print("Started collecting links")
        while True:
            if not check_page_exists(page_number):
                break
            
            links = collect_links(page_number)
            all_links.extend(links)

            for link in links:
                file.write(link + '\n') 
            
            page_number += 1
    
    print("The program has finished running successfully.")
    return

main()