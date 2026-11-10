/*
 * Sistemas embebidos y de tiempo real
 * Ejercicio2: Bomba de agua y monitor de caudal
 * Autor: Waldo Fusiman
 */

#include "ej2.h"

int humidity;       // Porcentaje de humedad (%)
int caudal;         // Caudal: 1=hay caudal, 0 = no hay caudal
int rpm;            // Velocidad del motor (RPM)
bool pump;          // Bomba de agua,  encendida = true, apagada = false 
bool alarmp;        // Alarma. encendida = true, apagada = false 
String errMessage;  // Mensaje de error a notificar 

SemaphoreHandle_t xMutexSensors; // mutex para lectuta escritura de sensores.

static void setPump( bool on_off ) {
  pump = on_off;
  digitalWrite( LED_PIN, on_off ? HIGH: LOW );
}

void setup() {
  initSystem();
  delay( 1000 );

  Serial.println("\t|------------------------------------------------------------");
  Serial.println("\t| Inicia Sistema de Control de Riego y control de Bomba ");
  Serial.println("\t| Comandos:");
  Serial.println("\t| \t H:XX (ej. H:40 --> cambia humedad a 40%)");
  Serial.println("\t| \t C:1|0 (ej. C:0 --> cambia a sin caudal)");
  Serial.println("\t| \t B:XXXX (ej. B:1500 --> cambia a 1500 rpm de la bomba)");
  Serial.println("\t|------------------------------------------------------------\n");

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

/*
* Inicializa variables
*/
void initSystem() {
  Serial.begin( SERIAL_BAUD );
  Serial.setTimeout(50); 
  //limpiar el buffer: Lee y descarta todo lo que tenga acumulado
  while (Serial.available() > 0) {
    Serial.read(); 
  }

  pinMode( LED_PIN, OUTPUT );
  setPump( false ); // bomba de agua apagadad
  humidity = HUMIDITY_LOW; // Porcentaje de humedad 
  caudal = 1;      // con caudal
  rpm = 1000;      
  alarmp = false;  // Alarma apagada 
  errMessage = ""; // sin mensajes

  // Crea mutex para proteger variables compartidas
  xMutexSensors = xSemaphoreCreateMutex();

}

/*
* Lectura de Sensores humedad de suelo, caudal de agua, rpm del motor, por ntrada Serial (prioridad = 2, Periodicidad: 1 segundo)
* Formato entrada: Humedad -> H:XX | Caudal -> C:XX, | RPM -> B:XXXX
*/ 
void sensorsRead(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_READ_MS );

  for (;;) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      
      if (input.startsWith("H:")) { // Entrada de humedad
        int val = input.substring(2).toInt();
        if (xSemaphoreTake(xMutexSensors, pdMS_TO_TICKS(20)) == pdTRUE) {
          humidity = val;
          xSemaphoreGive(xMutexSensors);
        }
      } else if (input.startsWith("C:")) { // Entrada de caudal: 0 no hay caudal, >= 1 hay caudal
        int val = input.substring(2).toInt();
        if (xSemaphoreTake(xMutexSensors, pdMS_TO_TICKS(20)) == pdTRUE) {
          caudal = val;
          xSemaphoreGive(xMutexSensors);
        }
      } else if (input.startsWith("B:")) { // Entrada rpm de bomba
        int val = input.substring(2).toInt();
        if (xSemaphoreTake(xMutexSensors, pdMS_TO_TICKS(20)) == pdTRUE) {
          rpm = val;
          xSemaphoreGive(xMutexSensors);
        }
      }
    }
    vTaskDelayUntil(&xLastWakeTime, period);
  }
}

/*
* Control de Seguridad bomba (Prioridad Máxima = 4, Periodicidad: 1 segundo)
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

    if (xSemaphoreTake(xMutexSensors, pdMS_TO_TICKS(10)) == pdTRUE) {

      if (alarmp) { // control de alarma y duracion
        if (modeAlarmCaudal) {
          counterAlarmCaudal++;
          if (counterAlarmCaudal >= ALARM_FLOW_TICKS) { // 15 segundos 
            alarmp = false;
            modeAlarmCaudal = false;
            counterAlarmCaudal = 0;
          }
        } else if (modeAlarmRPM) {
        counterAlarmRPM++;
          if (counterAlarmRPM >= ALARM_RPM_TICKS) { // 30 segundos 
            alarmp = false;
            modeAlarmRPM = false;
            counterAlarmRPM = 0;
          }
        }
      }
      else if (pump) { // si la bomba esta prendida , chequea caudal. 
        if (caudal == 0) { // Si no hay caudal durante 5 segundos -> Apagar bomba y activar alarma 15 segundos
          counterNoCaudal++;
          if (counterNoCaudal >= NO_FLOW_TICKS) { // 5 segundos
            setPump( false );
            alarmp = true;
            modeAlarmCaudal = true;
            counterAlarmCaudal = 0;
            counterNoCaudal = 0;
            counterRPM = 0;
            errMessage = "ERROR: Bomba detenida por falta de caudal";
          }
        } else {
          counterNoCaudal = 0;
        }

        // Chequeo revoluciones del motor
        counterRPM++;
        if (counterRPM == RPM_CHECK_TICKS + 1) { // Después de 1 segundo 
          if (rpm < RPM_MIN) { 
            setPump( false );
            alarmp = true;
            modeAlarmRPM = true;
            counterNoCaudal = 0;
            counterRPM = 0;
            counterAlarmRPM = 0;
            errMessage = "ERROR: Bomba sin rotacion";
          }
        } 
      } 
      else { // bomba apagada
        counterNoCaudal = 0;
        counterRPM = 0;
      }
      xSemaphoreGive( xMutexSensors );
    }
    vTaskDelayUntil(&xLastWakeTime, period );
  }
}

/*
* Control Normal de Riego (Prioridad alta = 3, Periodicidad: 1 segundo)
*/ 
void irrigationControl(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS( PERIOD_CONTROL_IRG_MS );

  static int counterOnOff = 0; // cuenta los segundos en el ciclo de riego.

  for (;;) {
    if (xSemaphoreTake(xMutexSensors, pdMS_TO_TICKS(20)) == pdTRUE) {
      if (alarmp) { // Si esta la alarma activa, no se puede activar el riego
        counterOnOff = 0;
      }
      else if (humidity > HUMIDITY_HIGH) { // si la humedad > 90%, no se riega
        setPump( false );
        counterOnOff = 0;
      }
      else if (humidity < HUMIDITY_LOW || counterOnOff > 0) { // si humedad < 30%, iniciar riego con ciclos de 10 segundo encendido y 20 segundos apagado
        counterOnOff++;
        setPump( counterOnOff <= IRRIG_ON ); // encendida los primeros 10 segundos.
        if (counterOnOff >= IRRIG_PERIOD) { 
          counterOnOff = 0;
        }
      } else {
        setPump( false );
        counterOnOff = 0;
      }
      xSemaphoreGive( xMutexSensors );
    }
    vTaskDelayUntil(&xLastWakeTime, period );
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
    String msg = "";

    if (xSemaphoreTake(xMutexSensors, pdMS_TO_TICKS(20)) == pdTRUE) {
      actualPump = pump;
      h = humidity;
      c = caudal;
      r = rpm;
      a = alarmp;
      msg = errMessage;
      errMessage = ""; 
      xSemaphoreGive(xMutexSensors);
    }
    // Imprime si hay cambios de estado de la bomba
    if (actualPump != lastPump) {
      Serial.printf("*** Bomba: %s\n", actualPump ? "ENCENDIDA":"APAGADA");
      lastPump = actualPump;
    }
    // Imprime errores de seguridad, si hay
    if (msg != "") {
      Serial.println(msg);
    }
    // Notifica valores 
    Serial.printf("[Sensores] --> Humedad: %d%% | Caudal: %d | RPM: %d | [Actuadores] --> Bomba: %s | Alarma %s\n", h, c, r, actualPump ? "ENCENDIDA" : "APAGADA", a ? "ENCENDIDA":"APAGADA");
  }
}
