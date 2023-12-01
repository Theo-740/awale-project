# Fonctionnalités implémentées :
    - partie d'awale 1 vs 1 (abandon possible)
    - reconnection à une partie en cours en cas de déconnexion de l'utilisateur
    - liste des utilisateurs connectés et leurs états (libre, en partie...)
    - defier un utilisateur connecté et libre
    - observation d'une partie en cours (il est possible d'avoir plusieurs observateurs d'une même partie)
    - partie terminées stockées dans une liste (mais pas de moyen de consultation)

Il n'y a pas de persistance entre les différents lancements du serveur.

# Utilisation :
    Pour compiler les executables faire un "make" à la racine du projet (il est possible de compiler en mode debug en faisant "make debug").
    executable serveur : server.exe
    executable client : client.exe

# Manuel utilisateur :
## Menu principal :
    '0' : affichage de l'ensemble des utilisateurs connectés
    '1' : affichage de l'ensemble des parties en cours

## Liste des utilisateurs connectés :
    nombre : défier l'utilisateur correspondant à ce nombre
    'q' : retour au menu

## Défi en cours :
    'q' : annulation du défi et retour au menu

## Demande de défi :
    '1' : accepter le défi
    '2' : refuser le défi et retour au menu

## Partie en cours :
    'w' : abandonner la partie, l'adversaire gagne, retour au menu
    nombre : jouer un coup si c'est votre tour

## Liste des parties en cours :
    nombre : observer la partie correspondant à ce nombre
    'q' : retour au menu

## Observation de partie :
    'q' : arrêter d'observer, retour au menu