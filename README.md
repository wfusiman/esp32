# Arduino ESP32


## 1. Control de caldera de gas 

Implementacion de sistema de control para una caldera de gas. 
La temperatura del agua en la caldera debe esta entre 70°C y 98°C. 
Si supera los 98°C, la válvula eléctrica de gas debe cerrarse inmediatamente. 
La caldera debe estar prendida mas de 10 segundos, y descansar el mismo tiempo para volver a encenderse.

<img width="632" height="413" alt="Captura desde 2026-11-04 12-29-36" src="https://github.com/user-attachments/assets/da5a2fee-ea2e-4a04-9130-d6d2d0b14bc6" />


Sensores y actuadores:
- Sensor de temperatura de agua: toma temperatura del agua.
- Válvula de gas solenoide:  abre / cierra paso de gas.

Lógica de control:
- leer temperatura.
- Si la temperatura > 98°C, cerrar paso de gas, resetear contador segundos.
- Si la temperatura < 70°C y caldera apagada, abrir paso de gas (prender caldera), iniciar contador segundos.
- Si caldera prendida y contador = 10 segundos, cerrar paso de gas (apagar caldera), iniciar contador segundos.
- Si caldera apagada y contador = 10 segundos, abrir paso de gas (prender caldera), iniciar contador segundos.

Diagrama de decisión de la lógica de control (tarea de control normal, cada 1 s):

Figura 1. Diagrama de decisión. La tarea de seguridad (cada 500 ms) repite solo la primera decisión (T > 98 °C).
<img width="892" height="680" alt="diagdesiciones" src="https://github.com/user-attachments/assets/78d9ea51-9729-4f86-b84b-f7ca808264e4" />

Tareas implementadas:
- tarea lectura de temperatura (temperatureReader): toma la temperatura del agua cada 1 segundo leyendo el puerto serie (formato T:XX), sin bloquear, y actualiza la variable compartida bajo mutex (prioridad 3 / periodicidad 1000 ms).
- tarea control temperatura máxima (temperatureControlSecurity): monitorea cada 500 ms si la temperatura supera 98°C; en ese caso cierra el paso de gas y resetea el contador de segundos (prioridad máxima 4 / periodicidad 500 ms).
- tarea control temperatura normal (temperatureControlNormal): cada 1 segundo controla la temperatura y los tiempos de encendido y apagado: 10 segundos encendida y 10 segundos de descanso antes de volver a encender (prioridad normal 2 / periodicidad 1000 ms).
- tarea notificación y simulación (temperatureValveNotif): cada 1 segundo simula el calentamiento (1° cada 10 segundos) y enfriamiento (1° cada 20 segundos) del agua y cada 5 segundos informa por consola (prioridad baja 1 / periodicidad 1000 ms).

Las prioridades siguen el criterio Rate Monotonic (menor período, mayor prioridad). Las cuatro tareas corren en un unico procesador.
La válvula es un único actuador que escriben la tarea de seguridad y la de control normal, por lo que es un recurso compartido. Las variables temperature, valve y counter se protegen con un mutex de FreeRTOS (xMutexTemperature), que hereda prioridad y evita inversiones de prioridad prolongadas. Cada decisión se toma dentro de una única sección crítica, de modo que ninguna tarea actúa con una temperatura desactualizada.

Explicación del código:
- init(): configura el puerto serie a 115200 baudios, crea el mutex, genera la temperatura inicial aleatoria entre 50°C y 120°C con esp_random(), deja la válvula cerrada y el contador en 10 para que la caldera pueda encender de inmediato.
- temperatureReader: cada 1 segundo lee los caracteres disponibles del puerto serie, arma la línea hasta el salto de línea y, si tiene el formato T:XX y un valor válido, reemplaza la temperatura actual. Así se puede forzar cualquier temperatura (ej. T:75) desde el monitor serie.
- temperatureControlSecurity y temperatureControlNormal: implementan el diagrama de decisión. Si T > 98°C cierran la válvula y ponen el contador en 0. Si la válvula está abierta cuentan hasta 10 segundos y la cierran; si está cerrada cuentan el descanso de 10 segundos y solo la abren si T < 98°C. Como el descanso de 10 segundos también se exige cuando T < 70°C, esa regla queda incluida y el umbral de 70°C no cambia la decisión.
- temperatureValveNotif: cada 1 segundo simula la caldera: con la válvula abierta suma 1°C cada 10 segundos y con la válvula cerrada resta 1°C cada 20 segundos. Cada 5 segundos evalúa e informa por consola: imprime la temperatura y, solo cuando el estado cambió, “Valvula Gas ABIERTA” o “Valvula Gas CERRADA”.
- setup() crea las cuatro tareas en el core 1 y loop() se elimina con vTaskDelete porque todo el trabajo corre en las tareas.

## 2. Bomba de agua y monitor de caudal

Automatizacion de encendido de una bomba de agua para riego, cuando se detecta falta de humedad en el suelo.
Si la humedad < 30% prender bomba. Si humedad > 90% el sistema no riega.
Se debe encender con falta de humedad, cada 30 minutos (30 segundos) y permanecer encendida 10 segundos.
Verificar si existe caudal para evitar que el motor trabaje en vacio, sensando cada 1 segundo.
Si no hay caudal durante 5 segundos, detener la bomba y activar alarma por 15 segundos.
Verificar que la bomba gire al menos a 600 rpm despues de 1 segundo, sino funciona accionar alarma durante 30 segundos.

<img width="950" height="241" alt="Captura desde 2026-11-04 12-32-37" src="https://github.com/user-attachments/assets/773213e2-7dce-42b0-895d-3992c618fc82" />

Sensores y actuadores:
- Sensor de humedad: toma valores de humedad del suelo.
- Sensor de caudal (caudalimetro): toma para medir el caudal de agua que sale de la bomba hacia el suelo.
- Tacometro: mide las revoluciones del motor de la bomba de agua.
- Motor bomba de agua: enciende y apaga para realizar el riego.
- Alarma: sirena que se emite para indicar falla del motor.




