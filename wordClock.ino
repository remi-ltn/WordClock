//************************************************************************************************************
// IMPORTATIONS
//************************************************************************************************************

#include <TimeLib.h>
#include <Timezone.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#define FASTLED_ESP8266_D1_PIN_ORDER
#include <FastLED.h>



//************************************************************************************************************
// CONSTANTES ET VARIABLES GLOBALES
//************************************************************************************************************

// Identifiants Wi-fi
const char ssid[] = "";                     // SSID du réseau
const char pass[] = "";       // Mot de passe du réseau

// Serveur NTP pour récupérer l'heure
static const char ntpServerName[] = "pool.ntp.org";
WiFiUDP Udp;
unsigned int localPort = 8888;  // Port local pour recevoir les paquets UDP

// Fuseau horaire et changement d'heure
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central Europe Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central Europe Time
Timezone centralEurope(CET, CEST);

// Variables de temps
time_t utcTime, localTime;

// Déclaration des LED
const int NB_LEDS = 114;
const int LED_PIN = 1;
CRGB leds[NB_LEDS];

// Intensité lumineuse
const int POT_PIN = 0;
const float MIN_INTENSITY = 10.0;
const float MAX_INTENSITY = 100.0;
float intensity = MIN_INTENSITY;

// État des LED
bool isOn[NB_LEDS];
bool willBeOn[NB_LEDS];

// Variables pour la fonction loop
float intensityOld;
int minuteOld;
int iter;

// Mois de l'année
const char monthes[12][12] = {"janvier", "février", "mars",
                             "avril", "mai", "juin", "juillet", 
                             "août", "septembre", "octobre",
                             "novembre", "décembre"};


//************************************************************************************************************
// FONCTIONS PRINCIPALES
//************************************************************************************************************

void setup(){
  // Initialisation
  pinMode(POT_PIN, INPUT);
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NB_LEDS);
  setupHourFunctions();
  setupMinuteFunctions();
  Serial.begin(9600);
  WiFi.begin(ssid, pass);
  delay(250);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  for (int i=0; i<NB_LEDS; i++){
    isOn[i] = false;
    willBeOn[i] = false;
  }
  readIntensity();
  testAllLeds();
  readTime();
  updateWords();
  minuteOld = minute(localTime);
  intensityOld = intensity;
  iter = 0;
}

void loop(){
  // Boucle
  if (iter == 20){
    readTime();
    if (minute(localTime) != minuteOld){
      updateWords();
      minuteOld = minute(localTime);
    }
    iter = 0;
  }
  readIntensity();
  if (intensity != intensityOld){
    updateIntensity();
    intensityOld = intensity;
  }
  iter ++;
  delay(50);
}



//************************************************************************************************************
// LECTURE ET ÉCRITURE SIMPLE DE L'HEURE 
//************************************************************************************************************

void readIntensity(){
  // Lit la consigne d'intensité lumineuse.
  intensity = map(analogRead(POT_PIN), 0, 1023, MIN_INTENSITY, MAX_INTENSITY);
}

void readTime(){
  // Lit l'heure.
  utcTime = now();
  localTime = centralEurope.toLocal(utcTime);
  serialTimeDisplay();
}

void printDigits(int i){
  // Écrit l'entier (<100) en deux chiffres sur le moniteur série.
  if (i<10){
    Serial.print(0);
  }
  Serial.print(i);
}

void serialTimeDisplay(){
  // Écrit la date et l'heure sur le moniteur série.
  Serial.print("Nous sommes le ");
  Serial.print(day(localTime));
  Serial.print(" ");
  Serial.print(monthes[month(localTime)-1]);
  Serial.print(" ");
  Serial.print(year(localTime));
  Serial.println(".");
  Serial.print("Il est ");
  Serial.print(hour(localTime));
  printDigits(minute(localTime));
  printDigits(second(localTime));
  Serial.println(".");
}


//************************************************************************************************************
// COMMANDE DES LED
//************************************************************************************************************

void (*hourFunctions[12])();
void (*minuteFunctions[12])();

void setupHourFunctions(){
  // Initialise les fonction d'heures.
  hourFunctions[0] = nullFunction;
  hourFunctions[1] = uneHeure;
  hourFunctions[2] = deuxHeures;
  hourFunctions[3] = troisHeures;
  hourFunctions[4] = quatreHeures;
  hourFunctions[5] = cinqHeures;
  hourFunctions[6] = sixHeures;
  hourFunctions[7] = septHeures;
  hourFunctions[8] = huitHeures;
  hourFunctions[9] = neufHeures;
  hourFunctions[10] = dixHeures;
  hourFunctions[11] = onzeHeures;
}

void setupMinuteFunctions(){
  // Initialise les fonctions de minutes.
  minuteFunctions[0] = nullFunction;
  minuteFunctions[1] = cinq;
  minuteFunctions[2] = dix;
  minuteFunctions[3] = etQuart;
  minuteFunctions[4] = vingt;
  minuteFunctions[5] = vingtCinq;
  minuteFunctions[6] = etDemie;
  minuteFunctions[7] = moinsVingtCinq;
  minuteFunctions[8] = moinsVingt;
  minuteFunctions[9] = moinsLeQuart;
  minuteFunctions[10] = moinsDix;
  minuteFunctions[11] = moinsCinq;
}

void setLight(int i){
  // Allume la LED i et/ou actualise son intensité lumineuse.
  int R = map(intensity, MIN_INTENSITY, MAX_INTENSITY, 20.4, 204.0);
  int G = map(intensity, MIN_INTENSITY, MAX_INTENSITY, 23.0, 230.0);
  int B = map(intensity, MIN_INTENSITY, MAX_INTENSITY, 25.5, 255.0);
  leds[i].setRGB(R, G, B);
  isOn[i] = true;
}

void turnOff(int i){
  // Éteint la LED i.
  leds[i].setRGB(0, 0, 0);
  isOn[i] = false;
}

void turnAllOff(){
  // Éteint toutes les LED.
  for (int i=0; i<NB_LEDS; i++){
    turnOff(i);
  }
}

void setOn(int i){
  // Indique que la LED i doit être allumée.
  // Si elle l'est déjà, rien ne se passe.
  willBeOn[i] = true;
}

void setOff(int i){
  // Indique que la LED i doit être éteinte.
  // Si elle l'est déjà, rien ne se passe.
  willBeOn[i] = false;
}

void testAllLeds(){
  // Allume toutes les LED une par une pour vérifier si elles fonctionnent.
  for (int i=0; i<NB_LEDS; i++){
    turnAllOff();
    setLight(i);
    FastLED.show();
    delay(300);
  }
}

void updateWords(){
  // Actualise l'affichage de l'heure sur l'horloge.
  int H = hour(localTime);
  int M = minute(localTime);
  for (int i=0; i<NB_LEDS; i++){
    setOff(i);
  }
  ilEst();
  if (M>=35){
    H ++;
    H %= 24;
  }
  if (H==0){
    minuit();
  } else if (H==12){
    midi();
  } else {
    H %= 12;
    (*hourFunctions[H])();
  }
  int Q = M/5;
  int R = M%5;
  if (M>=5){
    (*minuteFunctions[Q])();
  }
  if (R!=0){
    remainderMinutes(R);
  }
  for (int i=0; i<NB_LEDS; i++){
    if ((!isOn[i]) && (willBeOn[i])){
      setLight(i);
    } else if ((isOn[i]) && (!willBeOn[i])){
      turnOff(i);
    }
  }
  FastLED.show();
  delay(30);
}

void updateIntensity(){
  // Actualise l'intensité lumineuse des LED allumées.
  for (int i=0; i<NB_LEDS; i++){
    if (isOn[i]){
      setLight(i);
    }
  }
  FastLED.show();
  delay(30);
}


//************************************************************************************************************
// ÉCRITURE DE L'HEURE SUR L'HORLOGE
//************************************************************************************************************

void nullFunction(){}

void ilEst(){
  setOn(112);
  setOn(111);
  setOn(109);
  setOn(108);
  setOn(107);
}

void heure(){
  setOn(51);
  setOn(52);
  setOn(53);
  setOn(54);
  setOn(55);
}

void heures(){
  heure();
  setOn(56);
}

void uneHeure(){
  setOn(85);
  setOn(84);
  setOn(83);
  heure();
}

void deuxHeures(){
  setOn(105);
  setOn(104);
  setOn(103);
  setOn(102);
  heures();
}

void troisHeures(){
  setOn(96);
  setOn(97);
  setOn(98);
  setOn(99);
  setOn(100);
  heures();
}

void quatreHeures(){
  setOn(90);
  setOn(91);
  setOn(92);
  setOn(93);
  setOn(94);
  setOn(95);
  heures();
}

void cinqHeures(){
  setOn(75);
  setOn(76);
  setOn(77);
  setOn(78);
  heures();
}

void sixHeures(){
  setOn(72);
  setOn(73);
  setOn(74);
  heures();
}

void septHeures(){
  setOn(82);
  setOn(81);
  setOn(80);
  setOn(79);
  heures();
}

void huitHeures(){
  setOn(68);
  setOn(69);
  setOn(70);
  setOn(71);
  heures();
}

void neufHeures(){
  setOn(89);
  setOn(88);
  setOn(87);
  setOn(86);
  heures();
}

void dixHeures(){
  setOn(65);
  setOn(64);
  setOn(63);
  heures();
}

void onzeHeures(){
  setOn(46);
  setOn(47);
  setOn(48);
  setOn(49);
  heures();
}

void midi(){
  setOn(67);
  setOn(66);
  setOn(65);
  setOn(64);
}

void minuit(){
  setOn(62);
  setOn(61);
  setOn(60);
  setOn(59);
  setOn(58);
  setOn(57);
}

void cinq(){
  setOn(17);
  setOn(16);
  setOn(15);
  setOn(14);
}

void dix(){
  setOn(37);
  setOn(36);
  setOn(35);
}

void quart(){
  setOn(27);
  setOn(28);
  setOn(29);
  setOn(30);
  setOn(31);
}

void etQuart(){
  setOn(24);
  setOn(25);
  quart();
}

void vingt(){
  setOn(23);
  setOn(22);
  setOn(21);
  setOn(20);
  setOn(19);
}

void vingtCinq(){
  vingt();
  setOn(18);
  cinq();
}

void etDemie(){
  setOn(1);
  setOn(2);
  setOn(4);
  setOn(5);
  setOn(6);
  setOn(7);
  setOn(8);
}

void moins(){
  setOn(45);
  setOn(44);
  setOn(43);
  setOn(42);
  setOn(41);
}

void moinsVingtCinq(){
  moins();
  vingtCinq();
}

void moinsVingt(){
  moins();
  vingt();
}

void moinsLeQuart(){
  moins();
  setOn(39);
  setOn(38);
  quart();
}

void moinsDix(){
  moins();
  dix();
}

void moinsCinq(){
  moins();
  cinq();
}

void plusOne(){
  // Si les minutes sont congrues à 1 modulo 5
  setOn(113);
}

void plusTwo(){
  // Si les minutes sont congrues à 2 modulo 5
  plusOne();
  setOn(101);
}

void plusThree(){
  // Si les minutes sont congrues à 3 modulo 5
  plusTwo();
  setOn(12);
}

void plusFour(){
  // Si les minutes sont congrues à 4 modulo 5
  plusThree();
  setOn(0);
}

void remainderMinutes(int R){
  // Affiche le nombre de minutes à ajouter à l'heure écrite sur l'horloge.
  switch (R){
    case 1:
      plusOne();
      break;
    case 2:
      plusTwo();
      break;
    case 3:
      plusThree();
      break;
    case 4:
      plusFour();
      break;
  }
}



//************************************************************************************************************
// RÉCUPÉRATION DE L'HEURE
//************************************************************************************************************

const int NTP_PACKET_SIZE = 48; // L'heure NTP se situe dans les 48 premiers octets du paquet
byte packetBuffer[NTP_PACKET_SIZE]; // Tampon pour stocker les paquets entrants en sortants

time_t getNtpTime(){
  IPAddress ntpServerIP; // L'adresse IP du serveur NTP
  while (Udp.parsePacket() > 0) ; // Efface les paquets éventuels déjà reçus
  Serial.println("Transmission de la requête NTP");
  // Prend un serveur au hasard parmi ceux spécifiés
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(" : ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Réponse NTP reçue");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // Lit le paquet depuis le tampon
      unsigned long secsSince1900;
      // Convertit 4 octets (positions 40-44) en entier long
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    }
  }
  Serial.println("Pas de réponse du serveur NTP :-(");
  return 0; // Renvoie 0 si l'heure n'a pas pu être lue
}

// Envoie une requête au serveur NTP dont l'adresse est donnée
void sendNTPpacket(IPAddress &address){
  // Remet tous les octets dans le tampon à 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialise les valeurs nécessaires to pour former une requête NTP
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // Une valeur a maintenant été assignée à tous les champs NTP
  // On peut donc envoyer une requête pour obtenir un temps instantané:
  Udp.beginPacket(address, 123); // Les requêtes NTP doivent aller au 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
