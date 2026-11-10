/*
 * ej1.h
 * Sistemas embebidos y de tiempo real.
 * Ejercicio 1: Control de caldera
 * Autor: Waldo Fusiman
 */

#ifndef EJ1_H

#define EJ1_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define SERIAL_BAUD           115200

#define TEMP_MAX              98     // Temperatura maxima de la caldera
#define TEMP_MIN              70     // Temperatura minima de la caldera
#define TIME_ON_OFF           10     // minimo encendida y minimo apagada en segundos

#define PERIOD_SECURITY_MS   500      // periodo tarea control de seguridad, 1/2 segundo
#define PERIOD_READ_MS       1000     // periodo tarea lector temperatura 1 segundo
#define PERIOD_CONTROL_MS    1000     // periodo tarea control normal de temperatura 1 segundo
#define PERIOD_NOTIF_MS      1000     // periodo tarea notificacion 1 segundo
 
// Simulacion y notificacion de la caldera 
#define HEAT_SECONDS         10 // +1° C cada 10 segundos
#define COOL_SECONDS         20 // -1° C cada 20 segundos
#define NOTIF_SECONDS        5  // evaluacion/notificacion cada 5 s

#define PRIORITY_SECURITY    4  // prioridad tarea control de seguridad
#define PRIORITY_READ        3  // prioridad tarea lectura de temperatura
#define PRIORITY_CONTROL     2  // prioridad tarea control normal.
#define PRIORITY_NOTIF       1  // priorirdad tarea notificacion valores

#define TASK_STACK_SIZE      4096 // tamaño de pila de las tareas
#define CORE                 1    // Un core
#define LED_PIN              2    // Pin led de la placa

/* -- variables globales */
extern int temperature;   // Valor de temperatura actual
extern bool valve;        // valvula => abierta=true, cerrada=false
extern int counter;       // contador de segundos 
extern SemaphoreHandle_t xMutexTemperature; // acceso lectura/escritura al valor de la temperatura 

/* -- prototipos */

void temperatureReader( void *pvParams );
void temperatureControlSecurity( void * pvParams );
void temperatureControlNormal( void *pvParams );
void temperatureValveNotif( void *pvParams );

void initSystem();

#endif

