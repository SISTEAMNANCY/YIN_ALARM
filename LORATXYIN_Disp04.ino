
#include <Arduino.h>
#include <driver/i2s.h>
#include <Yin.h>
#include "pin_config.h"
#include <Wire.h>
#include "SSD1306Wire.h" 
#include "pins_arduino.h"
#include <SPI.h>
#include <RadioLib.h>

#define LoRa_MOSI 10
#define LoRa_MISO 11
#define LoRa_SCK 9

#define LoRa_nss 8
#define LoRa_dio1 14
#define LoRa_nrst 12
#define LoRa_busy 13

SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED);


#define I2S_WS  45
#define I2S_DIN  47
#define I2S_BCK 46
#define I2S_PORT I2S_NUM_0
SX1262 radio = new Module(LoRa_nss, LoRa_dio1, LoRa_nrst, LoRa_busy);



Yin yin;
int bufferLen =512;
int32_t audioPlaceHolder[512];
int32_t local[512];
size_t localIdx = 0;
int32_t sample = 0;
size_t bytesIn = 0;
int sampleRate=32000; 
float pitch;
float prob;
int32_t Smax=0;
float threshold=0.15;
//String oldmessage= "Hello!";
const char* mess1 = "Bonjour AFIB";
unsigned long temps_precedent = NULL; 
int note_precedente = 0;
float k = (float)pow(2,1/12); // 1.059463094
float o = k + 0.059463094;    // 1.059463094 + 0.059463094 = 1.118926188 */
int a = 0;
int tableau[4]={0,0,0,0};
int ox[4]={-1,-2,-2,-2}; 
int eg[4]={2,2,1,2};
int vent[4]={9,-4,4,-4};
int mec[4]={-10,5,5,-10};
int gen[4]={0,0,0,0};
int card[4]={4,3,0,5};
int perf[4]={6,-6,0,6};
int alim[4]={-12,0,12,-12};


// I2S Setup 
void initI2S() 
{
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 32000,
    .bits_per_sample = i2s_bits_per_sample_t(32),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);   
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_DIN
  };
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_start(I2S_PORT);
}

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}
void VextOFF(void)  //Vext default OFF
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}


void displayReset(void) {
  // Send a reset
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, HIGH);
  delay(1);
  digitalWrite(RST_OLED, LOW);
  delay(1);
  digitalWrite(RST_OLED, HIGH);
  delay(1);
}

void setup() {
  Serial.begin(115200);
  initI2S();
  delay(500); 
  yin.initialize(sampleRate,bufferLen,threshold); 
  delay(500);  
  // This turns on and resets the OLED on the Heltec boards
  VextON();
  displayReset();
  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_nss);
  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }
  Envoi(mess1);  
}



void loop() 
{
  i2s_read(I2S_PORT, &sample, sizeof(sample), &bytesIn, portMAX_DELAY);
  // Write the sample to the local buffer
  local[localIdx] = sample;
  localIdx++;
  if( sample> Smax){
    Smax=sample;
  }  
  // When local buffer is full, send it to processing task 
  if (localIdx >= bufferLen)
    {
      if( Smax> 3000000){
       memcpy(audioPlaceHolder, local, sizeof(int32_t) * bufferLen);
       ProcessingTask();
     }     
     Smax=0;
     localIdx = 0;
    }
}



// AUDIO PROCESSING 
void ProcessingTask(void)
{      
        pitch = yin.getPitch(audioPlaceHolder);  
        prob = yin.getProbability();

        if ((pitch>80)&&(prob>0.85)){  
            int note= int(pitch);

         if ((note>=150 && note<=1000))
            {
               if (temps_precedent == NULL)
               {
                 temps_precedent = millis();
               }
               else {
                 unsigned long temps_courant = millis();
                 unsigned long delai = temps_courant - temps_precedent;
                //Serial.printf("Delai %ld\n", delai);
                temps_precedent = temps_courant;
        
                  if ((delai > 120) && ((note >= 150 && note <= 1000)))
                  {
                 // Serial.println(note);
         
                   int n = calcul_difference_demi_tons(note, note_precedente);
      
          
                   for (int f = 0; f<3; f++)
                     {
                     tableau[f] = tableau[f+1];
                     }
                     tableau[3] = n;
                      //Serial.printf("\n\n%d %d %d %d\n\n",tableau[0],tableau[1],tableau[2],tableau[3]); // Permet l'affichage des 4 derniers demis-tons regardés
 
                    // Test si alarme critique détecté correspond à une alarme critique connu 
                     test_tableau_demi_ton(tableau, ox, "OXYGENE");
                     test_tableau_demi_ton(tableau, eg,"ENERGIE");
                     test_tableau_demi_ton(tableau, vent,"VENTILATION");
                     test_tableau_demi_ton(tableau, mec,"MEDICAMENT");
                     test_tableau_demi_ton(tableau, gen,"GENERALITES");
                     test_tableau_demi_ton(tableau, card,"CARDIAQUE");
                     test_tableau_demi_ton(tableau, perf,"PERFUSION");
                     test_tableau_demi_ton(tableau, alim,"ALIMENTATION");
                     note_precedente = note;
                  } 
 
              }
            }
        }     
   }





void test_tableau_demi_ton(int* enregistrement, int* test, char* message) {
  if (enregistrement[0] == test[0] &&
      enregistrement[1] == test[1] &&
      enregistrement[2] == test[2] &&
      enregistrement[3] == test[3] )
  {    
    Serial.println(message); 
    display.clear();
	  display.drawString(0, 0, message);
	  display.display();
    //int state = radio.transmit(message);
    Envoi(message);
   }
}
 
int calcul_difference_demi_tons(int a, int b) {
  float dt = (float)b/a;
  if (dt<1)
    {
      dt = (float)a/b;
    }
  int n = round(log(dt) / log(o));
  if (a<b)
    {
      n = n * -1;
    }
  else
    {
      n = n;
    }  
  return n;
}

void Envoi( const char* message)
{
  Serial.print(F("[SX1262] Transmitting packet ... "));

  int state = radio.transmit(message);

  if (state == RADIOLIB_ERR_NONE)
  {
    // the packet was successfully transmitted
    Serial.println(F("success!"));

    // print measured data rate
    Serial.print(F("[SX1262] Datarate:\t"));
    Serial.print(radio.getDataRate());
    Serial.println(F(" bps"));
  }
  else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
  {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));
  }
  else if (state == RADIOLIB_ERR_TX_TIMEOUT)
  {
    // timeout occured while transmitting packet
    Serial.println(F("timeout!"));
  }
  else
  {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // wait for a second before transmitting again
  delay(0);
}

 
