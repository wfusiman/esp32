
/*
 * Sistemas embebidos y de tiempo real
 * Ejercicio3: Control de temperatura de pileta
 * Autor: Waldo Fusiman
 */

#include "ej3.h"

volatile bool pump;  // Bomba encendida = true, bomba apagada = false.
volatile bool valve; // Caldera encedida = true, caldera apgada = false
int poolTemp;        // temperatura pileta.
int boilerTemp;      // Temperatura caldera. 

SemaphoreHandle_t xMutexTemp; // mutex para lectura escritura de temperaturas

/*
* Bomba de circulacion, prende o apaga bomba y enciende o apaga led de la placa.
*/
static void setPump( bool on_off ) {
  pump = on_off;
  digitalWrite( PIN_LED_PUMP, on_off ? HIGH:LOW );
}

void setup() {
    initSystem();
    delay(1000);

    Serial.println("\t|--------------------------------------------------------------");
    Serial.println("\t| Sistema de Control de Temperatura de Pileta");
    Serial.println("\t| Comandos:");
    Serial.println("\t|  P:XX (ej. P:27 -> Cambia la temperatura de la pileta a 27°C");
    Serial.println("\t|  C:XX (ej. C:58 -> Cambia la temperatura de la caldera a 58°C");
    Serial.println("\t|--------------------------------------------------------------");

    xTaskCreatePinnedToCore( readTemps,"Lectura temperaturas caldera y pileta", STACK_TASK_SIZE, NULL, PRIORITY_READ_TEMP, NULL, CORE);                        
    xTaskCreatePinnedToCore( pumpControl,"Control de bomba de circulacion", STACK_TASK_SIZE, NULL, PRIORITY_CONTROL_PUMP, NULL, CORE );
    xTaskCreatePinnedToCore( boilerControl,"Control de caldera", STACK_TASK_SIZE, NULL, PRIORITY_CONTROL_BOILER, NULL, CORE );
    xTaskCreatePinnedToCore( runNotif,"Notificaciones", STACK_TASK_SIZE, NULL, PRIORITY_NOTIF, NULL, CORE );
}

void loop() {
  // quita la tarea loop del scheduler
  vTaskDelete( NULL );
}

void initSystem() {
  Serial.begin( SERIAL_BAUD );
  Serial.setTimeout(50); 
  while( Serial.available() > 0) 
    Serial.read();
  
  pinMode( PIN_LED_PUMP, OUTPUT );
  setPump( false );     // bomba apagada
  valve = false;    // caldera apagada
  // Inicialización valores aleatorio para temperaturas
  randomSeed(analogRead(0));
  poolTemp = random(20, 30); // Pileta: entre 20°C y 29°C;   
  boilerTemp = random(35, 59); // Caldera: entre 35°C y 58°C;

  // Creacion de  mutex
  xMutexTemp = xSemaphoreCreateMutex();
}

/*
* Lectura de temperatura de pileta o temperatura caldera, por entrada serial.
*/
void readTemps( void *pvParams ) {
  TickType_t xLastWakeTime;
  const TickType_t period = pdMS_TO_TICKS( PERIOD_READ ); 
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    while (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      if (!input.startsWith("P:") && !input.startsWith("C:")) {
        Serial.println("*** Comando no valido. Uso P:XX | C:XX");
      }
      else if (input.startsWith("P:")) {
        if (xSemaphoreTake( xMutexTemp, pdMS_TO_TICKS(20)) == pdTRUE) {
          int temp = input.substring(2).toInt();
          poolTemp = temp;
          xSemaphoreGive( xMutexTemp );
          Serial.printf("-- Temperatura de pileta actualizada: %d °C\n", poolTemp );
        }
      } 
      else if (input.startsWith("C:")) {
        int temp = input.substring(2).toInt();
        if (xSemaphoreTake( xMutexTemp, pdMS_TO_TICKS(20)) == pdTRUE) {
          boilerTemp = temp;
          xSemaphoreGive( xMutexTemp );
          Serial.printf("-- Temperatura de caldera actualizada: %d °C\n", boilerTemp );
        }
      }
    }
    vTaskDelayUntil(&xLastWakeTime, period);
  }
}

/*
* Control de bomba de circulacion
*/
void pumpControl( void *pvParams ){ 
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(PERIOD_CONTROL_PUMP_MS);

  int segCounter = 0; // ticks transcurridos desde el arranque.
  int counterActPump = 0; // contador de segundos que lleva activa la bomba.
  int counterTempPool = 0; // contador de segundos seguidos conla pileta >= 30°.
  int lastTempPool = 0; // ultimo valor de temperatura de la pileta.
  int lastTempBoiler = 0; // ultimo valor de temperatura de la caldera.

  for (;;) {
    // Tomar valores si corresponde segun el tiempo.
    if (xSemaphoreTake(xMutexTemp, pdMS_TO_TICKS(20)) == pdTRUE) { // lectura de temperaturas
      if (segCounter % TIME_SENSE_POOL == 0) // Lee temperatura de pileta cada 6 segundos
        lastTempPool = poolTemp;
      if (segCounter % TIME_SENSE_BOILER == 0) // Lee temperatura de caldera cada 15 segundos.
        lastTempBoiler = boilerTemp;
      xSemaphoreGive(xMutexTemp);
    }
    segCounter++;

    if (!pump) { // si la bomba esta apagada
      if (lastTempBoiler >= TEMP_BOILER_MIN && lastTempBoiler <= TEMP_BOILER_MAX  && lastTempBoiler >= TEMP_BOILER_ACT && lastTempPool < TEMP_POOL_MAX) { // si la caldera esta en rango y la temperatura de la pileta es menor a la temperatura maxima, se enciende la bomba.
        setPump( true );
        counterActPump = 0;
        counterTempPool = 0;
      }
    }
    else {
      counterActPump++; // bomba prendida, se incrementa tiempo predido
      counterTempPool = (lastTempPool >= TEMP_POOL_MAX) ? (counterTempPool + 1): 0; // si la temperatura de la pileta permanece en la temperatura se incrementa el tiempo, sino se resetea.
      if (lastTempBoiler < TEMP_BOILER_MIN || lastTempBoiler > TEMP_BOILER_MAX) { // Si la caldera no esta en las temperaturas aceptadas se apaga la bomba.
        setPump( false );
      }
      else if (counterTempPool >= TIME_MIN_ACT_PUMP && counterActPump >= TIME_MIN_ACT_PUMP) {  // despues de 30 segundos encendida con pileta en temperatura, se apgaga la bomba.
        setPump( false );
      } 
    }
    vTaskDelayUntil(&xLastWakeTime, period);
  }
}

/*
* Control de caldera: abre o cierra valvula para activar o apagar dependiendo de la temperatura de la caldera.
*/
void boilerControl( void *pvParams ) { 
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(PERIOD_CONTROL_BOILER_MS);

  for (;;) {
    int lboiler = 0;

    // Lectura de los valores actuales de temperatura
    if (xSemaphoreTake(xMutexTemp, pdMS_TO_TICKS(20)) == pdTRUE) {
      lboiler = boilerTemp;
      xSemaphoreGive(xMutexTemp);
      if (lboiler < TEMP_BOILER_MIN) { // Si la temperatura de la caldera esta baja, se prende caldera.
        valve = true;
      } else if (lboiler >= TEMP_BOILER_MAX) { // si la temperatura de la caldera es alta, se apaga la caldera.
        valve = false;
      }
    }
    vTaskDelayUntil(&xLastWakeTime, period );
  }
}

/*
* Notifica por salida serial los valores actuales.
*/
void runNotif(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_NOTIF );

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, period );

    int pt = 0;  // temperatura de pileta
    int ct = 0;   // temperatura de caldera.
    bool v = false; // caldera prendida o apagada
    bool b = false; // bomba prendida o apagada.

    if (xSemaphoreTake(xMutexTemp, pdMS_TO_TICKS(20)) == pdTRUE) {
      pt = poolTemp;
      ct = boilerTemp;
      v = valve;
      b = pump;
      xSemaphoreGive(xMutexTemp);
    }

    Serial.printf("[Sensores] -> temperatura Pileta: %d°C | temperatura Caldera: %d°C | [Actuadores] -> Válvula Gas: %s | Bomba Circulación: %s\n",
                    pt, ct, v ? "ABIERTA" : "CERRADA", b ? "ENCENDIDA" : "APAGADA");
  }
}


