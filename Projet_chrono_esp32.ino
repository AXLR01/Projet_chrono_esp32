#include <PxMatrix.h> // libraiirie panel
#include "MyButton.h" // libraiirie perso boutons
#define Button_OK 33
#define Button_MOINS 25
#define Button_PLUS 26
#define Button_NO 27

/***************************** Pins LED MATRIX *********************/
#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 2
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
uint8_t display_draw_time = 0;
PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D);
int key_state = 4;
/*******************************************************************/

/**************************** COLORS *****************************/
uint16_t R = display.color565(255, 0, 0) ;     // ROUGE
uint16_t G = display.color565(0, 255, 0) ;     // VERT
uint16_t B = display.color565(0, 0, 255) ;     // BLEU
uint16_t W = display.color565(255, 255, 255) ; // BLANC
uint16_t Y = display.color565(255, 255, 0) ;   // JAUNE
uint16_t C = display.color565(0, 255, 255) ;   // CYAN
uint16_t M = display.color565(255, 0, 255) ;   // MAGENTA
uint16_t P = display.color565(250, 101, 148) ; // ROSE
uint16_t X = display.color565(0, 0, 0) ;       // NOIR
/*******************************************************************/

/******************** Variables globale ***************************/
// variables temps
unsigned int set_minutes = 0; // minute(s) entrée(s)
unsigned int set_heures = 0; // heure(s) entrée(s)
unsigned long setTime ; // conversion en ms
unsigned long startTime ; // depart du chrono
unsigned long oldCumul ; // comparateur
unsigned long cumul = 0 ; // temps ecoulé
int convert = 0; // conversion %
// variables boutons
MyButton buttonOK;
MyButton buttonNO;
MyButton buttonPlus;
MyButton buttonMoins;
bool paused = false ;
// variables animation
unsigned long stepMillis ; // comparateur
unsigned long lastStepMillis ; // comparateur
unsigned long animMillis ; // frame
unsigned long lastAnimMillis ; 
long previousMillis ;
int stepAnimation = 0;
int anim ; // bascule d'animation
int bande ; // progression du temps sur panel
// variables jeton
bool settingHour = false; // jeton heure(s)
bool settingMinute = true; // jeton minute(s)
// variables haut-parleur
enum etat : byte { parameters, start_chrono, run_chrono, music, wait}; // variable machine etat
etat state = parameters; // init etat
unsigned char order[4] = {0xAA, 0x06, 0x00, 0xB0};

/****************** Fonction progression ************************/

// Setup
void setup() {
  // Audio
  Serial2.begin(9600);
  volume(30);//Volume settings 0x00-0x1E
  Serial.begin(9600);
  // boutons
  buttonOK.setup(Button_OK);
  buttonNO.setup(Button_NO);
  buttonPlus.setup(Button_PLUS);
  buttonMoins.setup(Button_MOINS);
  // Display
  display.begin(16);
  display.setFastUpdate(true);
  display_update_enable(true);
  display.drawLine(0, 0, 63, 0, B);
  display.drawLine(0, 31, 63, 31, B);
  display.drawLine(0, 0, 0, 30, B);
  display.drawLine(63, 1, 63, 30, B);
  display.setCursor(12, 11);
  display.print("Bonjour");
  delay(3000);
  displaySettingsHour();
}

void loop()
{
  buttonOK.loop();
  buttonNO.loop();
  buttonPlus.loop();
  buttonMoins.loop();
  switch (state) {
    
    // Réglage du temps
    case parameters:
      set_clock();
      break;

    // Début chrono
    case start_chrono:
      convert = 0;
      cumul = 0;
      setTime = (set_minutes * 60000) + (set_heures * 3600000);
      startTime = millis() ;
      stepAnimation = 0;
      state = run_chrono;
      break;

    case run_chrono:
      convert = (cumul * 100) / setTime;
      avancement();
      
      // Détecte les etats de pause
      if (buttonOK.isChange && buttonOK.isPressed)
      {
        display.clearDisplay();
        display.setCursor(18, 12);
        display.print("PAUSE");
        paused = !paused ;

        // Quitte l'etat de pause
        if (!paused)
        {startTime = millis() ;}
      }

      // Comptage chrono
      if (!paused)
      {
        cumul += millis() - startTime ;
        startTime = millis() ;
        if ((cumul - oldCumul) >= 1000)
        {
          Serial.println(cumul) ;
          oldCumul = cumul ;
        }

        // Animation
        drawAnimation();
        avancement();

        // Retour parametrage
        if (buttonNO.isChange && buttonNO.isPressed)
        {
          state = parameters;
        }

        // Check temps
        if (cumul > setTime)
        {
          state = music;
        }
      }
      break;

    // Alarme
    case music:
      play(0x01);
      display.clearDisplay();
      display.setCursor(19, 12);
      display.print("FIN!!");
      state = wait;
      break;

    // Attente d'instruction
    case wait:
      if (buttonOK.isChange && buttonOK.isPressed) 
      {
        set_minutes = 0;
        set_heures = 0;
        setTime ;
        startTime ;
        oldCumul ;
        cumul = 0 ;
        state = parameters ;
      }
      break;

    // Defaut
    default:
      Serial.print("ERREUR");
      break;
  }
}

/**************** Fonction animations *****************/
uint16_t colorFromBytePos(uint16_t value, byte pos, uint16_t colorOFF, uint16_t colorON)
{
  if (bitRead(value, pos))
  {return colorON ; }
  else
  {return colorOFF ; }
}

void drawByte16(int x, int y, uint16_t value, uint16_t colorOFF, uint16_t colorON)
{
  for (byte i = 0 ; i < 16; i++)
  {
    uint16_t color = colorFromBytePos(value, i, colorOFF, colorON) ;
    if (color != colorOFF)
    {
      display.drawPixel(x + i, y, color);
    }
  }
}
void drawPacManRight(byte state, int x, int y)
{

  switch (state)
  {
    case 0:
      drawByte16(x, y + 0,  496,  X, Y) ;
      drawByte16(x, y + 1,  2044, X, Y) ;
      drawByte16(x, y + 2,  4094, X, Y) ;
      drawByte16(x, y + 3,  4094, X, Y) ;
      drawByte16(x, y + 4,  1023, X, Y) ;
      drawByte16(x, y + 5,  127,  X, Y) ;
      drawByte16(x, y + 6,  15,   X, Y) ;
      drawByte16(x, y + 7,  127,  X, Y) ;
      drawByte16(x, y + 8,  1023, X, Y) ;
      drawByte16(x, y + 9,  4094, X, Y) ;
      drawByte16(x, y + 10, 4094, X, Y) ;
      drawByte16(x, y + 11, 2044, X, Y) ;
      drawByte16(x, y + 12, 496,  X, Y) ;
      break;
    case 1:
      drawByte16(x, y + 0,  496,  X, Y) ;
      drawByte16(x, y + 1,  2044, X, Y) ;
      drawByte16(x, y + 2,  4094, X, Y) ;
      drawByte16(x, y + 3,  4094, X, Y) ;
      drawByte16(x, y + 4,  8191, X, Y) ;
      drawByte16(x, y + 5,  8191, X, Y) ;
      drawByte16(x, y + 6,  15,   X, Y) ;
      drawByte16(x, y + 7,  8191, X, Y) ;
      drawByte16(x, y + 8,  8191, X, Y) ;
      drawByte16(x, y + 9,  4094, X, Y) ;
      drawByte16(x, y + 10, 4094, X, Y) ;
      drawByte16(x, y + 11, 2044, X, Y) ;
      drawByte16(x, y + 12, 496,  X, Y) ;
      break;
  }
}

void drawPacManLeft(byte state, int x, int y)
{
  switch (state)
  {
    case 0:
      drawByte16(x, y + 0,  496,  X, Y) ;
      drawByte16(x, y + 1,  2044, X, Y) ;
      drawByte16(x, y + 2,  4094, X, Y) ;
      drawByte16(x, y + 3,  4094, X, Y) ;
      drawByte16(x, y + 4,  8184, X, Y) ;
      drawByte16(x, y + 5,  8128, X, Y) ;
      drawByte16(x, y + 6,  7680, X, Y) ;
      drawByte16(x, y + 7,  8128,  X, Y) ;
      drawByte16(x, y + 8,  8184, X, Y) ;
      drawByte16(x, y + 9,  4094, X, Y) ;
      drawByte16(x, y + 10, 4094, X, Y) ;
      drawByte16(x, y + 11, 2044, X, Y) ;
      drawByte16(x, y + 12, 496,  X, Y) ;
      break;
    case 1:
      drawByte16(x, y + 0,  496,  X, Y) ;
      drawByte16(x, y + 1,  2044, X, Y) ;
      drawByte16(x, y + 2,  4094, X, Y) ;
      drawByte16(x, y + 3,  4094, X, Y) ;
      drawByte16(x, y + 4,  8191, X, Y) ;
      drawByte16(x, y + 5,  8191, X, Y) ;
      drawByte16(x, y + 6,  7680, X, Y) ;
      drawByte16(x, y + 7,  8191, X, Y) ;
      drawByte16(x, y + 8,  8191, X, Y) ;
      drawByte16(x, y + 9,  4094, X, Y) ;
      drawByte16(x, y + 10, 4094, X, Y) ;
      drawByte16(x, y + 11, 2044, X, Y) ;
      drawByte16(x, y + 12, 496,  X, Y) ;
      break;
  }
}

void drawBlinky(byte state, int x, int y)
{
  switch (state)
  {
    case 0:
      drawByte16(x, y + 0,  480,   X, R) ;
      drawByte16(x, y + 1,  2040,  X, R) ;
      drawByte16(x, y + 2,  4092,  X, R) ;
      drawByte16(x, y + 3,  5070,  X, R) ;
      drawByte16(x, y + 4,  390,   X, R) ;
      drawByte16(x, y + 5,  390,   X, R) ;
      drawByte16(x, y + 6,  8583,  X, R) ;
      drawByte16(x, y + 7,  13263, X, R) ;
      drawByte16(x, y + 8,  16383, X, R) ;
      drawByte16(x, y + 9,  16383, X, R) ;
      drawByte16(x, y + 10, 16383, X, R) ;
      drawByte16(x, y + 11, 16383, X, R) ;
      drawByte16(x, y + 12, 16383, X, R) ;
      drawByte16(x, y + 13, 16383, X, R) ;
      drawByte16(x, y + 14, 15855, X, R) ;
      drawByte16(x, y + 15, 6342,  X, R) ;
      drawByte16(x, y + 3,  3120,  X, W) ;
      drawByte16(x, y + 4,  7800,  X, W) ;
      drawByte16(x, y + 5,  1560,  X, W) ;
      drawByte16(x, y + 6,  1560,  X, W) ;
      drawByte16(x, y + 7,  3120,  X, W) ;

      break;
    case 1:
      drawByte16(x, y + 0,  480,   X, R) ;
      drawByte16(x, y + 1,  2040,  X, R) ;
      drawByte16(x, y + 2,  4092,  X, R) ;
      drawByte16(x, y + 3,  5070,  X, R) ;
      drawByte16(x, y + 4,  390,   X, R) ;
      drawByte16(x, y + 5,  390,   X, R) ;
      drawByte16(x, y + 6,  8583,  X, R) ;
      drawByte16(x, y + 7,  13263, X, R) ;
      drawByte16(x, y + 8,  16383, X, R) ;
      drawByte16(x, y + 9,  16383, X, R) ;
      drawByte16(x, y + 10, 16383, X, R) ;
      drawByte16(x, y + 11, 16383, X, R) ;
      drawByte16(x, y + 12, 16383, X, R) ;
      drawByte16(x, y + 13, 16383, X, R) ;
      drawByte16(x, y + 14, 14139, X, R) ;
      drawByte16(x, y + 15, 9009,  X, R) ;
      drawByte16(x, y + 3,  3120,  X, W) ;
      drawByte16(x, y + 4,  7800,  X, W) ;
      drawByte16(x, y + 5,  1560,  X, W) ;
      drawByte16(x, y + 6,  1560,  X, W) ;
      drawByte16(x, y + 7,  3120,  X, W) ;
      break;
  }
}

void drawFleeingGhost(byte state, int x, int y)
{
  switch (state)
  {
    case 0:
      drawByte16(x, y + 0,  480,   X, B) ;
      drawByte16(x, y + 1,  2040,  X, B) ;
      drawByte16(x, y + 2,  4092,  X, B) ;
      drawByte16(x, y + 3,  8190,  X, B) ;
      drawByte16(x, y + 4,  8190,  X, B) ;
      drawByte16(x, y + 5,  7374,  X, B) ;
      drawByte16(x, y + 6,  15567, X, B) ;
      drawByte16(x, y + 7,  16383, X, B) ;
      drawByte16(x, y + 8,  16383, X, B) ;
      drawByte16(x, y + 9,  16383, X, B) ;
      drawByte16(x, y + 10, 13107, X, B) ;
      drawByte16(x, y + 11, 11469, X, B) ;
      drawByte16(x, y + 12, 16383, X, B) ;
      drawByte16(x, y + 13, 16383, X, B) ;
      drawByte16(x, y + 14, 15855, X, B) ;
      drawByte16(x, y + 15, 6342,  X, B) ;

      break ;
    case 1:
      drawByte16(x, y + 0,  480,   X, B) ;
      drawByte16(x, y + 1,  2040,  X, B) ;
      drawByte16(x, y + 2,  4092,  X, B) ;
      drawByte16(x, y + 3,  8190,  X, B) ;
      drawByte16(x, y + 4,  8190,  X, B) ;
      drawByte16(x, y + 5,  7374,  X, B) ;
      drawByte16(x, y + 6,  15567, X, B) ;
      drawByte16(x, y + 7,  16383, X, B) ;
      drawByte16(x, y + 8,  16383, X, B) ;
      drawByte16(x, y + 9,  16383, X, B) ;
      drawByte16(x, y + 10, 13107, X, B) ;
      drawByte16(x, y + 11, 11469, X, B) ;
      drawByte16(x, y + 12, 16383, X, B) ;
      drawByte16(x, y + 13, 16383, X, B) ;
      drawByte16(x, y + 14, 14139, X, B) ;
      drawByte16(x, y + 15, 9009,  X, B) ;
      break ;
  }
}

bool left = true ;
void drawAnimation()
{
  if(stepMillis == 0) ;
    stepMillis = millis() ; // relève le temps
  if (stepMillis - lastStepMillis >= 50)  // deplacement de l'animation
  {
    display.clearDisplay() ;
    if(left)
      stepAnimation++ ;
    else
      stepAnimation-- ;
    lastStepMillis = stepMillis ;
  }
  if(stepMillis == 0) ; // alternance d'animation
    animMillis = millis() ;
  if (animMillis - lastAnimMillis >= 500)
  {
    anim = 1 - anim ;
    lastAnimMillis = animMillis ;
  }
  if (left)
  {
    drawPacManRight(anim, -15 + stepAnimation, 14) ;
    anim = 1 - anim ;
    drawBlinky(anim, -32 + stepAnimation, 11) ;
    anim = 1 - anim ;
    if(stepAnimation == 99)
      left = !left ;
  } 
  else
  { 
    drawPacManLeft(anim, -15 + stepAnimation, 14) ;
    anim = 1 - anim ;
    drawFleeingGhost(anim, -32 + stepAnimation, 11) ;
    anim = 1 - anim ;
    if(stepAnimation == 0)
      left = !left ;          
  }
}

String On2(int val) {
  if (val > 9)
    return String(val);
  return "0" + String(val);
}

void displaySettingsHour() {
  display.clearDisplay();
  display.setCursor(29, 16);
  display.print(":");
  display.setCursor(30, 16);
  display.print(":");
  display.setCursor(39, 7);
  display.print("M");
  display.setCursor(21, 7);
  display.print("H");
  if ( settingMinute == true )
  {
    display.fillRect(35, 15, 13, 1, B);
    display.fillRect(35, 23, 13, 1, B);
    display.fillRect(35, 15, 1, 9, B);
    display.fillRect(47, 15, 1, 9, B);
  }
  if ( settingHour ==  true )
  {
    display.fillRect(17, 15, 13, 1, B);
    display.fillRect(17, 23, 13, 1, B);
    display.fillRect(17, 15, 1, 9, B);
    display.fillRect(29, 15, 1, 9, B);
  }
  display.setCursor(18, 16);
  display.print(On2(set_heures));
  display.setCursor(36, 16);
  display.print(On2(set_minutes));
}

/************************ Fonction réglage du temps ***********************/
void set_clock()
{
  bool displayChange = false;
  // settings des minutes
  if ( settingMinute == true )
  {
    if (buttonPlus.ToPress()) //( isButtonPlus) // ajout de temps
    {
      set_minutes++;
      if (set_minutes > 59)
      {
        set_minutes -= 60;
        if (set_heures == 23)
          set_heures = 0;
        else
          set_heures++;
      }
      displayChange = true;
    }
    if (buttonMoins.isChange && buttonMoins.isPressed) //(isButtonMoins) // enlevement de temps
    {
      if (set_minutes == 0)
      {
        set_minutes = 59;
        if (set_heures == 0) {
          set_heures = 23;
        } else {
          set_heures--;
        }
      } else {
        set_minutes--;
      }
      displayChange = true;
    }
  } // settings minute

  // setting Hour
  if ( settingHour ==  true )
  {
    if (buttonPlus.isChange && buttonPlus.isPressed) //( isButtonPlus)// ajout de temps
    {
      //isButtonPlus = false;
      set_heures++;
      if (set_heures > 23)
      {
        set_heures -= 24;
      }
      displayChange = true;
    }
    if (buttonMoins.isChange && buttonMoins.isPressed) // (isButtonMoins) // enlevement de temps
    {
      //isButtonMoins = false;
      if (set_heures > 0)
        set_heures--;
      else
        set_heures = 23;
      displayChange = true;
    }
  }// settings Hour

  if (buttonNO.isChange && buttonNO.isPressed) // (isButtonNO)
  {
    //isButtonNO = false;
    if (settingMinute) {
      settingMinute = false;
      settingHour = true;
    } else {
      settingHour = false;
      settingMinute = true;
    }
    displayChange = true;
  }

  if (displayChange) 
  {displaySettingsHour();}

  if (buttonOK.isChange && buttonOK.isPressed) // (isButtonOK)
  {state = start_chrono;}
}

/************************** Fonctions pannel ******************************/
void IRAM_ATTR display_updater() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(display_draw_time);
  portEXIT_CRITICAL_ISR(&timerMux);
}

void display_update_enable(bool is_enable)
{
  if (is_enable)
  {
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 2000, true);
    timerAlarmEnable(timer);
  }
  else
  {
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
}

/***************** Fonctions pour haut-parleur ****************/
void play(unsigned char Track)
{
  unsigned char play[6] = {0xAA, 0x07, 0x02, 0x00, Track, Track + 0xB3};
  Serial2.write(play, 6);
}

void volume( unsigned char vol)
{
  unsigned char volume[5] = {0xAA, 0x13, 0x01, vol, vol + 0xBE};
  Serial2.write(volume, 5);
}

void audioLoopMode( unsigned char mode)
{
  unsigned char arr[5] = {0xAA, 0x18, 0x01, 0x03, 0xC6};
  Serial2.write(arr, 5);
}

void audioRepeat( unsigned char nbRepeat)
{
  unsigned char arr[6] = {0xAA, 0x19, 0x02, 0x00, nbRepeat, nbRepeat + 0xC5};
  Serial2.write(arr, 6);
}
/**************************** Fct Progression *****************************/
void avancement()
{
  bande = floor((convert * 64) / 100) ;
  if ( convert <= 50 )
  {
    display.fillRect(0, 0, bande, 2, G);
  }
  if ( ( convert >= 50 ) && ( convert < 90 ) )
  {
    display.fillRect(0, 0, bande, 2, Y);
  }
  if ( ( convert >= 90 ) && ( convert <= 100 ) )
  {
    display.fillRect(0, 0, bande, 2, R);
  }
}
