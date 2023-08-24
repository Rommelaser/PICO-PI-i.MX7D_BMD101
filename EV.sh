
#!/bin/bash
# Este script configura las variables de ambiente para 
# Compilar el Kernel de Linux para la pico-pi-iMX7D

export PATH=$PATH:/opt/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin
echo PATH = $PATH
export ARCH=arm
echo ARCH = $ARCH
export CROSS_COMPILE=/opt/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
echo CROSS_COMPILE = $CROSS_COMPILE


bash -i
