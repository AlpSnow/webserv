<?php

session_start();
//*? Ceci va démarrer une nouvelle session, ou reprendre une session déjà existante
// Suivant la superglobales PHP utilisé, toutes les informations de la session seront gardées en mémoire via différentes manières (cookies, serveurs etc..)

if(!isset($_SESSION['mot'])){
// $_SESSION est une superglobale, c'est une varibale interne à PHP toujours disponible quelque soit le contexte (globale ou locale)
// Le terme "superglobales" signifie que ces variables sont disponibles dans n'importe quel 
// script PHP : autrement dit, il est inutile de vérifier si elles existent (avec la fonction isset(), 
// par exemple), elles sont créées automatiquement par le serveur. Néanmoins, elles peuvent 
// être vides (et le seront si aucune donnée n'est transmise).

// Toutes les superglobales prennent la forme d'un tableau associatif 

//*? Si il n'existe aucune session avec un mot déjà défini alors :
    $mots = file("mots.txt");
    //*? On met notre liste de mots provenant de mots.txt dans une variable $mots
    $mot = rtrim(strtoupper($mots[array_rand($mots)]));
    //*? On choisi un mot au hasard avec la fonction array_rand (qui provient du tableau $mots)
    //*? On met ce mot en majuscule avec strtoupper
    //*? On supprime les espaces en fin de chaîne
    $_SESSION['mot'] = $mot;
    //*? on met le mot choisi au hasard en session
    $_SESSION['essais'] = [];
    //*? On crée un tableau en session avec les lettres que l'utilisateur a choisi, au début ce tableau est vide car l'utilisateur n'a rien choisi
    $_SESSION['vies'] = 6;
    //*? On crée une variable en session avec le nombre de vie de l'utilisateur (ici 6)
    if(!isset($_SESSION['partiesGagnees'])){
        $_SESSION['partiesGagnees'] = 0;
    }
    //*? Si la variable $_SESSION['partiesGagnees'] n'est pas déclarée (donc null)
    //*? alors $_SESSION['partiesGagnees'] = 0. (logique car l'utilisateur n'a gagné aucune partie)
    
    if(!isset($_SESSION['partiesPerdues'])){
        $_SESSION['partiesPerdues'] = 0;
    }
    //*? Si la variable $_SESSION['partiesPerdues'] n'est pas déclarée (donc null)
    //*? alors $_SESSION['partiesPerdues'] = 0. (logique car l'utilisateur n'a perdu aucune partie)
}

if(isset($_POST['essai'])){
//*? Si $_POST['essai'] est déclarée (donc différent de null et vide) Alors ...
//*? $_POST['essai'] représente les lettres que l'utilisateur envoie via le formulaire
//*? On recupere les données envoyé par l'utilisateur avec la Superglobale POST 
    if(!in_array($_POST['essai'], $_SESSION['essais'])){
    //*? Si la lettre qu'a choisi l'utilisateur ($_POST['essai']) n'est pas dans notre tableau de lettre ($_SESSION['essais']) Alors
        if(strpos($_SESSION['mot'], $_POST['essai']) === FALSE){
        //*? Comme au dessus. mais sans tableau donc on utilise strpos
        //*? Si la lettre choisi par l'utilisateur n'est pas dans le mot à trouver alors 
            $_SESSION['vies']--;
            //*? On enleve une vie a l'utilisateur car il s'est trompé 
        }
        $_SESSION['essais'][] = $_POST['essai'];
        //*? On rajoute la lettre qu'a choisi l'utilisateur (dans notre tableau $_SESSION['essais']) 
        //*? même si elle n'est pas dans le mot afin que l'utilisateur ne se trompe pas deux fois
    } else {
        // echo "<p class='lettreDejaDeviner'> Lettre déjà devinée </p><br>";
        //*? Ici ceci ne sert a rien. Car par la suite on va utiliser $lettreRestantes donc l'utilisateur n'aura jamais ce message
    }

}

$lettresRestantes = array_diff(range('A', 'Z'), $_SESSION['essais']);
//*? On crée une variable $lettreRestantes.
//*? On place dans cette variable l'alphabet de A a Z avec range('A', 'Z')
//*? On enleve de l'alphabet toutes les lettres déjà choisi par l'utilisateur ($_SESSION['essais']) avec array_diff


if($_SESSION['vies'] <= 0){
//*? Si l'utilisateur n'a plus de vie alors
    echo "<p class='milieu'> Vous avez perdu! </p><br>";
    //*? On affiche le message : tu as perdu avec un saut de ligne
    echo "<p class='milieu'>Le mot était ".$_SESSION['mot']."</p>";
    //*? maintenant que l'utilisateur a perdu, on affiche le mot qui était a trouver
    $_SESSION['partiesPerdues']++;
    //*? On rajoute +1 partie perdue
    unset($_SESSION['mot']);
    //*? On détruit la variable mot dans la session afin de relancer une partie
    echo "<div class='margeVD'></div>";
    echo "<div class='margeD'></div>";
} else {
    $lettresRestantesADeviner = 0;
    //*? On crée une variable avec le nombre de lettres restantes a deviner par l'utilisateur 
    //*? On l'initialise a zero 
    $etat = '';
    //*? On crée une variable $etat vide car on va s'en servir par la suite
    //*? cette variable représentera l'état de la partie 
    $longueurDuMot = strlen($_SESSION['mot']);
    //*? On calcul la longueur du mot a deviner avec strlen puis on met le résultat dans une variable $longueurDuMot


    $premiereLettre = substr($_SESSION['mot'], 0, 1); 
    $derniereLettre = substr($_SESSION['mot'], -1); 

    
    if (!in_array($premiereLettre, $_SESSION['essais'])){
        $_SESSION['essais'][] = $premiereLettre;
    }
    if (!in_array($derniereLettre, $_SESSION['essais'])){
        $_SESSION['essais'][] = $derniereLettre;
    }


    for($i = 0; $i < $longueurDuMot; $i++){
    //*? On fait une boucle allant de 1 jusqu'au nombre de lettre du mot a deviner
    //*? Ici elle va de 0 jusqu'au jusqu'au nombre de lettre du mot a deviner -1
        if(in_array($_SESSION['mot'][$i], $_SESSION['essais'])){
        //*? Si la lettre choisit par l'utilisateur (qu'on a mis dans le tableau $_SESSION['essais']) est dans le mot
        //*? (précisement a la place de la lettre dans le mot)  
            $etat .= $_SESSION['mot'][$i];
            //*? Alors on met la lettre dans notre variable $etat
            //*? .= signifie qu'on concatène la variable afin de mettre plusieurs résultats dans celle-ci
        } else {
        //*? Sinon ... donc Si la lettre choisit par l'utilisateur (qu'on a mis dans le tableau $_SESSION['essais']) n'est pas dans le mot
            $etat .= "_";
            //*? On affiche un underscore a la place de la lettre a deviner 
            $lettresRestantesADeviner++;
            //*? On incrémente de 1 notre variable lettre restantes a deviner. (pour chaque lettre restante a deviner)
        }
        $etat .= " ";
        //*? On met un espace dans notre variable $etat pour faire plus jolie au niveau de l'affichage et que les _ ne soient pas collés
        //*? comme ça on pourra différencier chaque lettre précisément 
    }
    echo "<p class='milieu'>".$etat."</p>";
    //*? On affiche notre variable $etat qu'on a concaténé 
    //*? Pour afficher le mot avec les lettres devinées et celles restantes a trouver (sous la forme "_")

    if($lettresRestantesADeviner == 0){
    //*? Si il ne reste plus aucune lettre a trouver 
    //*? donc si $lettresRestantesADeviner++ ne s'est pas incrémenté une seul fois (ce qui signifie que tout a été trouvé) 
        echo "<p class='milieu'> Vous avez gagné </p>";
        //*? on affiche un message de victoire 
        $_SESSION['partiesGagnees']++;
        //*? On rajoute +1 partie gagnée dans $_SESSION['partiesGagnees']
        unset($_SESSION['mot']);
        //*? On détruit la variable mot dans la session pour recommencer une partie
        echo "<div class='margeVD'></div>"; 
        echo "<div class='margeV'></div>"; 
    }

}


if($_SESSION['vies'] !=0 && $lettresRestantesADeviner !=0){
//*? Si le nombre de vie restantes est différent de zéro
//*? et Si le nombre de lettres restantes a deviner est différent de zéro alors 
?>

    <form class="formulaire" method = "post" action = "">
    <!-- On crée notre formulaire. C'est ici que l'utilisateur va choisir ses lettres 
    On utilise la method et superglobale POST. 
    Ici il n'y a pas d'action car on redirige le formulaire sur la même page : index.php  -->
        <fieldset>
            <div class="form-group">
                <label for="exampleSelect2" class="form-label mt-4">Choisissez une lettre</label>
                <select name = "essai" multiple="" class="form-select" id="exampleSelect2">
                <!-- ici on a choisi un affichage en select donc dans un menu déroulant  -->
                <?php
                    foreach($lettresRestantes AS $lettre){
                    //*? Pour chaque lettres restantes (voir la variable $lettresRestantes plus haut Ligne 64)
                        echo '<option value = "'.strtoupper($lettre).'">'.strtoupper($lettre).'</option>';
                        //*? On met chaque lettre restantes dans notre menu déroulant 
                }   
                ?>
                </select>
            </div>
            <input type="submit" name = "submit" value = "ESSAI" class="btn btn-primary">
            <!-- On fait un input pour soumettre le formulaire  -->
        </fieldset>
    </form>
<?php
} ?>
