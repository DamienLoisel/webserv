#!/usr/bin/env python3
import cgi
import os

def draw_unicorn():
    # Création d'une licorne simple en pixel art
    unicorn = [
        "    * ",
        "   /))",
        " _//__)",
        "( (@>  ",
        " `~~'  "
    ]
    return "\n".join(unicorn)

def main():
    # En-têtes HTTP nécessaires pour le CGI
    print("Content-Type: text/html\n")
    
    # Récupération des données POST
    form = cgi.FieldStorage()
    
    # Création de la licorne
    unicorn_art = draw_unicorn()
    
    # Vérification si une requête POST a été faite
    if os.environ['REQUEST_METHOD'] == 'POST':
        # Sauvegarde dans un fichier
        with open('unicorn.txt', 'w') as f:
            f.write(unicorn_art)
        response = "Licorne sauvegardée avec succès!"
    else:
        response = "Utilisez une requête POST pour sauvegarder la licorne."
    
    # Création de la page HTML de réponse
    html = f"""
    <!DOCTYPE html>
    <html>
        <head>
            <title>Licorne Pixel Art</title>
        </head>
        <body>
            <pre>{unicorn_art}</pre>
            <p>{response}</p>
            <form method="POST">
                <input type="submit" value="Sauvegarder la licorne">
            </form>
        </body>
    </html>
    """
    print(html)

if __name__ == "__main__":
    main()
