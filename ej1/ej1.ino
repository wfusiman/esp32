/*
 * Sistemas embebidos y de tiempo real
 * Ejercicio1: Control de temperatura de caldera
 * Autor: Waldo Fusiman
 */


#include "ej1.h"

int temperature = 0;  // Valor de temperatura actual
bool valve = false;   // valvula abierta=true, cerrada=false
int counter = 0;  // contador de tiempo on/off de la valvula
SemaphoreHandle_t xMutexTemperature; // mutex para proteger lectura/escritura de temperatura y seteo de valvula

/*
* Abre/cierra valvula, enciende/apaga led.
*/
static void setValve( bool on_off ) {
  valve = on_off;
  digitalWrite( LED_PIN, on_off ? HIGH:LOW );
}

void setup() {
  initSystem();
  delay( 1000 );

  Serial.println("\t|---------------------------------------------------------------");
  Serial.println("\t| Inicia Sistema de Control de Caldera");
  Serial.printf("\t| Temperatura de caldera inicial %d°C\n", temperature );
  Serial.println("\t| Comando:");
  Serial.println("\t|  T:XX (ej. T:50 -> cambia la temperatura de la caldera a 50°C)");
  Serial.println("\t|---------------------------------------------------------------");

  // Creacion de tareas 
  xTaskCreatePinnedToCore( temperatureControlSecurity, "Control de seguridad limite de temperatura", TASK_STACK_SIZE, NULL, PRIORITY_SECURITY, NULL, CORE );
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
void initSystem() {
  // inicializa monitor serial
  Serial.begin( SERIAL_BAUD );
  Serial.setTimeout(50); 
  //limpiar el buffer: Lee y descarta todo lo que tenga acumulado
  while (Serial.available() > 0) {
    Serial.read(); 
  }

  // Inicializa led (apagado: caldera apagada)
  pinMode( LED_PIN, OUTPUT );
  setValve( false );

  // Inicializa temperatura aleatoria y valvula cerrada
  randomSeed( esp_random() );
  temperature = random( 50,121 ); // temperatura inicial entre 50 y 120
  counter = 0; // contador reseteado
  // Iniicializa mutex
  xMutexTemperature = xSemaphoreCreateMutex();
}

/*
* Lectura de temperatura
* Prioridad = 2, peridicidad 1 segundo
*/
void temperatureReader( void *pvParams ) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_READ_MS );
  char buf[16];
  uint8_t n = 0;

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

  for (;;) {
    xSemaphoreTake( xMutexTemperature, portMAX_DELAY);      
    // Controla si el valor supera la temperatura maxima 98°C.
    if (temperature > TEMP_MAX) {
      setValve( false );
      counter = 0;
    }
    xSemaphoreGive( xMutexTemperature );
    vTaskDelayUntil( &xLastWakeTime, period );
  }
}

/*
* Control normal de temperatura entre los limites permitidos
*/
void temperatureControlNormal( void *pvParams ) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_CONTROL_MS );
  
  for (;;) {
    xSemaphoreTake( xMutexTemperature, portMAX_DELAY );
    if (valve){ // si la valvula esta abierta.
      if (counter < TIME_ON_OFF) // si no completo el tiempo de valvula abierta, incrementa.
        counter++;
      if (counter >= TIME_ON_OFF) { // si completo el tiempo de valvula abierta, se cierra valvula.
        setValve( false );
        counter = 0;
      }
    }
    else { // valvula cerrada
      if (counter < TIME_ON_OFF) {// si no completo el tiempo de valvula cerrada, incrementa.
        counter++;
      }
      else if (temperature < TEMP_MAX) { // si se completo el tiempo de cerrado y la temperatura no esta al limite, se abre valvula
        setValve( true );
        counter = 0;
      }
    }
    xSemaphoreGive( xMutexTemperature );
    vTaskDelayUntil( &xLastWakeTime, period );
  }
}

/*
* Ejecucion simulacion calentamiento/enfriamiento de la caldera
* y notificacion de cambios de estado  de la valvula
*/
void temperatureValveNotif( void *pvParams ) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_NOTIF_MS );

  int counterHeat = 0; // contador de calentamiento
  int counterCool = 0; // contador de enfriamiento
  int counterNotif = 0; // contador para notificar
  bool lastValve = !valve; // ultimo valor de la valvula

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
        Serial.printf( "[Sensor] Temperatura caldera: %d °C --> [Actuador] Valvula Gas %s\n",t, v ? "ABIERTA" : "CERRADA" );
        lastValve = v;
    }
    else if (++counterNotif >= NOTIF_SECONDS) { // Evaluación y Notificación cada 5 segundos
      counterNotif = 0;
      Serial.printf("[Sensor] Temperatura caldera: %d °C\n", t);
    }
  }
}


