/*
 * Sistemas embebidos y de tiempo real
 * Ejercicio1: Control de temperatura de caldera
 * Autor: Waldo Fusiman
 */


#include "ej1.h"

int temperature = 0;  // Valor de temperatura actual
bool valve = false;   // valvula abierta=true, cerrada=false
int counter = 0;  // contador on/off de la valvula
SemaphoreHandle_t xMutexTemperature;


void setup() {
  init();
  Serial.printf("\n");
  delay( 1000 );
  Serial.printf("\n");
  Serial.printf("\t|--------------------------------------\n");
  Serial.printf("\t| Inicia Sistema de Control de Caldera \n");
  Serial.printf("\t| temperatura inicial: %d °C \n", temperature);
  Serial.printf("\t|--------------------------------------\n");

  // Creacion de tareas 
  xTaskCreatePinnedToCore( temperatureControlSecurity, "Control de limite de temperatura", TASK_STACK_SIZE, NULL, PRIORITY_SECURITY, NULL, CORE );
  xTaskCreatePinnedToCore( temperatureReader, "Lectura de temperatura", TASK_STACK_SIZE, NULL, PRIORITY_READ, NULL, CORE );
  xTaskCreatePinnedToCore( temperatureControlNormal, "Control normal de temperatura", TASK_STACK_SIZE, NULL, PRIORITY_CONTROL, NULL, CORE );
  xTaskCreatePinnedToCore( temperatureValveNotif, "Notificacion temperatura valvula", TASK_STACK_SIZE, NULL, PRIORITY_NOTIF, NULL, CORE );
}

void loop() {
  // quita la tarea del scheduler
  vTaskDelete( NULL );
}

/*
* Inicializa 
*/
void init() {
  // inicializa monitor serial
  Serial.begin( SERIAL_BAUD );
  //limpiar el buffer: Lee y descarta todo lo que tenga acumulado
  while (Serial.available() > 0) {
    Serial.read(); 
  }

  // Iniicializa mutex
  xMutexTemperature = xSemaphoreCreateMutex();

  // Inicializa temperatura aleatoria y valuvula cerrada
  randomSeed( esp_random() );
  temperature = random( 50,121 );
  valve = false;
  counter = TIME_ON_OFF;
}

/*
* Lectura de temperatura
* Prioridad media, peridicidad 1 segundo
*/
void temperatureReader( void *pvParams ) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_READ_MS );
  char buf[16];
  uint8_t n = 0;

  Serial.printf("-- Inicia tarea lectura de temperatura...\n" );
  for (;;) {
    vTaskDelayUntil( &xLastWakeTime, period );

    while (Serial.available() > 0) {
      char c = (char)Serial.read();
      if (c == '\n' || c == '\r') {
        if (n > 0) {
          buf[n] = '\0';
          n = 0;
          int temp;
          if (sscanf( buf, "T:%d", &temp ) == 1 && temp > -50 && temp < 300) {
            if (xSemaphoreTake( xMutexTemperature, pdMS_TO_TICKS(50) ) == pdTRUE) {
              temperature = temp;
              xSemaphoreGive( xMutexTemperature );
            }
          }
        }
      } else if (n < sizeof(buf) - 1) {
        buf[n++] = c;
      } else {
        n = 0;  // linea demasiado larga: descartar
      }
    }
  }
}

/*
* Control de seguridad de temperatura sobre el limite permitido
*/
void temperatureControlSecurity( void *pvParams ) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_SECURITY_MS );
  int temp = 0; // Guarda el valor de la temperatura actual leida.

  Serial.printf("-- Inicia tarea control seguridad temperatura...\n" );
  for (;;) {
    vTaskDelayUntil( &xLastWakeTime, period );

    xSemaphoreTake( xMutexTemperature, portMAX_DELAY);      
    // Controla si el valor supera la temperatura maxima 98°C.
    if (temperature > TEMP_MAX) {
      valve = false;
      counter =0;
    }
    xSemaphoreGive( xMutexTemperature );
  }
}

/*
* Control normal de temperatura entre los limites permitidos
*/
void temperatureControlNormal( void *pvParams ) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_CONTROL_MS );
  
  int temp = 0; // Guarda el valor de la temperatura actual leida.
  counter = 0; // contador de segundos
  Serial.printf("-- Inicia tarea control valvula...\n" );
  for (;;) {
    vTaskDelayUntil( &xLastWakeTime, period );

    xSemaphoreTake( xMutexTemperature, portMAX_DELAY );
    if (temperature > TEMP_MAX) {
      valve = false;
      counter = 0;
    }
    else if (valve){
      if (++counter >= TIME_ON_OFF) {
        valve = false;
        counter = 0;
      }
    }
    else {
      if (counter < TIME_ON_OFF)
        counter++;
      if (counter >= TIME_ON_OFF && temperature < TEMP_MAX) {
        valve = true;
        counter = 0;
      }
    }
    xSemaphoreGive( xMutexTemperature );
  }
}

/*
* Ejecucion e impresion de cambios de valvula
*/
void temperatureValveNotif( void *pvParams ) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_NOTIF_MS );

  int counterHeat = 0; // contador de calentamiento
  int counterCool = 0; // contador de enfriamiento
  int counterNotif = 0; // contador para notificar
  bool lastValve = !valve; // ultimo valor de la valvula

  Serial.printf("-- Inicia tarea notificacion valvula...\n" );
  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, period);
    bool v;
    int t;

    xSemaphoreTake(xMutexTemperature, portMAX_DELAY);
    if (valve) {
        counterHeat++;
        if (counterHeat >= HEAT_SECONDS) { // +1°C cada 10 segundos
          temperature++;
          counterHeat = 0;
        }
    } else {
        counterCool++;
        if (counterCool >= COOL_SECONDS) { // -1°C cada 20 segundos
          temperature--;
          counterCool = 0;
        }
    }
    v = valve;
    t = temperature;
    xSemaphoreGive(xMutexTemperature);

    if (v != lastValve) { // Si cambio el estado de la valvula se imprime
        Serial.printf( "-- Temperatura: %d °C \t --> Valvula Gas %s\n",t, v ? "ABIERTA" : "CERRADA" );
        lastValve = v;
    }
    else if (++counterNotif >= NOTIF_SECONDS) { // Evaluación y Notificación cada 5 segundos
      counterNotif = 0;
      Serial.printf("-- Temperatura: %d °C\n", t);
    }
  }
}


