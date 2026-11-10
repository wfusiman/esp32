
/*
 * ej3.h
 * Sistemas embebidos y de tiempo real.
 * Ejercicio 3: Control de temperatura de pileta.
 * Autor: Waldo Fusiman
 */

#ifndef EJ3_H

#define EJ3_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define SERIAL_BAUD                  115200
// Led de la placa = estado de la bomba (encendido = bomba encendida).
#ifdef LED_BUILTIN
  #define PIN_LED_PUMP               LED_BUILTIN
#else
  #define PIN_LED_PUMP               2
#endif

#define TEMP_POOL_MIN              26 // Temperatura minima de la pileta
#define TEMP_POOL_MAX              30 // Temperatura maxima de la pileta
#define TEMP_BOILER_MIN            45 // Temperatura minima de la caldera
#define TEMP_BOILER_MAX            60 // Temperatura maxima de la caldera
#define TEMP_BOILER_ACT            55 // temperatura minima de activacion de la caldera.

#define TIME_MIN_ACT_PUMP          30 // tiempo minimo de encendido de bomba de circulacion = 30 segundos.
#define TIME_SENSE_POOL            6  // cada 6 segundos toma temperatura de la pileta
#define TIME_SENSE_BOILER          15 // cada 15 segundos toma temperatura de la caldera

#define PERIOD_CONTROL_BOILER_MS   15000 // periodo de la tarea control de caldera = 15 segundos
#define PERIOD_CONTROL_PUMP_MS     1000 // periodo de la tarea control de bomba = 1 segundo
#define PERIOD_READ                2000 // periodo de la tarea lectura de valores de temperatura = 2 segundos
#define PERIOD_NOTIF               5000 // periodo de la tarea notificacion de valores = 5 segundos
 
#define PRIORITY_CONTROL_BOILER    1   // prioridad tarea control de caldera baja.
#define PRIORITY_NOTIF             2   // prioridad tarea muestra salida. normal.
#define PRIORITY_READ_TEMP         3   // prioridad tarea lectura de valores de temperatura.
#define PRIORITY_CONTROL_PUMP      4   // prioridad tarea control de bomba. alta



#define STACK_TASK_SIZE 4096  // Tamaño del stack por tarea. 
#define CORE        1         // Un core


// --- Variables Globales ---
extern volatile bool pump;           // Bomba encendida = true, bomba apagada = false.
extern volatile bool valve;          // valvula abierta = true -> Caldera encedida, valvula cerrada = false -> caldera apagada
extern int poolTemp;      // temperatura pileta.
extern int boilerTemp;    // Temperatura caldera. 
                            
extern SemaphoreHandle_t xMutexTemp;  // mutex para leer/escribir valores temperatura caldera y pileta

// --- Tareas ---
void readTemps( void *pvParams ); // lectura de valores de temperaturas caldera y pileta.
void pumpControl( void *pvParams ); // control de bomba de circulacion.
void boilerControl( void *pvParams ); // control de caldera.
void runNotif( void *pvParams ); // ejecuta y muestra notificaciones.
void initSystem();  // inicializacion

#endif
