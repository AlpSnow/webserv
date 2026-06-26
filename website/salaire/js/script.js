let salaireDeBase = Number(prompt("Quel est le salaire de base du commercial ?"));
let chiffreAffaire = Number(prompt("A combien s'élève le chiffre d'affaire du commercial ?"));
let nbKm = Number(prompt("Quel est le nombre de kilomètres parcourus par le commercial ?"));

let bonusAffaire = "";

if(chiffreAffaire >= 50000) {
    bonusAffaire = chiffreAffaire * 0.04;
} else {
    bonusAffaire = chiffreAffaire * 0.02;
};


let RemboursementKilometrique = nbKm * 0.26;



let salaireFinal = Number(bonusAffaire) + Number(salaireDeBase) + Number(RemboursementKilometrique);



document.getElementById("result").innerHTML = "Le salaire du commercial est de " + salaireFinal + " €.";
document.getElementById("result2").innerHTML = "Ses primes de transports sont de " + RemboursementKilometrique + " €.";
document.getElementById("result3").innerHTML = "La prime pour son chiffre d'affaire s'élève à " + bonusAffaire + " €.";
document.getElementById("result4").innerHTML = "Son salaire de base est de " + salaireDeBase + " €.";
document.getElementById("result5").innerHTML = "Son chiffre d'affaire s'élève à " + chiffreAffaire + " €.";
