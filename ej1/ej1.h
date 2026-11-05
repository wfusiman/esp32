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

#define SERIAL_BAUD                  115200

#define TEMP_MAX              98     // Temperatura maxima 
#define TEMP_MIN              70     // Temperatura minima 
#define TIME_ON_OFF           10     // minimo encendida y minimo apagada

#define PERIOD_SECURITY_MS   500      // periodo medio segundo
#define PERIOD_READ_MS       1000     // periodo lector temperatura 1 segundo
#define PERIOD_CONTROL_MS    1000     // periodo control normal de temperatura 1 segundo
#define PERIOD_NOTIF_MS      1000     // periodo notificacion 1 segundo
 
// Simulacion y notificacion de la caldera 
#define HEAT_SECONDS         10 // +1° C cada 10 segundos
#define COOL_SECONDS         20 // -1° C cada 20 segundos
#define NOTIF_SECONDS        5  // evaluacion/notificacion cada 5 s

#define PRIORITY_SECURITY    4
#define PRIORITY_READ        3
#define PRIORITY_CONTROL     2
#define PRIORITY_NOTIF       1

#define TASK_STACK_SIZE      4096 // tamaño de pila de las tareas
#define CORE                 1    // Un core


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

void init( void );

#endif

