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

<img width="1386" height="1158" alt="image" src="https://github.com/user-attachments/assets/b0a65549-2c92-4ced-be2c-31a6f45df91f" />

Simulación: implementación en placa ESP32 con FreeRTOS.
- Leer temperatura desde el puerto serie, valores en formato T:XX (ej, T:75 indica 75°C), cada 1 segundo.
- Evaluar cada 5 segundos y notificar por consola el estado de la válvula, imprimiendo “Válvula Gas ABIERTA” y “Válvula Gas CERRADA” solo cuando cambia estado.
- Temperatura inicial aleatoria entre 50°C y 120°C, (tasa de enfriamiento 1° casa 20 segundos) (tasa de calentamiento 1° casa 10 segundos).

Tareas implementadas:
- tarea lectura de temperatura (temperatureReader): toma la temperatura del agua cada 1 segundo leyendo el puerto serie (formato T:XX), sin bloquear, y actualiza el valor actual de temperature. (prioridad media = 3  / periodicidad 1000 ms).
- tarea control temperatura máxima (temperatureControlSecurity): monitorea cada medio segundo si la temperatura supera 98°C; en ese caso cierra el paso de gas y resetea el contador de segundos (prioridad máxima = 4 / periodicidad 500 ms).
- tarea control temperatura normal (temperatureControlNormal): cada 1 segundo controla la temperatura y los tiempos de encendido y apagado: 10 segundos encendida y 10 segundos de descanso antes de volver a encender (prioridad normal = 2 / periodicidad 1000 ms).
- tarea notificación y simulación (temperatureValveNotif): cada 1 segundo simula el calentamiento (1° cada 10 segundos) y enfriamiento (1° cada 20 segundos) del agua y cada 5 segundos informa por consola (prioridad baja 1 / periodicidad 1000 ms).

Las prioridades siguen el criterio Rate Monotonic (menor período, mayor prioridad). Las cuatro tareas corren en un unico procesador.
El actuador válvula es un recurso compartido, que escriben la tarea de seguridad y la de control normal. Las variables temperature, valve y counter se protegen con un mutex (xMutexTemperature), que hereda prioridad y evita inversiones de prioridad. Todas las tareas que acceden a la temperatura toman el valor actualizado.

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




