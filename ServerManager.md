# Comprendre ServerManager et les notions de base (socket, bind, epoll)

## ✨ Objectif du fichier `ServerManager.cpp`
Ce fichier contient la classe `ServerManager`, qui est responsable de :
- Lire la configuration depuis un fichier
- Créer un ou plusieurs serveurs (en écoutant sur des ports donnés)
- Gérer les connexions clients en utilisant `epoll` (pour la performance)

---

## 🔍 Notions de base

### 7 `socket()`
Crée une "prise réseau", comme une porte d'entrée pour recevoir des connexions.

```cpp
int fd = socket(AF_INET, SOCK_STREAM, 0);
```
- `AF_INET` : IPv4
- `SOCK_STREAM` : mode TCP
- Retourne un identifiant de fichier (fd), utilisé pour manipuler la socket

---

### 🔗 `bind()`
Associe la socket à un port pour que les clients sachent où se connecter.

```cpp
bind(fd, (struct sockaddr *)&address, sizeof(address));
```
- Si on ne fait pas le `bind`, le serveur ne saura pas sur quel port recevoir les connexions.

---

### 🔊 `listen()`
Dit au système d'écouter les connexions entrantes sur la socket.

```cpp
listen(fd, 10);
```
- Le `10` est la taille de la file d'attente pour les connexions en attente.

---

### 🔄 `epoll`
Permet de gérer plein de connexions clients efficacement (au lieu de `select()` ou `poll()`).

- `epoll_create1(0)` : crée un gestionnaire d'événements
- `epoll_ctl(...)` : ajoute une socket à la liste des sockets à surveiller
- `epoll_wait(...)` : attend qu'une ou plusieurs sockets aient des événements (comme "des données sont arrivées")

---

## 🔢 Détail des fonctions de `ServerManager`

### Constructeurs / destructeur
```cpp
ServerManager(std::string filename)
ServerManager(const ServerManager &copy)
ServerManager &operator=(const ServerManager &copy)
~ServerManager()
```
- Stocke le nom du fichier de config
- Initialise l'objet `_config`
- Appelle `loadConfig()`

---

### `void loadConfig()`
Charge le fichier de configuration (actuellement commenté).
Permettrait d'afficher ou vérifier le contenu du fichier.

---

### `void startServer()` ⚡
Cette fonction fait tout le travail important :

1. **Lecture de la config**
   - Récupère la valeur du champ `listen` pour chaque serveur

2. **Création de socket**
   - `socket()` + `setsockopt()` pour réutiliser le port si besoin

3. **Association à une adresse IP + port**
   - `bind()` puis `listen()`

4. **Création de l'instance epoll**
   - `epoll_create1(0)`

5. **Ajout des sockets serveurs à epoll**
   - Chaque socket est surveillée pour savoir quand un client veut se connecter

6. **Boucle principale (infinie)**
   - `epoll_wait()` attend les événements
   - Si l'événement vient d'une socket serveur, on appelle `accept()` pour accepter le client
   - Si l'événement vient d'un client :
     - On lit les données
     - On tente de parser la requête HTTP
     - On gère la réponse

---

### `getConfigValue(int server, std::string key)`
Retourne une valeur unique depuis la config pour un serveur donné.
Ex : `getConfigValue(0, "listen")` ➞ "8080"

### `getConfigValues()`
Retourne toutes les clés/valeurs globales de tous les serveurs.

### `getLocationValues()`
Retourne les valeurs des blocs `location {}` pour chaque serveur.

---

## ✨ En résumé
- `socket()` : crée une porte d'entrée
- `bind()` : définit l'adresse (IP:port)
- `listen()` : commence à écouter
- `epoll` : gère plein de connexions sans ralentir
- `ServerManager` : lit la config, crée les serveurs, accepte les clients, traite leurs requêtes

---

