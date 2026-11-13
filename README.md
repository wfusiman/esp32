# Arduino ESP32


## 1. Control de caldera de gas 

Implementacion de sistema de control para una caldera de gas. 
La temperatura del agua en la caldera debe esta entre 70°C y 98°C. 
Si supera los 98°C, la válvula eléctrica de gas debe cerrarse inmediatamente. 
La caldera debe estar prendida mas de 10 segundos, y descansar el mismo tiempo para volver a encenderse.

<img width="369" height="327" alt="image" src="https://github.com/user-attachments/assets/2371837c-c67e-40f4-af58-685cf7a5337d" />


Sensores y actuadores:
- Sensor de temperatura de agua: toma temperatura del agua.
- Válvula de gas solenoide:  abre / cierra paso de gas.

Lógica de control:
- Leer temperatura.
- Si la temperatura > 98°C, cerrar la válvula de gas (apagar caldera).
- Si la temperatura <= 98°C, la caldera está apagada y ya descansó al menos 10 segundos, abrir la válvula de gas (prender caldera) e iniciar el contador de encendido.
- Si la caldera está prendida y el contador de encendido >= 10 segundos, cerrar la válvula de gas (apagar caldera).
- Si la caldera está apagada y el contador de apagado >= 10 segundos, abrir la válvula de gas (prender caldera).


<img width="538" height="713" alt="image" src="https://github.com/user-attachments/assets/834df258-f3a9-419f-aaac-f68caccb6426" />

Tareas imlementadas:
|Tarea| descripcion | prioridad | periodo |
|-----|-------------|-----------|---------|
|Lectura de temperatura |	Toma la temperatura del agua |	Alta (3) |	1 seg |
|Control de temperatura máxima |	Monitorea  la temperatura del agua de la caldera y cierra la válvula de gas inmediatamente si supera los 98°C.	| Máxima (4) | 1 seg |
|Control de temperatura normal |	Monitorea  la temperatura del agua de la caldera, entre 70°C y 98°C, y los tiempos de encendido y apagado (10 segundos). | Media (2) |	1 seg |
|Simulación y notificación	| Simula el calentamiento (1°C cada 10 segundos) y el enfriamiento (1°C cada 20 segundos) del agua. Informa cada 5 segundos y notifica si cambia el valor de la válvula de gas. | Baja (1) |	1 seg |

Las prioridades siguen el criterio Rate Monotonic (menor período, mayor prioridad). Las cuatro tareas corren en un unico procesador.
El actuador válvula es un recurso compartido, que escriben la tarea de seguridad y la de control normal. Las variables temperature, valve y counter se protegen con un mutex (xMutexTemperature), que hereda prioridad y evita inversiones de prioridad. Todas las tareas que acceden a la temperatura toman el valor actualizado.

Simulación: implementación en placa ESP32 con FreeRTOS.
- Leer temperatura desde el puerto serie, valores en formato T:XX (ej, T:75 indica 75°C), cada 1 segundo.
- Evaluar cada 5 segundos y notificar por consola el estado de la válvula, imprimiendo “Válvula Gas ABIERTA” y “Válvula Gas CERRADA” solo cuando cambia estado.
- Temperatura inicial aleatoria entre 50°C y 120°C, (tasa de enfriamiento 1° casa 20 segundos) (tasa de calentamiento 1° casa 10 segundos).
  
| Prueba	| Resultado|
|---------|----------|
|T:50 | con válvula cerrada	Abre válvula (inicia ciclo 10 segundos), prende led |
|T:99 | con válvula abierta	Cierra válvula inmediatamente, apaga led |
|T:60 | inmediatamente después de cerrar válvula	Abre válvula después de 10 segundos de descanso, prende led cuando abre valvula |  
  


--- 

## 2. Bomba de agua y monitor de caudal

Automatizacion de encendido de una bomba de agua para riego, cuando se detecta falta de humedad en el suelo.
Si la humedad < 30% prender bomba. Si humedad > 90% el sistema no riega.
Se debe encender con falta de humedad, cada 30 minutos (30 segundos) y permanecer encendida 10 segundos.
Verificar si existe caudal para evitar que el motor trabaje en vacio, sensando cada 1 segundo.
Si no hay caudal durante 5 segundos, detener la bomba y activar alarma por 15 segundos.
Verificar que la bomba gire al menos a 600 rpm despues de 1 segundo, sino funciona accionar alarma durante 30 segundos.

<img width="814" height="306" alt="image" src="https://github.com/user-attachments/assets/fa2ec877-8e46-445f-8df1-da3f80f948d7" />


Sensores y actuadores:
- Sensor de humedad: toma valores de humedad del suelo.
- Sensor de caudal (caudalimetro): Mide la cantidad de agua que sale de la bomba hacia el canal de riego del suelo.
- Sensor de revoluciones (Tacometro): mide las revoluciones del motor de la bomba de agua.
- Actuador contactor: enciende y apaga la bomba de agua para impulsar/cortar el agua para el riego.
- Actuador alarma: sirena que se emite para indicar falla del motor.

Lógica de control:
    - Tomar el valor de humedad del suelo.
    - Si la humedad > 90%, apagar la bomba.
    - Si la humedad < 30% y ya pasaron 30 segundos de descanso de la bomba, encender la bomba por 10 segundos.
    - Verificar el caudal mientras la bomba de agua se encuentra encendida.
    - Si la bomba de agua se encuentra encendida y no hay caudal durante 5 segundos seguidos, detener la bomba y emitir alarma durante 15 segundos.
    - Verificar las revoluciones de la bomba de agua mientras se encuentra encendida.
    - Si las rpm < 600, apagar la bomba de agua y emitir alarma durante 30 segundos.

    <img width="750" height="631" alt="image" src="https://github.com/user-attachments/assets/4ada0082-4d45-4353-82c6-ffcec1342600" />

Tareas implementadas:
|Tarea	| Descripción |	Prioridad |	Periodicidad |
|-------|-------------|-----------|--------------|
|Lectura de sensores | Lee los valores de humedad del suelo, caudal de agua y rpm de la bomba de agua. |	Media (2) |	1 seg |
|Control de seguridad de la bomba |	Controla que haya caudal y que las rpm del motor sean superiores a 600; si no se cumple, apaga el motor y emite alarma.| Máxima (4) |	1 seg |
|Control de riego |	Controla el riego según los límites de humedad, prendiendo y apagando la bomba de agua en los tiempos establecidos. |	alta (3) |	1 seg |
|Notificaciones | Notifica por salida serial los valores de los sensores y los mensajes de error de la bomba. |	Normal (1) |	1 seg |

Simular el sistema en la placa ESP32, mediante FreeRTOS y usando el puerto Serie:
- Humedad por debajo del 30% se debe prender la bomba por arriba del 90% el sistema no riega
más.
- El caudal se simulará ingresando leyendo por entrada serial C:1 (hay caudal) o C:0 (no hay
caudal).
- Salida (Simulación de motor y alarma): El sistema debe imprimir "Bomba: ENCENDIDA" y
"Bomba: APAGADA" según corresponda. Si durante el ciclo de encendido se detecta que no
hay caudal, imprimir "ERROR: Bomba detenida por falta de caudal" o ERROR: Bomba sin
rotación.
- La velocidad de motor se ingresa a través una entrada serial B:velocidad rpm.

Simulacion:
| Prueba |	Resultado |
|--------|------------|
| H:20 |	Muestra Bomba: ENCENDIDA prende led  |
| H:95 con bomba encendida |	Muestra Bomba: APAGADA, apaga led |
| C:0 con bomba encendida |	Después de 5 segundos muestra ERROR: Bomba detenida por falta de caudal, enciende alarma 15 segundos, apaga led |
| B:300 con bomba encendida |	Muestra ERROR: Bomba sin rotación, enciende alarma 30 segundos, apaga led |


