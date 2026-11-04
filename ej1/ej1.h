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

//#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define SERIAL_BAUD                  115200

#define TEMP_MAX              98     // Temperatura maxima 
#define TEMP_MIN              70     // Temperatura minima 
#define TIME_ON_OFF           10     // minimo encendida y minimo apagada

#define PERIOD_SECURITY_MS   500
#define PERIOD_READ_MS       1000
#define PERIOD_CONTROL_MS    1000
#define PERIOD_NOTIF_MS      1000
#define REPORT_EVERY_S       5       // evaluacion/notificacion cada 5 s
 
// Simulacion de la caldera
#define HEAT_SECONDS_PER_DEG 10      // +1 C cada 10 s con valvula abierta
#define COOL_SECONDS_PER_DEG 20      // -1 C cada 20 s con valvula cerrada

#define PRIORITY_SECURITY    4
#define PRIORITY_READ        3
#define PRIORITY_CONTROL     2
#define PRIORITY_NOTIF       1

#define TASK_STACK_SIZE      4096 // tamaño de pila de las tareas
#define CORE                 1    // Un core


/* -- variables globales */

extern int temperature;   // Valor de temperatura actual
extern bool valve;        // valvula => abierta=true, cerrada=false
extern int counter;
extern SemaphoreHandle_t xMutexTemperature; // acceso lectura/escritura al valor de la temperatura 

/* -- prototipos */

void temperatureReader( void *pvParams );
void temperatureControlSecurity( void * pvParams );
void temperatureControlNormal( void *pvParams );
void temperatureValveNotif( void *pvParams );

void init( void );

#endif

