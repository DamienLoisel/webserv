1/ Socket serveur qui écoute les connexions entrantes
Ressource : 
https://ncona.com/2019/04/building-a-simple-server-with-cpp/

2/ Multiplexage des connexions avec select/poll/epoll/kqueue
https://www.youtube.com/watch?v=Y6pFtgRdUts&ab_channel=JacobSorber

3/ Parser HTTP pour analyser les requêtes

4/ Gestion des méthodes GET, POST, DELETE

5/ Configuration du serveur via fichier .conf

6/ Support des CGI

Ou cé qu'on en nez:


- Damien : Ajouter une redirection vers index.html
- Damien : CGI
- Damien : Conf file et socket, implémentation
- Damien et Damien : les code d'erreurs.



Les tests :


400 Bad Request
bash
CopyInsert
# Requête mal formée
curl -X GET "http://localhost:7000/\0"
# Requête avec en-tête invalide
curl -H "Content-Length: invalid" http://localhost:7000/
403 Forbidden
bash
CopyInsert
# Accès à un script CGI non exécutable
curl http://localhost:7000/cgi-bin/non-executable.py
# Tentative d'accès à un fichier en dehors du root
curl http://localhost:7000/../private/file.txt
404 Not Found
bash
CopyInsert
# Fichier inexistant
curl http://localhost:7000/not-exists.html
# CGI script inexistant
curl http://localhost:7000/cgi-bin/not-exists.py
405 Method Not Allowed
bash
CopyInsert
# DELETE sur une route qui ne l'autorise pas
curl -X DELETE http://localhost:7000/tours/
# POST sur une route qui n'accepte que GET
curl -X POST http://localhost:7000/index.html
413 Payload Too Large
bash
CopyInsert
# POST avec un body plus grand que client_max_body_size
curl -X POST -H "Content-Type: plain/text" --data "$(yes 'A' | head -c 2048)" http://localhost:7000/
500 Internal Server Error
bash
CopyInsert
# CGI script qui génère une erreur
curl http://localhost:7000/cgi-bin/error.py
# Script CGI avec une boucle infinie
curl http://localhost:7000/cgi-bin/infinite-loop.py
501 Not Implemented
bash
CopyInsert
# Méthode HTTP non supportée
curl -X PATCH http://localhost:7000/
curl -X CONNECT http://localhost:7000/
Tests de configuration
bash
CopyInsert
# Test de virtual hosts
curl --resolve example.com:7000:127.0.0.1 http://example.com:7000/

# Test d'autoindex
curl http://localhost:7000/tours/  # si autoindex est activé

# Test de redirection
curl -L http://localhost:7000/tours/  # si une redirection est configurée
Tests de limites


curl -X PATCH http://localhost:7000/
ça ça devrait faire dire au serveur  501 not implemented
Mais là ça donne 500 Internal Server Error



Et aussi à un moment j'ai reçu

[1]    156541 alarm      ./webserv example.confQu'est ce que ça pourrait vouloir dire ça ?EditCette erreur semble indiquer qu'un programme nommé "webserv" s'est terminé avec un signal d'alarme (SIGALRM) lors de son exécution avec le fichier de configuration "example.conf".
Décomposons cette ligne :

[1] : C'est l'identifiant du processus (PID)
156541 : Le numéro du processus spécifique
alarm : Indique que le programme s'est terminé à cause d'un signal d'alarme (SIGALRM)
./webserv : Le programme qui était en cours d'exécution
example.conf : Le fichier de configuration utilisé comme argument

Un signal d'alarme (SIGALRM) est généralement envoyé lorsqu'un minuteur programmé (timer) arrive à expiration. Cela peut se produire dans plusieurs situations :

Le programme a mis en place un timer avec alarm() ou setitimer() et ce timer a expiré
Une opération a pris trop de temps et un mécanisme de timeout s'est déclenché
Une boucle infinie ou un blocage dans le code

Pour résoudre ce problème, vous pourriez :

Vérifier s'il y a des boucles qui pourraient prendre trop de temps
Examiner la gestion des timers dans votre code
Regarder les logs du serveur pour plus de détails sur ce qui se passait au moment de l'erreur

Vous voulez qu'on regarde ensemble certaines parties spécifiques du code ?