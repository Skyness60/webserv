#!/usr/bin/python3

import os
import sys
import urllib.parse

print("Content-Type: text/html; charset=utf-8\n")  # Ajout de charset=utf-8

# Lire la longueur des données envoyées en POST
length = int(os.environ.get("CONTENT_LENGTH", 0))

# Lire exactement cette quantité d’octets depuis stdin
data = sys.stdin.read(length)

# Parser les données : "name=John&message=Hello"
params = urllib.parse.parse_qs(data)

# Affichage HTML
print('<html lang="fr"><body><h1>Données POST reçues:</h1>')
print(f"<p>Nom: {params.get('name', [''])[0]}</p>")
print(f"<p>Message: {params.get('message', [''])[0]}</p>")
print("</body></html>")
