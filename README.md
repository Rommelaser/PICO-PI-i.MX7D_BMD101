# PICO-PI-i.MX7D_BMD101
Driver elaborado para el control del AFE BMD101 De NeuroSky controlado por la plataforma  PICO-PI-i.MX7

Para utilizar debe complementar los archivos del BSP de FreeRTOS dispuestos por TechNexion

Se separan los archivos del driver en 2 carpetas una asociada al módulo kernel en el Lado master de los procesadores Arm Cortex-A7 del núcleo heterogéneo incorporado al SoC, y una carpeta referente al firmware FreeRTOS cargado al núcleo micro-controlador Arm Cortex-M4 
