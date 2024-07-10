## Imágenes del proyecto

![Consumo optimizando](IMG_3211.JPG)
![Repostado con 311 Kilometros](IMG_3098.JPG)
![Resultado de un ciclo de conducción](IMG_2972.JPG)


Proyecto Prius: Conversión a Híbrido Enchufable
Introducción
Este proyecto busca transformar un Toyota Prius en un vehículo híbrido enchufable mediante pruebas de laboratorio y modificaciones técnicas específicas.

Modificaciones y Procedimientos
1. Alteración del Sensor de Intensidad de la Batería EV
Modelo de Sensor: Sensor_intensidad_watchdog_millis_A0_V02_SIN
Objetivo: Controlar de forma precisa la carga y descarga de la batería EV para mejorar la eficiencia del sistema híbrido.
Procedimiento: Se realizarán ajustes controlados en el sensor para monitorizar y regular la intensidad de la batería EV, garantizando un rendimiento óptimo y la seguridad del sistema eléctrico.
2. Acceso al CAN OBD del Vehículo
Herramientas Utilizadas: USB_Canhacker_V2 y Comandos_EV_V2
Objetivo: Integrar y optimizar el sistema de gestión del vehículo con la nueva configuración híbrida enchufable.
Procedimiento: Mediante la interfaz OBD, se accederá a los sistemas de control del vehículo para ajustar los parámetros operativos del Prius transformado. Esta integración permitirá una supervisión en tiempo real y ajustes dinámicos durante las pruebas.
Componentes Adicionales y Sustituciones
Circuito Simulador de Batería: Esencial para las pruebas sin la batería original. Alternativamente, se puede mantener la batería original instalada.
Relés de Corte de Batería EV: Sustitución por modelos TE Tyco HVDC EV200AAANA, capaces de manejar mayores intensidades y evitar problemas de funcionamiento debido a la intensidad superior de las baterías de litio.
Notas Adicionales
A medida que el proyecto avance, se añadirá información detallada sobre los resultados de las pruebas, ajustes realizados y recomendaciones para futuras conversiones.

Conclusiones Provisionales
Este proyecto no solo aspira a convertir el Toyota Prius en un modelo más eficiente y adaptable a tecnologías limpias, sino también a explorar las capacidades y limitaciones de los sistemas híbridos enchufables en vehículos existentes.

Funcionalidad Principal
Comunicación CAN:

Utiliza la librería due_can para interactuar con el bus CAN.
Configura filtros para capturar mensajes específicos de interés.
Procesa mensajes CAN para monitorear parámetros como RPM, temperatura, voltaje y corriente.
Pantalla OLED:

Utiliza la librería U8g2lib para mostrar información en una pantalla OLED.
Muestra estado del CAN, SOC (estado de carga de la batería), energía consumida/regenerada, voltaje y corriente.
Muestra mensajes de error y operación normal.
Control de Energía:

Realiza seguimiento de la energía consumida y regenerada.
Calcula estos valores basándose en el voltaje y corriente medidos.
Muestra la energía acumulada y regenerada en la pantalla OLED.
Monitoreo de Voltaje y SOC:

Implementa control para monitorear y ajustar el voltaje y SOC de la batería.
Usa umbrales predefinidos para niveles de SOC y voltaje.
Histéresis y Control de Mensajes:

Controla el envío de mensajes basados en condiciones específicas usando histéresis.
Asegura estabilidad en lecturas de SOC y voltaje.
Gestión de Dispositivos de Salida:

Controla dispositivos de salida como relés basados en condiciones detectadas a través de CAN.
Activa relés para gestionar el voltaje de acuerdo con parámetros de carga de la batería.
Funcionalidad de Seguridad y Diagnóstico:

Incluye funciones para borrar errores y manejar condiciones de error.
Monitorea estado y activación de componentes clave.
Interacción con el Usuario:

Procesa entradas de botones e interruptores para realizar acciones como borrar errores o cambiar configuraciones.
Muestra alertas visuales en la pantalla OLED en respuesta a interacciones del usuario.
Gestión del Tiempo y Eventos:

Usa temporizadores para gestionar intervalos de actualización y monitoreo.
Realiza acciones periódicas como envío de mensajes y actualización de la pantalla.
Código en Detalle
Inicialización:

Configura la pantalla OLED y la comunicación CAN.
Borra errores iniciales del vehículo.
Configura filtros CAN para mensajes específicos.
Loop Principal:

Lee el estado del botón para borrar errores.
Llama a funciones para leer mensajes CAN (leerCan), procesar la activación del EV (activaEv), controlar la carga (conCarga) y actualizar el estado de la pantalla (lecSegundo, updateu8g2).
Funciones Clave:

sendCANMessage: Envía mensajes CAN con ID y datos específicos.
error: Borra códigos de error del vehículo.
mensajeSOC: Envía mensajes de estado de carga (SOC).
drawCurrentIcon, drawThrottleBar, drawBatteryIcon, dibujarNivelCarga: Funciones para dibujar diversos elementos en la pantalla OLED.
calcularCargaModuloBat: Calcula el porcentaje de carga de un módulo de batería basado en su voltaje.
processCanMessage: Procesa mensajes CAN recibidos y actualiza variables globales.
activaEv: Gestiona la activación del modo EV basado en mensajes CAN y condiciones del vehículo.
conCarga: Controla la salida A0 basada en condiciones de voltaje y SOC.
lecSegundo: Actualiza el conteo de lecturas y mensajes enviados por segundo.
checkConnectionStatus: Verifica el estado de la conexión y muestra mensajes de error si no hay datos recibidos.

