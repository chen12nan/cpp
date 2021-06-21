# import urllib2

import requests
import os



def xiaodai():
    url = "https://quote.fx678.com/symbol/XAU"

    proxies = {
        'http': 'http://149.28.38.64:1081',
        'https': 'https://149.28.38.64:1081'
    }

    # headers = {
    #     'User-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.163 Safari/537.36'
    # }

    headers = { 'User-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.90 Safari/537.36 Edg/89.0.774.57'}
    try:
        response = requests.get (url, headers=headers)
        fd = open("price_html.xml", "w", encoding='utf-8')
        # print(type(response.text))
        fd.write(response.text)
        fd.close()
        # print(response.text)
    except Exception as e:
        print(e)

if __name__ == '__main__':
    xiaodai()
