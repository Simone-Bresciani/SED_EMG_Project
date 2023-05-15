//librerie per LCD
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

const int readings = 20; //grandezza della finestra su cui si lavora
const int finestraOnOff = 9; //dopo quanti valori stesso gruppo posso cambiare stato 
const int finestraDerivata = 5; //quanti valori prendere per calcolare la derivata a valori discreti

const int pinEMG = 12;
const int pinLED_riposo = 26;
const int pinLED_sforzo = 25;
const int pinON = 33;
const int pinMOT = 27;

//pin LCD I2C
const int pinSDA = 13;
const int pinSCL = 14;

float analog[readings]; //vettore segnale analogico EMG
float threshold;    //soglia di attivazione muscolo
float max_value=0;  //massimo valore rilevato 
int countOn=0;  //conta numero volte sopra threshold
int countOff=0; //conta numero volte sotto threshold
int previous_onset=0; //stato precedente

//calcola la media di un vettore di float
float getMean (float * val, int arrayCount){
  float sum = 0;
  for ( int i = 0; i< arrayCount; i++){
    sum += val[i];
  }
  float avg = sum/arrayCount;
  return avg;
}

//calcolo deviazione standard di un vettore di float
float getStdDev(float * val, int arrayCount) {
  float avg = getMean(val, arrayCount);
  float sum = 0;
  for ( int i = 0; i< arrayCount; i++){
    sum = sum + ((val[i]) - avg)*((val[i]) - avg);
  }
  float variance = sum/arrayCount;
  return sqrt(variance);
}

LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  Serial.begin(19200);
  //definizio dei pin utilizzati
  pinMode(12, INPUT);
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);
  pinMode(33, OUTPUT);
  
  // Inizializzazione del display
  Wire.begin(pinSDA, pinSCL);
  lcd.init();
  lcd.clear();   // cancellazione dello schermo      
  lcd.backlight();      // Make sure backlight is on
  
  lcd.setCursor(3, 1); // posizionamento del cursore nella riga 0, colonna 0
  lcd.print("Progetto SED");
  lcd.setCursor(4, 2);
  lcd.print("A.A. 22/23"); // stampa del messaggio  
  delay(2000);
  lcd.clear(); 
  lcd.setCursor(4, 0);
  lcd.print("Universita'"); 
  lcd.setCursor(7, 1);
  lcd.print("degli");
  lcd.setCursor(2, 2);
  lcd.print("Studi di Brescia");  
  delay(2000);
  lcd.clear(); 
  lcd.setCursor(2, 1);
  lcd.print("Interpretazione");
  lcd.setCursor(7, 2);
  lcd.print("di un");  
  lcd.setCursor(4, 2);
  lcd.print("segnale EMG");
  delay(5000); // attende 2 secondi
  lcd.clear(); // cancellazione dello schermo

  lcd.setCursor(0, 0);
  lcd.print("Inizio fase");
  lcd.setCursor(0, 1);
  lcd.print("rilassamento...");
  for ( int i=1; i<10; i++){
    digitalWrite(pinLED_riposo, 1);
    delay(1000/i);
    digitalWrite(pinLED_riposo, 0);
    delay(1000/i);
  }
  lcd.clear();

  //prendo un set di 300 valori con muscolo a riposo
  digitalWrite(pinLED_riposo, 1);  
  int n_calibrazione = 300; // numero di dati campionati per fare calibrazione a riposo
  float x[n_calibrazione];
 
  //CALCOLO DEL THRESHOLD SUL SEGNALE RMS A RIPOSO SU UN CAMPIONE DI DATI INIZIALI
  float inizio[readings];
  
  //riempio i tutto il vettore tranne ultima posizione cosi poi ottengo un vettore circolare
  
  lcd.setCursor(1, 1);
  lcd.print("Rimanere rilassati");
  delay(2000);
  for (int i = 0; i< readings-1; i++){
    inizio[i] = analogRead(pinEMG); 
  }
  for (int i=0; i <n_calibrazione; i++){
    inizio[readings-1] = analogRead(pinEMG);

    float avg = getMean( inizio, readings); //media del vettore
    float offsetRemoved[readings];  
    float rect[readings];
    for (int k = 0 ; k < readings; k++){ 
      offsetRemoved[k] = inizio[k]-avg; //rimozione offset (media dei valori a riposo)
      rect[k] = abs(offsetRemoved[k]);  //rectification
    }
    
    //calcolo la derivata a valori discreti della finestra
    float deriv[readings-finestraDerivata];
    for (int k = 0 ; k < readings-finestraDerivata ; k++){
        float sum1=0;
        for (int j = k; j < k+finestraDerivata; j++){
          sum1 += rect[j]/finestraDerivata;   
        }
        deriv[k] = sum1;
    }
  
    //calcolo del root mean square sul vettore deriv
    float rms;
    float sum = 0;
    for(int k = 0; k < readings-finestraDerivata; k++){
      sum += deriv[k] * deriv[k];
    }
    rms = sqrt(sum/(readings-finestraDerivata));

   //ruoto il vettore di una posizione
    for(int k=0; k < readings-1; k++){
    inizio[k] = inizio[k+1];
    }

    //salvo rms
    x[i] = rms;
    delay(10);
  }
  digitalWrite(pinLED_riposo, 0);

  //Normal Distribution: serve per trovare la deviazione standard e la media 
  //dei valori prelevati in un periodo a riposo
  threshold = 3*getStdDev( x, n_calibrazione)+getMean( x, n_calibrazione); //3 deviazioni standard

  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print("Contrarre il");
  lcd.setCursor(6, 2);
  lcd.print("bicipite");
  
  //calcolo il massimo valore di sforzo applicando le stesse elaborazioni fatte su quelle a riposo
    digitalWrite(pinLED_sforzo, 1);
    delay(1000);
    max_value=0;
    for (int i = 0; i< readings-1; i++){
      inizio[i] = analogRead(pinEMG); 
    }
    for (int i=0; i <n_calibrazione; i++){
    inizio[readings-1] = analogRead(pinEMG);

    float avg = getMean( inizio, readings);
    float offsetRemoved[readings];
    float rect[readings];
    
    for (int k = 0 ; k < readings; k++){ 
      offsetRemoved[k] = inizio[k]-avg;
      rect[k] = abs(offsetRemoved[k]);
    }
     
    //calcolo la derivata a valori discreti della finestra
    float deriv[readings-finestraDerivata];
    for (int k = 0 ; k < readings-finestraDerivata ; k++){
        float sum2=0;
        for (int j = k; j < k+finestraDerivata; j++){
          sum2 += rect[j]/finestraDerivata;   
        }
        deriv[k] = sum2;
    }
  
    //calcolo del root mean square sul vettore deriv
    float rms;
    float sum = 0;
    for(int k = 0; k < readings-finestraDerivata; k++){
      sum += deriv[k] * deriv[k];
    }
    rms = sqrt(sum/(readings-finestraDerivata));


    for(int k=0; k < readings-1; k++){
    inizio[k] = inizio[k+1];
    }
    
    x[i] = rms;
    delay(10);
  }  
  //trovo il max valore nell'array
  for(int i = 0; i< n_calibrazione; i++){
    if(max_value < x[i]) max_value = x[i]; 
  }
  digitalWrite(pinLED_sforzo, 0);
  
  //creo un vettore di lettura del seganle lungo readings per fare elaborazione del segnale
  for (int i = 0 ; i < readings-1; i++){ 
    analog[i] = analogRead(pinEMG);
  }

  lcd.clear();
}

void loop() {
  //inizializzo gli array dei valori letti dal pin analogico
  analog[readings-1] = analogRead(pinEMG);

  //calcolo della finestra offsetRemoved
  float offsetRemoved[readings];
  // assieme al calcolo della rettificata
  float rect[readings];
  float avg = getMean( analog, readings);
  for (int i = 0 ; i < readings; i++){ 
    offsetRemoved[i] = analog[i]-avg;
    rect[i] = abs(offsetRemoved[i]);
  }
  
  
  //calcolo la derivata a valori discreti della finestra
  float deriv[readings-finestraDerivata];
  for (int k = 0 ; k < readings-finestraDerivata ; k++){
      float sum3 = 0;
      for (int j = k; j < k+finestraDerivata; j++){
        sum3 += rect[j]/finestraDerivata;   
      }
      deriv[k] = sum3;
  }

  //calcolo del root mean square sul vettore deriv
  float rms;
  float sum = 0;
  for(int k = 0; k < readings-finestraDerivata; k++){
    sum += deriv[k] * deriv[k];
  }
  rms = sqrt(sum/(readings-finestraDerivata));

  //calcolo dell'onset tramite il threshold
  int onset=0;
  
  if( rms >= threshold) {
    //se sopra la soglia aumento di uno il contatore on e azzero contatore off
    countOn++;
    countOff=0;
  }else {
    //se sotto la soglia aumento di uno il contatore off e azzero contatore on
    countOff++;
    countOn=0;
  }

  //caso in cui stato precedente OFF
  if(previous_onset == 0){
    if(countOn >= finestraOnOff){
      //se contatore on maggiore del numero preimpostato passo in STATO ON e azzero contatore off
      onset = 1;
      countOff = 0;
      countOn = finestraOnOff; 
    }else onset = 0;
    
    if(countOff >= finestraOnOff){
      countOff = finestraOnOff;
    }
  }

  //caso in cui stato precedente ON
  if(previous_onset == 1){
    if(countOff >= finestraOnOff){
      //se contatore off maggiore del numero preimpostato passo in STATO OFF e azzero contatore on
      onset = 0;
      countOn = 0;
      countOff = finestraOnOff;
    }else onset = 1;
    
    if(countOn >= finestraOnOff){
      countOn = finestraOnOff; 
    }
  }
    
  //salvo lo stato del mio onset per il prossimo ciclo
  previous_onset=onset;
/*
  if(rms >= threshold) onset = 1;
 */ 
  //se ricevo un segnale che supera il massimo riassegno il massimo per non avere problemi grafici
  if (rms > max_value) max_value = rms;
  
 
  //illumino un led in presenza di onset
  digitalWrite(pinON, onset);

  //stampa sul Lcd
  
  lcd.setCursor(0, 0);
  lcd.print("Elaborazione segnale");
  
  lcd.setCursor(0, 1);
  lcd.print("Threshold: ");
  lcd.print(threshold);
  
  
  lcd.setCursor(0, 2);
  lcd.print("Value: ");
  lcd.print(rms);

  
  //stampa sul monitor seriale
  Serial.print("Max:");
  Serial.print(100);
  Serial.print(",");
  Serial.print("onset:");
  Serial.print(onset*50);
  Serial.print(",");
  Serial.print("threshold:"); 
  Serial.print(threshold*100/(max_value));
  Serial.print(",");
  Serial.print("RMS:");
  Serial.println(rms*100/(max_value));

  // scorro il vettore della lettura del segnale
  for(int i=0; i < readings-1; i++){
    analog[i] = analog[i+1];
  }
}
