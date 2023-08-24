/*
 * File:   str_echo_freertos.c
 * Universidad Simón Bolivar
 * Author: Rommel A. J. Contreras F.
 *
 * Driver Para controlar el AFE:
 * BMD101 mediante el nucleo arm cortex M4
 *
 */

#include "rpmsg/rpmsg_rtos.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "string.h"
#include "board.h"
#include "mu_imx.h"
#include "debug_console_imx.h"
#include "uart_imx.h"
#include <stdio.h>
#include<stdlib.h>

////////////////////////////////////////////////////////////////////////////////
// Definitions Para RPMsg
////////////////////////////////////////////////////////////////////////////////
#define APP_TASK_STACK_SIZE 256
#define SYNC_BYTE 0xAA
#define EXCODE_BYTE 0x55
/*
 * APP decided interrupt priority
 */
#define APP_MU_IRQ_PRIORITY 3
#define MAX 16
/* Globals */
static char app_buf[512]; /* Each RPMSG buffer can carry less than 512 payload */
#define BUFFER_SIZE 16
uint8_t dataBuffer[BUFFER_SIZE];

//////////////////////////// Definiciones importantes para el driver  /////////////////////////////////


unsigned char receivedByte;
unsigned char payload[256];
int payloadLength;
unsigned char checksum;
unsigned char crc;
unsigned char excodeCount;
unsigned char * code;
unsigned char codeValue;
unsigned char length;
unsigned char data[256];
int dataIndex;
////////////////////////////////////////////////////////////////////////////////
// Prototipes
////////////////////////////////////////////////////////////////////////////////
//static void UART_SendDataPolling(void *base, const uint8_t *txBuff, uint32_t txSize);
static void UART_ReceiveDataPolling(void *base, uint8_t *rxBuff, uint32_t rxSize);
unsigned char calculateChecksum(unsigned char *data, int length);
char* handleData(unsigned char excodeCount, unsigned char * code, unsigned char length, unsigned char *data);


/*!
 * @brief A basic RPMSG task
 */
static void StrEchoTask(void *pvParameters)
{
    int result;
    struct remote_device *rdev = NULL;
    struct rpmsg_channel *app_chnl = NULL;
    void *rx_buf;
    int len;
    unsigned long src;
    unsigned long size;
    void *tx_buf;
    PRINTF("\r\nUniversidad Simón Bolivar\r\n");
    //unsigned long size;
    PRINTF("\r\nPor: Rommel Contreras / USB 14-10242\r\n");
    PRINTF("\r\n Driver Serial BMD101 \r\n");
    /* Print the initial banner */

//    /* RPMSG Init as REMOTE */
//    PRINTF("RPMSG Inicializando el Cortex-M4 como Remoto\r\n");

    // 0: Identificador del CPU remoto con el que se quiere comunicar (el primero disponible).
    // &rdev: Puntero a la estructura rpmsg_device que se utilizara para la comunicación.
    // RPMSG_MASTER: Indicador de que el procesador local es el maestro en la comunicación RPMsg.
    // &app_chnl: Puntero a la estructura rpmsg_channel que se utilizará para la comunicación.
    result = rpmsg_rtos_init(0 /*REMOTE_CPU_ID*/, &rdev, RPMSG_MASTER, &app_chnl);

    // a verificar si la funcion rpmsg_rtos_init() se ejecuto correctamente (si result == 0).
    assert(result == 0);

PRINTF("\r\nEl reconocimiento del servicio de nombre se ha completado, el Cortex-M4 ha configurado un canal rpmsg channel [%d ---> %d]\r\n", app_chnl->src, app_chnl->dst);

        /* Get RPMsg rx buffer with message */
    	/*
    	 * app_chnl->rp_ept: es el endpoint de RPMsg que se utiliza para recibir el mensaje.
		 * &rx_buf: puntero al búfer donde se almacena el mensaje que se reciba.
		 * &len: puntero a la variable que almacena la longitud del mensaje que se recibe.
		 * &src: puntero a la variable donde se almacena el identificador del origen del mensaje.
		 * 0xFFFFFFFF: tiempo de espera m�ximo en mili s para la operación de recepción.
		 * En este caso, se espera indefinidamente a que llegue un mensaje.
    	 */
        result = rpmsg_rtos_recv_nocopy(app_chnl->rp_ept, &rx_buf, &len, &src, 0xFFFFFFFF);
        assert(result == 0);

        /* Copy string from RPMsg rx buffer */
        assert(len < sizeof(app_buf));

        // copiar un bloque de memoria de tama�o len, de rx_buf a app_buf
        memcpy(app_buf, rx_buf, len);

        // establece el final de la cadena de caracteres
        app_buf[len] = 0; /* End string by '\0' */

        for(;;)
        {

        	// Step 1: Adquire one byte and compare it with the SYNC_BYTE 0xAA
            do {
            	UART_ReceiveDataPolling(BOARD_DEBUG_UART_BASEADDR, &receivedByte, 1);
            } while (receivedByte != SYNC_BYTE);
            //PRINTF("Debug_1\r\n");
            // Step 2: Verify SYNC byte
            UART_ReceiveDataPolling(BOARD_DEBUG_UART_BASEADDR, &receivedByte, 1);
            if (receivedByte != SYNC_BYTE) {
                continue;  // If not SYNC byte, go back to step 1, hace otra iteración del for(;;)
            }
            //PRINTF("Debug_2\r\n");
            // Step 3: Read PLENGTH byte
            UART_ReceiveDataPolling(BOARD_DEBUG_UART_BASEADDR, &receivedByte, 1);
            payloadLength = receivedByte;
            //PRINTF("Debug_3\r\n");

            // Step 4: Read PAYLOAD bytes
            for (int i = 0; i < payloadLength; i++) {
            	UART_ReceiveDataPolling(BOARD_DEBUG_UART_BASEADDR, &receivedByte, 1);
                payload[i] = receivedByte;
            }
            //PRINTF("Debug_4\r\n");

            // Step 5: Calculate checksum
            checksum = calculateChecksum(payload, payloadLength);
            //PRINTF("Debug_5\r\n");

            // Step 6: Verify CRC byte
            UART_ReceiveDataPolling(BOARD_DEBUG_UART_BASEADDR, &receivedByte, 1);
            crc = receivedByte;
            if (crc != checksum) {
                continue;  // If CRC does not match, go back to step 1
            }

            //PRINTF("Debug_6\r\n");


            // Step 7: Parse DataRows
            int index = 0;
            while (index < payloadLength) {
            	excodeCount = 0;
                while (payload[index] == EXCODE_BYTE) {
                    excodeCount++;
                    index++;
                }
                //PRINTF("Debug_7\r\n");

                // Step 7b: Parse CODE byte
                //codeValue= payload[index];
                code = &payload[index];
                index++;
                //PRINTF("Debug_8\r\n");

                // Step 7c: Parse LENGTH byte
                length = 0;
//              if (code != 0xFF)
                if (*code == 0x80) {
                    length = payload[index];
                    index++;
                    dataIndex = 0;
                    for (int i = 0; i < length; i++) {
                        data[dataIndex] = payload[index];
                        dataIndex++;
                        index++;
                    }
                    //codeValue=0x08;
                }
                else if(*code==0x03){
                	//PRINTF("\r\nCode: %X\r\n",*code);
                	data[0]=payload[index];
                	index++;
                	PRINTF("\r\nHEART RATE: %d Bpm\r\n",data[0]);
                }

                else if(*code==0x02){
                	PRINTF("\r\nCode: %d\r\n",*code);
                    data[0]=payload[index];
                    index++;
                    unsigned char signalQuality = data[0];
                    PRINTF("\r\n** Calidad de la señal **\r\n");
                    PRINTF("\r\nSQ=%d\n\r", data[0]);
                    if(signalQuality == 200){
                    	PRINTF("\n\rSensor ON\n\r");
                    }
                    if(signalQuality == 0){
                    	PRINTF("\n\rSensor OFF LOFF\n\r");
                    }
                }

            }


            //PRINTF("Debug_11\r\n");
            char* frame = handleData(excodeCount, code, length, data);
            tx_buf = rpmsg_rtos_alloc_tx_buffer(app_chnl->rp_ept, &size);
            assert(tx_buf);

            len=strlen(frame);

			memcpy(tx_buf, frame, len);

	        /* Envia el string recibido por el puerto serial with nocopy send */
			result = rpmsg_rtos_send_nocopy(app_chnl->rp_ept, tx_buf, len, src);
			assert(result == 0);

			free(frame);
			//PRINTF("Debug_12\r\n");

	    }
}

/*
 * MU Interrrupt ISR
 */
void BOARD_MU_HANDLER(void)
{
    /*
     * calls into rpmsg_handler provided by middleware
     */
    rpmsg_handler();
}

int main(void)
{
    hardware_init();

    /*
     * Prepare for the MU Interrupt
     *  MU must be initialized before rpmsg init is called
     */
    MU_Init(BOARD_MU_BASE_ADDR);
    NVIC_SetPriority(BOARD_MU_IRQ_NUM, APP_MU_IRQ_PRIORITY);
    NVIC_EnableIRQ(BOARD_MU_IRQ_NUM);

    /* Create a demo task. */
    xTaskCreate(StrEchoTask, "String Echo Task", APP_TASK_STACK_SIZE,
                NULL, tskIDLE_PRIORITY+1, NULL);

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never reach this point. */
    while (true);
}



//void UART_SendDataPolling(void *base, const uint8_t *txBuff, uint32_t txSize)
//{
//    while (txSize--)
//    {
//        UART_Putchar((UART_Type*)base, *txBuff++);
//        while (!UART_GetStatusFlag((UART_Type*)base, uartStatusTxEmpty));
//        while (!UART_GetStatusFlag((UART_Type*)base, uartStatusTxComplete));
//    }
//}

void UART_ReceiveDataPolling(void *base, uint8_t *rxBuff, uint32_t rxSize)
{
    while (rxSize--)
    {
        while (!UART_GetStatusFlag((UART_Type*)base, uartStatusRxDataReady));

        *rxBuff = UART_Getchar((UART_Type*)base);
        rxBuff++;
    }
}


unsigned char calculateChecksum(unsigned char *data, int length) {
    unsigned char checksum = 0;
    for (int i = 0; i < length; i++) {
        checksum += data[i];
    }
    checksum &= 0xFF;
    checksum = ~checksum & 0xFF;
    return checksum;
}

char* handleData(unsigned char excodeCount, unsigned char* code, unsigned char length, unsigned char* data) {
 // Data de los Electrodos
    if(*code == 0x80){

        // Combine the two bytes of data into a 16-bit signed integer
        short int rawValue = (data[0] << 8) | data[1];

        // Convert the raw value to its two's complement representation if necessary
        if (rawValue & 0x8000) {
            rawValue = -(~rawValue + 1);
        }

        // Ajustar el rango de los valores sumando 32768 (considerando que el valor más negativo se representa como 0)
        //rawValue = rawValue + 32768;

        // Convert the raw value to a floating-point value
        //float floatValue = (float)rawValue;

        // Convert the floating-point value to a string
        char* string = (char*)malloc(10 * sizeof(char));
    //    sprintf(string, "%f", floatValue);
        sprintf(string, "%d \r\n", rawValue);
        //PRINTF("%s", string);
        return string;

    }
// //Frecuencia Cardíaca
//    if(*code==0x03){
//    	PRINTF("\r\nCode: %X\r\n",*code);
//    	// Extrae la frecuencia cardíaca (bpm) from the data
//        unsigned char heartRate = data[0];
//        //Convert the heart rate to a string
//        char* heartRateString = (char*)malloc(10 * sizeof(char));  // Allocate memory for the string
//        sprintf(heartRateString, ", %d\r\n", heartRate);  // Format the heart rate as a string
//        return heartRateString;
//    }

    return NULL;
}
