# Directorio donde está el código fuente del kernel a compilar
KDIR=/home/rommelaser/linux-tn-imx

# Prefijo del compilador cruzado
CROSS_COMPILE=/opt/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

# Agregar la flag -Wno-unused-function para que no se reporte un warning
CFLAGS_MODULE += -Wno-unused-function

# Objetivo a compilar, en este caso el archivo de objeto imx_rpmsg_tty.o
obj-m += imx_rpmsg_tty.o

# Regla para compilar el módulo
all:
	make -C $(KDIR) M=$(shell pwd) modules

# Regla para limpiar el directorio de compilación
clean:
	make -C $(KDIR) M=$(shell pwd) clean