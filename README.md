## Imágenes del proyecto

<img src="Imagenes_Resultados/IMG_3211.JPG" alt="Consumo optimizando" width="300"/>
Repostado lleno.
<img src="Imagenes_Resultados/IMG_3098.JPG" alt="Repostado con 311 Kilometros" width="300"/>
Ciclo de 160 Kilómetros.
<img src="Imagenes_Resultados/IMG_2972.JPG" alt="Resultado de un ciclo de conducción" width="300"/>
DATOS PANTALLA:

1. SOC: Estado de carga de la batería EV, indicando la capacidad estimada. El 30.0% muestra la capacidad restante.
1. 3,750: Valor medio de las celdas de litio en voltios.
1. N 25%: Capacidad estimada restante de la batería.
1. V: 225: Voltaje total de la batería.
1. Con: Consumo eléctrico.
1. Reg: Energía regenerada.
1. Capacidad de la batería: 21.6 kW, cargada al 96% con descarga limitada al 25%.
1. T: Temperatura del motor de combustión.
1. RM: Revoluciones del motor.

    
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
# Lista de Materiales y Enlaces

## Baterías:
Las baterías utilizadas en este proyecto son unidades recicladas, procedentes de fábricas que las han desechado debido a modificaciones en sus líneas de fabricación. En principio, estas baterías no han sido instaladas nunca en ningún vehículo, por lo que su estado de uso es excelente, prácticamente como nuevas.
Es importante destacar que resulta difícil conseguir siempre los mismos modelos de baterías, debido a los cambios en las series de fabricación. Si necesitas baterías específicas, por favor envíame un [correo electrónico](mailto:lorenzovv@gmail.com) para verificar las que podemos conseguir y las posibilidades de adaptación.
Para que tengas una idea, la adaptación de cada una de las tres baterías que he utilizado en la versión V2 ha requerido aproximadamente 180 horas de dedicación. Además, es crucial tener en cuenta el riesgo de cortocircuitos de alta intensidad. Por ello, debes idear un procedimiento adecuado para reducir los riesgos.
En ambas versiones, he dispuesto las baterías en módulos de 20S para minimizar riesgos y costes de fabricación.

# Lista de Materiales y Enlaces

## Baterías:
Las baterías utilizadas en este proyecto son unidades recicladas, procedentes de fábricas que las han desechado debido a modificaciones en sus líneas de fabricación. En principio, estas baterías no han sido instaladas nunca en ningún vehículo, por lo que su estado de uso es excelente, prácticamente como nuevas.
Es importante destacar que resulta difícil conseguir siempre los mismos modelos de baterías, debido a los cambios en las series de fabricación. Si necesitas baterías específicas, por favor envíame un <a href="mailto:lorenzovv@gmail.com" target="_blank">correo electrónico</a> para verificar las que podemos conseguir y las posibilidades de adaptación.
Para que tengas una idea, la adaptación de cada una de las tres baterías que he utilizado en la versión V2 ha requerido aproximadamente 180 horas de dedicación. Además, es crucial tener en cuenta el riesgo de cortocircuitos de alta intensidad. Por ello, debes idear un procedimiento adecuado para reducir los riesgos.
En ambas versiones, he dispuesto las baterías en módulos de 20S para minimizar riesgos y costes de fabricación.

## Enlaces:
- <a href="https://mailto:lorenzovv@gmail.com" target="_blank">Baterías recicladas de alta calidad</a>
- <a href="https://en_espera.com/procedimiento-adaptacion" target="_blank">Procedimiento de adaptación de baterías</a>
- <a href="https://images.app.goo.gl/rugSroTajMnXxNb58" target="_blank">Medidas de seguridad para la manipulación de baterías</a>
- <a href="https://es.aliexpress.com/item/33007254474.html?spm=a2g0o.order_list.order_list_main.15.2646194dvNHbQR&gatewayAdapt=glo2esp" target="_blank">Tiras níquel 0.1x5x25mm</a>
- <a href="https://es.aliexpress.com/item/32919726235.html?spm=a2g0o.order_list.order_list_main.10.2646194dvNHbQR&gatewayAdapt=glo2esp" target="_blank">0.5 KG Tira de níquel puro 0.2x8 mm</a>
- <a href="https://es.aliexpress.com/item/32888169005.html?spm=a2g0o.order_list.order_list_main.5.2646194dvNHbQR&gatewayAdapt=glo2esp" target="_blank">1 KG de níquel puro 0.8x28 mm</a>
- <a href="https://amzn.eu/d/0dBJseQr" target="_blank">Rollo de Tira de níquel Puro de 5 Metros 99,96%</a>


## Enlaces:
- [Baterías recicladas de alta calidad](mailto:lorenzovv@gmail.com)
- [Procedimiento de adaptación de baterías](https://en_espera.com/procedimiento-adaptacion)
- [Medidas de seguridad para la manipulación de baterías](https://images.app.goo.gl/rugSroTajMnXxNb58)
- [Tiras níquel 0.1x5x25mm](https://es.aliexpress.com/item/33007254474.html?spm=a2g0o.order_list.order_list_main.15.2646194dvNHbQR&gatewayAdapt=glo2esp)
- [0.5 KG Tira de níquel puro 0.2x8 mm](https://es.aliexpress.com/item/32919726235.html?spm=a2g0o.order_list.order_list_main.10.2646194dvNHbQR&gatewayAdapt=glo2esp)
- [1 KG de níquel puro 0.8x28 mm](https://es.aliexpress.com/item/32888169005.html?spm=a2g0o.order_list.order_list_main.5.2646194dvNHbQR&gatewayAdapt=glo2esp)
- [Rollo de Tira de níquel Puro de 5 Metros 99,96%](https://amzn.eu/d/0dBJseQr)

conCarga: Controla la salida A0 basada en condiciones de voltaje y SOC.
lecSegundo: Actualiza el conteo de lecturas y mensajes enviados por segundo.
checkConnectionStatus: Verifica el estado de la conexión y muestra mensajes de error si no hay datos recibidos.

