présentation :
l'application permet de calculer le produit de deux matrices d’entiers positifs A(n, m)et B(m, p)
via un serveur qui récupère les requêtes des clients et gère les calculs. Elle utilise des mécanismes
de communication inter-processus tels que :
• Tubes nommés pour transmettre les requêtes.
• Sémaphores pour synchroniser les accès.
• Mémoire partagée pour le stockage des matrices et des résultats
compilation :
Sur un système compatible POSIX (Linux, macOS, etc.), vous pouvez compiler l'application à l'aide
du makefile fourni dans le projet.
- Ouvrez un terminal et exécutez la commande suivante : make
- Cette commande génère deux exécutables :
• serveur : le programme serveur.
• client : le programme client
utilisation :
1. démarrer le serveur : sur un terminal ouvert , lancer la commande ./serveur , le serveur reste en
attente de requête
2. sur un autre terminal ouvert lancer la commande « ./client <n> <m> <p> <borne> » avec les
paramètres :
- <n> : Nombre de lignes de A
- <m> : Nombre de colonnes de A et lignes de B
- <p> : Nombre de colonnes de B
-<borne> : Borne supérieure des éléments des matrices
- Vous pouvez envoyer plusieurs requêtes simultanément en utilisant la syntaxe suivante :
./client <n> <m> <p> <borne> & ./client <n2> <m2> <p2> <borne2> …
3. les matrices A , B et le résultat du calcule matriciel sera affiché dans client
