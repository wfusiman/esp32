/*
 * Sistemas embebidos y de tiempo real
 * Ejercicio2: Bomba de agua y monitor de caudal
 * Autor: Waldo Fusiman
 */

#include "ej2.h"

int humidity = HUMIDITY_MIN;       // Porcentaje de humedad (%)
int caudal = 1;         // Caudal: 1=hay caudal, 0 = no hay caudal
int rpm = 1000;         // Velocidad del motor (RPM)
bool pump = false;      // Bomba de agua,  encendida = true, apagada = false 
bool alarmp = false;    // Alarma. encendida = true, apagada = false 
String errMessage = ""; // Mensaje de error a notificar 

SemaphoreHandle_t xMutexSensores;

void setup() {
  init();
  Serial.printf("\n");
  delay( 1000 );
  Serial.printf("\n");
  Serial.printf("\t|-------------------------------------------------------\n");
  Serial.printf("\t| Inicia Sistema de Control de Riego y control de Bomba \n");
  Serial.printf("\t| Comandos: H:XX, C:1|0, B:XXXX \n");
  Serial.printf("\t|-------------------------------------------------------\n");

  // Se crean las tareas
  xTaskCreatePinnedToCore( securityControlPump, "Control de seguridad bomba de agua", STACK_TASK_SIZE, NULL, PRIORITY_CONTROL_SEC,NULL, CORE );
  xTaskCreatePinnedToCore( sensorsRead,"Lectura de sensores por serial", STACK_TASK_SIZE, NULL, PRIORITY_READ, NULL, CORE );
  xTaskCreatePinnedToCore( irrigationControl,"Control normal de riego", STACK_TASK_SIZE, NULL, PRIORITY_CONTROL_IRG, NULL, CORE);
  xTaskCreatePinnedToCore( runNotif,"Ejecucion simulacion y notificaciones", STACK_TASK_SIZE, NULL, PRIORITY_RUN_NOTIF, NULL, CORE );
}

void loop() {
  // quita la tarea del scheduler
  vTaskDelete( NULL );
}

void init() {
  Serial.begin( SERIAL_BAUD );
  //limpiar el buffer: Lee y descarta todo lo que tenga acumulado
  while (Serial.available() > 0) {
    Serial.read(); 
  }
  // Crea mutex para proteger variables compartidas
  xMutexSensores = xSemaphoreCreateMutex();

  // Inicialización de semilla aleatoria para simulación
  randomSeed(analogRead(0));
}

/*
* Lectura de Sensores humedad de suelo, caudal de agua, rpm del motor, por ntrada Serial (prioridad media = 3, Periodicidad: 1 segundo)
* Formato entrada: Humedad -> H:XX | Caudal -> C:XX, | RPM -> B:XXXX
*/ 
void sensorsRead(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_READ_MS );

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, period);

    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      
      if (input.startsWith("H:")) { // Entrada de humedad
        int val = input.substring(2).toInt();
        if (xSemaphoreTake(xMutexSensores, pdMS_TO_TICKS(20)) == pdTRUE) {
          humidity = val;
          xSemaphoreGive(xMutexSensores);
        }
      } else if (input.startsWith("C:")) { // Entrada de caudal: 0 no hay caudal, >= 1 hay caudal
        int val = input.substring(2).toInt();
        if (xSemaphoreTake(xMutexSensores, pdMS_TO_TICKS(20)) == pdTRUE) {
          caudal = val;
          xSemaphoreGive(xMutexSensores);
        }
      } else if (input.startsWith("B:")) { // Entrada rpm de bomba
        int val = input.substring(2).toInt();
        if (xSemaphoreTake(xMutexSensores, pdMS_TO_TICKS(20)) == pdTRUE) {
          rpm = val;
          xSemaphoreGive(xMutexSensores);
        }
      }
    }
  }
}

/*
* Control de Seguridad bomba (Prioridad Máxima = 4, Periodicidad: 500ms)
*/
void securityControlPump(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_CONTROL_SEC_MS );

  static int counterNoCaudal = 0;
  static int counterAlarmCaudal = 0;
  static int counterRPM = 0;
  static int counterAlarmRPM = 0;
  static bool modeAlarmCaudal = false;
  static bool modeAlarmRPM = false;

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, period );

    int tempCaudal = 0; // almacenamiento temporal valor caudal
    int tempRpm = 0;    // almacenamiento temporal valor rpm
    bool tempBomba = false; // almacenamiento temporal valor bomba

    if (xSemaphoreTake(xMutexSensores, pdMS_TO_TICKS(10)) == pdTRUE) {
      tempCaudal = caudal;
      tempRpm = rpm;
      tempBomba = pump;
      xSemaphoreGive(xMutexSensores);
    }

    if (alarmp) { // control de alarma y duracion
      if (modeAlarmCaudal) {
        counterAlarmCaudal++;
        if (counterAlarmCaudal >= ALARM_FLOW_TICKS) { // 15 segundos (150 * 100ms)
          alarmp = false;
          modeAlarmCaudal = false;
          counterAlarmCaudal = 0;
        }
      } else if (modeAlarmRPM) {
        counterAlarmRPM++;
        if (counterAlarmRPM >= ALARM_RPM_TICKS) { // 30 segundos (300 * 100ms)
          alarmp = false;
          modeAlarmRPM = false;
          counterAlarmRPM = 0;
        }
      }
    }
    else if (tempBomba) { // Chequeo de Caudal (Sensado cada 1 segundo (10 ciclos de 100ms)
      if (tempCaudal == 0) { // Si no hay caudal (caudal == 0) durante 5 segundos -> Apagar bomba y activar alarma 15 segundos
        counterNoCaudal++;
        if (counterNoCaudal >= NO_FLOW_TICKS) { // 5 segundos
          pump = false;
          alarmp = true;
          modeAlarmCaudal = true;
          counterAlarmCaudal = 0;
          counterNoCaudal = 0;
          errMessage = "ERROR: Bomba detenida por falta de caudal";
        }
      } else {
        counterNoCaudal = 0;
      }

      // Chequeo de Tacómetro
      counterRPM++;
      if (counterRPM == RPM_CHECK_TICKS) { // Después de 1 segundo (10 * 100ms)
        if (tempRpm < 600) { 
          pump = false;
          alarmp = true;
          modeAlarmRPM = true;
          counterAlarmRPM = 0;
          errMessage = "ERROR: Bomba sin rotacion";
        }
      } 
    } else {
      counterNoCaudal = 0;
      counterRPM = 0;
    }
  }
}

/*
* Control Normal de Riego (Prioridad Normal = 1, Periodicidad: 1s)
*/ 
void irrigationControl(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_CONTROL_IRG_MS );

  static int counterOnOff = 0; // Controla los tiempos de encendido (30s) y apagado (10s)

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, period );

    int tempHumidity = 0;
    bool tempAlarm = false;

    if (xSemaphoreTake(xMutexSensores, pdMS_TO_TICKS(20)) == pdTRUE) {
      tempHumidity = humidity;
      tempAlarm = alarmp;
      xSemaphoreGive(xMutexSensores);
    }

    // Si hay una alarma activa, no se puede activar el riego automático
    if (tempAlarm) {
      counterOnOff = 0;
    }
    else if (tempHumidity > 90) { // si la humedad > 90%, no se riega
      pump = false;
      counterOnOff = 0;
    }
    else if (tempHumidity < 30 || counterOnOff > 0) { // si humedad < 30%, iniciar riego con ciclos de 30 segundo encendido y 10 segundos apagado
      counterOnOff++;

      if (!pump && counterOnOff <= 30) { // Si la bomba no esta activa y countador <= 30 prender bomba.
        pump = true; 
      } else if (pump && counterOnOff > 30) { // Si la bomba esta activa y countador > 30 apagar bomba.
        pump = false;
      }
      // Reiniciar ciclo completo de pausa/riego (30s encendida + 10s apagada = 40s total)
      if (counterOnOff >= 40) {
        counterOnOff = 0;
      }
    } else {
      pump = false;
      counterOnOff = 0;
    }
  }
}

/*
* Ejecucion de simulación y notificaciones por consola (Periodicidad: 1 segundo / Reporte cada 1 segundo)
*/
void runNotif(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_RUN_NOTIF_MS );

  bool lastPump = false; // ultimo estado de la bomba de agua.

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, period );

    // Valores para almacenar temporalente: humedad, caudal, rpm, bomba, y alarma.
    bool actualPump = false;
    int h = 0;
    int c = 0;
    int r = 0;
    bool a = false;

    if (xSemaphoreTake(xMutexSensores, pdMS_TO_TICKS(20)) == pdTRUE) {
      actualPump = pump;
      h = humidity;
      c = caudal;
      r = rpm;
      a = alarmp;
      xSemaphoreGive(xMutexSensores);
    }
    // Imprime si hay cambios de estado de la bomba
    if (actualPump != lastPump) {
      Serial.printf("*** Bomba: %s\n", actualPump ? "ENCENDIDA":"APAGADA");
      lastPump = actualPump;
    }
    // Imprime errores de seguridad, si hay
    if (errMessage != "") {
      Serial.println(errMessage);
      errMessage = ""; 
    }
    // Notifica valores 
    Serial.printf("Valores -> Humedad: %d%% | Caudal: %d | RPM: %d | Bomba: %s | Alarma %s\n", h, c, r, actualPump ? "ENCENDIDA" : "APAGADA", alarmp ? "ENCENDIDA":"APAGADA");
  }
}