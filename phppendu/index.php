<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="css/bootstrap.min.css">
    <link rel="stylesheet" href="css/style.css">
    <title>Le jeu du pendu</title>
</head>


<body>
    <div class="containerAAA">
    
        <?php
            include "menu.php";
        ?>
    
        <section>

            <div class="partieGauche">
                <?php
                    include "pendu.php";
                    sort($_SESSION['essais']);
                    echo "<p class='lettreDejaDeviner'> Lettre déjà utilisées : ".implode(", ", $_SESSION['essais'])."</p>";
                ?>
            </div>

            <div class="partieDroite">
                <img src='image/hang<?=$_SESSION['vies']?>.gif' alt=''>
            </div>

        </section>


        <?php
            include "footer.php";
        ?>
        
    </div>

    <section class="sectionBas">
        <div class="recommencer">
            <p class="bouton1"> &nbsp;
                <input type="button" class="btn btn-primary" onclick="window.location.href = 'changerMot.php'" value="Recommencer la partie" />
            </p>
            <p class="bouton2"> &nbsp;
                <input type="button" class="btn btn-primary mettreGris" onclick="window.location.href = 'restartGame.php'" value="Réinitialiser les statistiques" />
            </p>
        </div>
        <div class="score">
            <p>Nombre de parties gagnées : <?=$_SESSION['partiesGagnees']?></p>
            <p>Nombre de parties perdues : <?=$_SESSION['partiesPerdues']?></p>
        </div>
    </section>


    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-ka7Sk0Gln4gmtz2MlQnikT1wXgYsOg+OMhuP+IlRH9sENBO0LRn5q+8nbTov4+1p" crossorigin="anonymous"></script>

</body>
</html>
