/*
 * ej1.h
 * Sistemas embebidos y de tiempo real.
 * Ejercicio 2: Bomba de agua y monitor de caudal
 * Autor: Waldo Fusiman
 */

#ifndef EJ2_H

#define EJ2_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define SERIAL_BAUD                  115200

#define PERIOD_CONTROL_SEC_MS    500  // peridod medio segundo
#define PERIOD_READ_MS           1000 // periodo 1 segundo
#define PERIOD_CONTROL_IRG_MS    1000 // periodo 1 segundo
#define PERIOD_RUN_NOTIF_MS      1000 // periodo 1 segundo
 
#define PRIORITY_CONTROL_SEC    4   // max prioridad
#define PRIORITY_READ           3   // prioridad media
#define PRIORITY_CONTROL_IRG    1   // prioridad baja
#define PRIORITY_RUN_NOTIF      1   // prioridad baja

#define STACK_TASK_SIZE 4096  // Tamaño del stack por tarea. 
#define CORE        1         // Un core

#define HUMIDITY_MIN    40 // Minima humedad del suelo
#define TICKS_PER_SEC     (1000 / PERIOD_CONTROL_SEC_MS)   // 2 ticks por segundo
#define NO_FLOW_TICKS     (5  * TICKS_PER_SEC)             // 5 s sin caudal
#define ALARM_FLOW_TICKS  (15 * TICKS_PER_SEC)             // alarma 15 s
#define ALARM_RPM_TICKS   (30 * TICKS_PER_SEC)             // alarma 30 s
#define RPM_CHECK_TICKS   (1  * TICKS_PER_SEC)             // chequeo de giro a 1 s

// --- Variables Globales ---
extern int humidity;       // Porcentaje de humedad (%)
extern int caudal;         // Caudal de agua en la bomba
extern int rpm;            // Velocidad del motor (RPM)
extern bool pump;          // bomba de agua, true = encendida, false = apagada
extern bool alarmp;        // alarma de bomba de agua, true = encendida, false = apagada
extern String errMessage;      // Mensaje error 

extern SemaphoreHandle_t xMutexSensores;  // mutex para leer/escribir valores sensores humedad, caudal y rpm

// --- Tareas ---
void sensorsRead(void *pvParameters);
void controlSecurityPump(void *pvParameters);
void irrigationControl(void *pvParameters);
void runNotif(void *pvParameters);
void init();

#endif