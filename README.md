# TP Web Service REST — kit de démarrage (C++20, Boost.Beast, Boost.JSON)

TP Réalisé par Eya MEZZI et Sarah MOSTAFAVI

## Dépendances

Ubuntu / Debian (Boost ≥ 1.81) :

    sudo apt install build-essential cmake curl libboost-dev libboost-json-dev

## Compilation et exécution

    cmake -S . -B build
    cmake --build build
    ./build/server            # écoute sur http://127.0.0.1:8080
    ./build/server 9000       # autre port

Dans un second terminal :

    curl -i http://localhost:8080/version
    ./test.sh                 # relancer le serveur avant chaque passage

## Contenu

- `src/` : sources du kit ; les fichiers à compléter contiennent des `TODO`
  (`ServicesManager.cpp`, `VersionService.cpp`, `UserService.cpp`, `main.cpp`).
- `dia/classes.dia` : diagramme UML des classes (éditable avec Dia).
- `test.sh` : tests de validation (curl), étiquetés par question.


## Objectif du TP 

L'objectif du TP est de réaliser une API REST CRUD  (Create / Read / Update / Delete).
L'API doit respecter les conventions **RFC 9110**.


Elle permet de gérer une mini base d'utilisateurs, décrits par deux attributs :
un nom (`name`) et un âge (`age`). Chaque utilisateur est identifié par un `id`
entier, attribué automatiquement par la base lors de sa création.

## Cahier des charges

Elle doit permettre : 

| Opération| Méthode HTTP | URL | Corps envoyé | Réponse attendue |
|----------------|--------------|-----|--------------|------------------|
| Lire les informations d'un utilisateur à partir de son id| GET | `/user/<id>` | — | 200 OK + l'utilisateur |
| Créer un nouvel utilisateur | POST | `/user` | l'utilisateur | 201 Created + `{"id": …}` |
| Modifier des champs d'un utilisateur à partir de son id| PATCH | `/user/<id>` | les champs à changer | 204 No Content |
| Supprimer un utilisateur à partir de son id| DELETE | `/user/<id>` | — | 204 No Content |


## Déroulé du TP

### **Préparations**

Nous avons récupéré le kit de démarrage sur Moodle et installé les dépendances
Debian nécessaires à la compilation (Boost, CMake, curl). Le projet compile et
le serveur démarre sur le port 8080.

Avant toute implantation, une requête sur `/version` renvoie :

    HTTP/1.1 501 Not Implemented
    {"error":"Not implemented yet"}

car les fonctions du kit sont encore des squelettes.

### Gestionnaire de version

#### Version 1

On souhaite d'abord récupérer la version de l'API, de manière à ce que si l'on place l'URL `http://localhost:8080/version` dans un navigateur, ou si l'on exécute :

    curl http://localhost:8080/version

on obtienne :

    {"major":1,"minor":0}

On veut donc accéder à la version par une requête GET sur l'URL `/version`. Pour une première version, nous avons uniquement modifié la méthode `queryService()` de la classe `ServicesManager`, le gestionnaire de services. C'est elle qui détermine ce qu'il faut exécuter à partir de l'URL et de la méthode HTTP de la requête.

Cette méthode vérifie que l'URL est `/version` (sinon elle renvoie une erreur 404) et que la méthode est GET (sinon une erreur 405, la méthode n'étant pas supportée par cette ressource). Elle construit ensuite l'objet JSON `{"major":1,"minor":0}` avec Boost.JSON, le convertit en texte dans la réponse et renvoie le statut 200. Les erreurs sont signalées en levant une `ServiceException`, que le serveur intercepte pour renvoyer le statut correspondant au client.

Nous avons vérifié le résultat avec `curl` :

- `curl -i http://localhost:8080/version` renvoie le JSON attendu avec le statut 200 ;
- `curl -i http://localhost:8080/autre` (une autre URL) renvoie une erreur 404 ;
- `curl -i -X POST http://localhost:8080/version` (une méthode autre que GET) renvoie, dans cette première version, une erreur 405.

#### Version 2 : la classe `VersionService`

Dans cette deuxième version, la logique du service de version n'est plus écrite directement dans le gestionnairelle mais elle est déplacée dans `VersionService` qui hérite de `AbstractService`. Chaque service est ainsi associé à un motif d'URL (ici `/version`), et le gestionnaire n'a plus qu'à le retrouver et à l'appeler.

Nous avons implanté la méthode `VersionService::get()`. Contrairement à la première version, elle ne produit pas de texte, mais remplit un objet JSON (`json::value`) passé en argument avec `{"major":1,"minor":0}`, puis renvoie le statut 200. La conversion en texte sera faite dans `ServicesManager`. L'identifiant reçu en argument est ignoré, car ce service ne gère aucun élément.

#### Version 3 : enregistrement et recherche des services

Pour utiliser `VersionService`, le gestionnaire doit pouvoir mémoriser les services et retrouver le bon à partir de l'URL. Nous avons complété trois méthodes de `ServicesManager` et ajouté une ligne dans `main()` :

- `registerService()` ajoute un service à la liste, en le déplaçant avec `std::move` car un `std::unique_ptr` ne peut pas être copié.
- `findService()` renvoie le service dont le motif correspond au début de l'URL, ou `nullptr` sinon. Le motif doit être suivi de la fin de l'URL ou d'un `/` : `/version/23` correspond, mais `/versions` non.
- `queryService()` retire la query string, trouve le service (404 s'il n'existe pas), appelle sa méthode `get()` puis convertit le JSON en texte.
- Dans `main()`, on enregistre une instance de `VersionService` avec `std::make_unique`.

Nous avons vérifié avec `curl` : `/version`, `/version?x=1` et `/version/23` renvoient `{"major":1,"minor":0}` (200), alors que `/versions` et `/autre` renvoient une erreur 404.

Le gestionnaire ne connaît plus le contenu du service : on peut donc ajouter d'autres services, comme `/user`, sans le modifier.

### Service utilisateur

#### Opération GET : lire un utilisateur

### Service utilisateur

#### Opération GET : lire un utilisateur

Pour lire un utilisateur, il faut extraire l'identifiant de l'URL, qui a la forme `<motif>/<identifiant>` (par exemple `/user/1`). Nous avons modifié `queryService()` pour décoder cet identifiant dans un `std::optional<int>`. Il reste vide s'il n'y a pas d'identifiant (`/user` ou `/user/`).

Le texte situé après le motif et le `/` est converti avec `std::from_chars` et ne lève pas d'exception. La conversion échoue si aucun nombre n'est lu (`abc`) ou s'il reste des caractères après le nombre (`1x`). Dans ces cas, on renvoie une erreur 400.

Nous avons ensuite remplacé le test sur la méthode par un `switch` sur `http::verb` dans `queryService()`. Pour l'instant, seule la méthode GET est traitée : on appelle le service trouvé avec l'identifiant décodé. Les autres méthodes renvoient une erreur 501 (Not Implemented). Ce comportement remplace le 405 de la première version.

Enfin, nous avons implanté `UserService::get()`. Elle cherche l'utilisateur dans la base avec son identifiant. Si l'identifiant est inconnu, elle renvoie une erreur 404. Sinon, elle convertit l'utilisateur en JSON avec `json::value_from()` et renvoie le statut 200. Une instance de `UserService` est enregistrée dans `main()`, en lui donnant la base d'utilisateurs en argument.

Nous avons vérifié avec `curl` :

- `/user/1` renvoie `{"name":"Paul","age":23}` (200) ;
- `/user/99` renvoie une erreur 404 ;
- `/user/abc` et `/user/1x` renvoient une erreur 400 ;
- `DELETE /version` renvoie une erreur 405.


#### Opération POST : créer un utilisateur

Pour créer un utilisateur, on envoie son nom et son âge en JSON avec une requête POST sur `/user`. Dans `queryService()`, nous avons ajouté le cas `post`. Le corps de la requête est analysé avec `json::parse()` sans exception : un corps absent ou invalide renvoie une erreur 400. Un POST vise la collection, donc `POST /user/3` renvoie une erreur 405.

Nous avons ensuite implanté `UserService::post()`. Elle vérifie que le corps est un objet JSON avec un `name` (chaîne non vide) et un `age` (entier entre 0 et 150). Toute donnée incorrecte renvoie une erreur 400. Sinon, elle ajoute l'utilisateur dans la base, renvoie son identifiant dans `{"id":…}` et le statut 201.

Nous avons vérifié avec `curl` :

- `POST /user` avec `{"name":"Jean","age":44}` renvoie `{"id":2}` (201) ;
- `GET /user/2` renvoie ensuite `{"name":"Jean","age":44}` ;
- un corps invalide, un nom vide, un âge hors limites ou un champ manquant renvoient une erreur 400 ;
- `POST /user/3` renvoie une erreur 405.

#### Opération PATCH : modifier un utilisateur

Pour modifier un utilisateur, on envoie uniquement les champs à changer avec une requête PATCH sur `/user/<id>`. Dans `queryService()`, nous avons ajouté le cas `patch`. Comme PATCH ne renvoie pas de données, la réponse est un 204 sans corps : `out` reste vide dans ce cas.

Dans `UserService::patch()`, on renvoie une erreur 405 sans identifiant et une erreur 404 si l'identifiant est inconnu. On travaille ensuite sur une copie de l'utilisateur, où l'on modifie les champs présents dans le corps. Un champ inconnu ou une valeur incorrecte renvoie une erreur 400. La copie remplace l'utilisateur seulement à la fin : la modification est donc atomique.

Nous avons vérifié avec `curl` :

- `PATCH /user/1` avec `{"age":24}` renvoie 204, et `GET /user/1` montre le nouvel âge ;
- un utilisateur inconnu renvoie une erreur 404 ;
- un champ inconnu ou un âge négatif renvoient une erreur 400, sans modifier l'utilisateur ;
- `PATCH /user` renvoie une erreur 405.

#### Opération DELETE : supprimer un utilisateur

Nous avons ajouté le cas `delete_` dans `queryService()`. `UserService::remove()` renvoie une erreur 405 sans identifiant et une erreur 404 si l'identifiant est inconnu. Sinon, elle supprime l'utilisateur et renvoie 204.

Nous avons vérifié avec `curl` :

- `DELETE /user/2` renvoie 204, puis `GET /user/2` renvoie une erreur 404 ;
- un second `DELETE /user/2` renvoie une erreur 404 ;
- `DELETE /user` renvoie une erreur 405.

Le script `./test.sh` permet de valider l'ensemble de ces cas.

### Pour aller plus loin

#### Liste des utilisateurs (Q13)

Nous avons modifié `UserService::get()` pour que `GET /user`, sans identifiant, renvoie la liste de tous les utilisateurs. On parcourt la base avec `UserDB::getUsers()`. Pour chaque utilisateur, on ajoute son `id` au JSON, puis on le range dans un `json::array`. Les utilisateurs sortent triés par identifiant, car la base est une `std::map`.

Nous avons vérifié avec `curl` que `GET /user` renvoie `[{"name":"Paul","age":24,"id":1}]` (200) lorsque Paul est le seul utilisateur.

#### Opération PUT : remplacer un utilisateur (Q14)

Nous avons ajouté une méthode virtuelle `put()` à `AbstractService`. Comme les autres, elle renvoie une erreur 405 par défaut. `UserService` la redéfinit, et `queryService()` a un nouveau cas `put`.

Dans `UserService::put()`, on renvoie une erreur 405 sans identifiant et une erreur 404 si l'identifiant est inconnu. Tous les champs sont obligatoires : le corps doit contenir exactement `name` et `age`, valides, sinon on renvoie une erreur 400. L'utilisateur est alors remplacé en entier et la réponse est un 204. Pour un identifiant inconnu, nous avons choisi de renvoyer 404 plutôt que de créer l'utilisateur.

#### Résultat des tests

Le script `./test.sh` passe 30 tests sur 31. Le test qui échoue est `PUT /version`, qui attend un 501 et reçoit un 400. Ce test a été écrit pour un serveur sans PUT, où la méthode tombait dans le cas par défaut du `switch`. Depuis que PUT est implanté (Q14), `queryService()` analyse le corps de la requête avant d'appeler le service. Le script envoie une requête sans corps : le JSON est invalide et l'erreur 400 est levée avant que le service ne réponde. Ce n'est donc pas un défaut de l'implantation, mais une conséquence du bonus. Sans le bonus, le test passerait.

**Quelle différence avec PATCH ?** 
PATCH modifie seulement les champs envoyés et garde les autres. PUT remplace la ressource entière : un champ absent est donc une erreur.

**Pourquoi PUT est-il idempotent et pas POST ?** 
Une opération est idempotente si la répéter donne le même état final qu'un seul appel. Envoyer deux fois le même PUT laisse l'utilisateur dans le même état. Envoyer deux fois le même POST crée deux utilisateurs, avec deux identifiants différents.
