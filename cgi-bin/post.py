#!/usr/bin/env python3
import sys
import os
import json

try:
    # Lire le corps de la requête
    content_len = int(os.environ.get('CONTENT_LENGTH', 0))
    body = sys.stdin.read(content_len)
    data = json.loads(body)
    name = data.get('name', '')

    # Lire le fichier HTML
    with open('birthday.html', 'r') as f:
        html_content = f.read()

    # Remplacer NAME par le nom fourni
    html_content = html_content.replace('NAME', name)

    # Envoyer la réponse exactement comme le fait le serveur pour les GET
    print("HTTP/1.1 200 OK")
    print("Content-Type: text/html")
    print(f"Content-Length: {len(html_content)}")
    print()
    print(html_content)

except Exception as e:
    print("HTTP/1.1 500 Internal Server Error")
    print("Content-Type: text/plain")
    print("Content-Length: 0")
    print()
