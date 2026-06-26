// Heure et date téléphone :

moment.locale("fr");
console.log(moment());

let day = moment().format('Do MMMM YYYY');
document.querySelector(".jour").innerHTML = day;


setInterval(updateTime, 1000);

function updateTime(){
    let heure = moment().format("H:mm:ss");
    document.querySelector(".heure").innerHTML = heure;
}


setInterval(updateTime2, 1000);

function updateTime2(){
    let heureHaut = moment().format("H:mm");
    document.querySelector(".partieGaucheHeure").innerHTML = heureHaut;
}

// Class active pour faire apparaître les éléments :

const boutton = document.querySelector(".boutton");
const ecranTelephone = document.querySelector(".ecranTel");

boutton.addEventListener("click", function() {
    ecranTelephone.classList.toggle("active");
})

// Recherche Google sur un nouvel onglet :

let query = document.querySelector(".query");
let searchBtn = document.querySelector(".searchBtn");

searchBtn.onclick = function(){
    let url ="https://www.google.fr/search?q="+query.value;
    window.open(url);
}

// Météo du téléphone :

window.weatherWidgetConfig =  window.weatherWidgetConfig || [];
window.weatherWidgetConfig.push({
    selector:".weatherWidget",
    apiKey:"7CREL5CYNCMZ8CRD5JYNBB43K", //Cle API personnelle 
    location:"Mulhouse, FR", //Choisir le lieu
    unitGroup:"metric", //"us" ou "metric"
    forecastDays:5, //Nombre de jours que l'on veut afficher
    title:"Mulhouse", //Titre optionnel 
    showTitle:true, 
    showConditions:false
    });

(function(){
    var d = document, s = d.createElement('script');
    s.src = 'https://www.visualcrossing.com/widgets/forecast-simple/weather-forecast-widget-simple.js';
    s.setAttribute('data-timestamp', +new Date());
    (d.head || d.body).appendChild(s);
    })();

// Class active2 pour l'affichage de la météo :

const meteoClique = document.querySelector(".weather");
const weatherWidget = document.querySelector(".weatherWidget");

meteoClique.addEventListener("click", function() {
    weatherWidget.classList.toggle("active2");
})

// Supprimer l'affichage de la météo lorsqu'on éteint le téléphone :

boutton.addEventListener("click", function() {
    weatherWidget.classList.remove("active2");
})
