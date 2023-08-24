// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019 NXP
 */

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/virtio.h>

// Agregado por RJCG
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/fcntl.h>

#define MAX_FILE_SIZE (2 * 1024 * 1024)  // Tamaño máximo del archivo: 10 megabytes

// hasta aca

/* this needs to be less then (RPMSG_BUF_SIZE - sizeof(struct rpmsg_hdr)) */
#define RPMSG_MAX_SIZE		256  // tamaño máx. del mensaje a enviar por RPMsg.
#define MSG		"hello world!"

/*
 * struct rpmsgtty_port - Wrapper struct for imx rpmsg tty port.
 * actúa como un envoltorio o contenedor para un puerto tty 
 * relacionado con el protocolo rpmsg en un entorno específico
 * encapsula los componentes necesarios para administrar un puerto TTY basado en RPMsg
 * @port:		TTY port data
 */
struct rpmsgtty_port {
	struct tty_port		port;		// Estructura que representa el puerto TTY. Contiene información sobre el puerto,
	spinlock_t		rx_lock;  		// bloqueo, garantizar la exclusión mutua en el acceso a los datos de recepción.
	struct rpmsg_device	*rpdev;		// puntero a una estructura que representa el dispo. RPMsg asociado al puerto TTY
	struct tty_driver	*rpmsgtty_driver;  // puntero al controlador de dispositivo TTY relacionado con este puerto. 
};

/*
 * La función rpmsg_tty_cb es el callback que se llama cuando llega un 
 * mensaje a través del canal RPMsg establecido en el driver rpmsg_tty_driver. 
*/ 
// static int rpmsg_tty_cb(struct rpmsg_device *rpdev, void *data, int len,
// 						void *priv, u32 src)
// {
// 	/*
// 	rpdev: puntero a estructura rpmsg_device, que tiene infor. del dispo. RPMsg
// 	data: un puntero al mensaje recibido
// 	len: el tamaño del mensaje recibido
// 	priv: un puntero a datos privados proporcionados en la llamada a register_rpmsg_driver
// 	src: la identificación del origen del mensaje.
// 	*/
	
// 	// espacio disponible del búfer de recepción del puerto tty.
// 	int space;
	
// 	// apunta al buffer con la info. recib. del dispositivo remoto-RPMsg.	
// 	unsigned char *cbuf;
	
// 	struct rpmsgtty_port *cport = dev_get_drvdata(&rpdev->dev);

// 	/* flush the recv-ed none-zero data to tty node */
// 	if (len == 0)
// 		return 0;
// 	// imprime la información de depuración
// 	dev_dbg(&rpdev->dev, "msg(<- src 0x%x) len %d\n", src, len);
	
// 	// muestra el contenido del mensaje recibido a través de
// 	print_hex_dump(KERN_DEBUG, __func__, DUMP_PREFIX_NONE, 16, 1,
// 			data, len,  true);

// 	// Evita que varias tareas o hilos accedan simultáneamente 
// 	// a la misma sección crítica del código y causen errores o corrupción de datos. 
// 	spin_lock_bh(&cport->rx_lock);

// 	// copia el mensaje en el buffer, adquiere el bloqueo del puerto serie del dispositivo, 
// 	// prepara el buffer de datos del puerto serie. Si space = 0, desbloquea el puerto.
// 	space = tty_prepare_flip_string(&cport->port, &cbuf, len);
// 	if (space <= 0) {
// 		dev_err(&rpdev->dev, "No memory for tty_prepare_flip_string\n");
// 		spin_unlock_bh(&cport->rx_lock);
// 		return -ENOMEM;
// 	}
// 	// copiar un bloque de memoria de tamaño len, de data al búfer de caracteres cbuf, 
// 	// que apunta a una zona de memoria reservada para el puerto tty
// 	memcpy(cbuf, data, len);
// 	// libera el mensaje del buffer
// 	tty_flip_buffer_push(&cport->port);
// 	// libera el bloqueo del puerto serie
// 	spin_unlock_bh(&cport->rx_lock);

// 	return 0;
// }

// Funcion agregada por RJCG

static int rpmsg_tty_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct rpmsgtty_port *cport = dev_get_drvdata(&rpdev->dev);
	// Para la ubicación exacta del archivo, usar la ruta absoluta:
	char *filename = "/samba/public/datos_rpmsg.txt";
	struct file *file;
	mm_segment_t old_fs;
	loff_t pos;
	ssize_t written;
	loff_t file_size;

	if (len == 0)
		return 0;
	
	// Imprime un mensaje de debug
//	dev_dbg(&rpdev->dev, "msg(<- src 0x%x) len %d\n", src, len);
	
	// Imprime un volcado hexadecimal de los datos
//	print_hex_dump(KERN_DEBUG, __func__, DUMP_PREFIX_NONE, 16, 1,
//			data, len, true);
	
	// Evita que varias tareas o hilos accedan simultáneamente
	// a la misma sección crítica del código y causen errores o corrupción de datos.
	spin_lock_bh(&cport->rx_lock);
	
	// Abre el archivo en modo escritura, crea el archivo si no existe y sobrescribe su contenido
	file = filp_open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (IS_ERR(file)) {
		// Si hay un error al abrir el archivo, imprime un mensaje de error
		dev_err(&rpdev->dev, "Failed to open file: %s\n", filename);
		return PTR_ERR(file);
	}
	
	// Guarda el valor actual del segmento de espacio de archivos
	old_fs = get_fs();
	// Establece el segmento de espacio de archivos al espacio de datos del kernel
	set_fs(KERNEL_DS);

	pos = 0;
	// Escribe los datos en el archivo
	written = kernel_write(file, data, len, &pos);
	if (written < 0) {
		// Si hay un error al escribir en el archivo, imprime un mensaje de error
		dev_err(&rpdev->dev, "Failed to write to file: %s\n", filename);
		filp_close(file, NULL);
		return written;
	}
	
	// Restaura el valor anterior del segmento de espacio de archivos
	set_fs(old_fs);
	// Cierra el archivo
	filp_close(file, NULL);

	file = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(file)) {
		dev_err(&rpdev->dev, "Failed to open file: %s\n", filename);
		return PTR_ERR(file);
	}

	// Obtener el tamaño del archivo
	file_size = vfs_llseek(file, 0, SEEK_END);
	filp_close(file, NULL);

	if (file_size >= MAX_FILE_SIZE) {
		// Si el archivo ha alcanzado o superado el tamaño máximo, se sobrescribe
		file = filp_open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (IS_ERR(file)) {
			dev_err(&rpdev->dev, "Failed to open file: %s\n", filename);
			return PTR_ERR(file);
		}
		filp_close(file, NULL);
	}

	// libera el mensaje del buffer
	tty_flip_buffer_push(&cport->port);
	// libera el bloqueo del puerto serie
	spin_unlock_bh(&cport->rx_lock);

	return 0;
}


// hasta aqui

static struct tty_port_operations  rpmsgtty_port_ops = { };

static int rpmsgtty_install(struct tty_driver *driver, struct tty_struct *tty)
{
	struct rpmsgtty_port *cport = driver->driver_state;

	return tty_port_install(&cport->port, driver, tty);
}

static int rpmsgtty_open(struct tty_struct *tty, struct file *filp)
{
	return tty_port_open(tty->port, tty, filp);
}

static void rpmsgtty_close(struct tty_struct *tty, struct file *filp)
{
	return tty_port_close(tty->port, tty, filp);
}

/*
* Obtiene el puerto rptty_port asociado con el dispo. tty y el canal RPMsg
* asignado; rpdev. Verifica que el búfer de entrada buf no sea	nulo. 
* Luego, se envían los datos dividiéndolos en mensajes de tamaño fijo
* utilizando la función rpmsg_send.  Si rpmsg_send falla, la función 
* devolverá el valor de retorno no nulo de la llamada. 
* Finalmente, la función devuelve el número total de bytes escritos.
*/
static int rpmsgtty_write(struct tty_struct *tty, const unsigned char *buf,
			 int total)
{
	int count, ret = 0;
	const unsigned char *tbuf;
	struct rpmsgtty_port *rptty_port = container_of(tty->port,
			struct rpmsgtty_port, port);
	struct rpmsg_device *rpdev = rptty_port->rpdev;

	if (buf == NULL) {
		pr_err("buf shouldn't be null.\n");
		return -ENOMEM;
	}

	count = total;
	tbuf = buf;
	do {
		/*
		* send a message to our remote processor
		* rpdev->ept: Es el punto final (endpoint) del canal de comunicación RPMsg utilizado para enviar el mensaje.
		* (void *)tbuf: Es el búfer que contiene los datos del mensaje que se enviará.
		* count > RPMSG_MAX_SIZE ? RPMSG_MAX_SIZE : count: Determina el tamaño del mensaje a enviar. 
		* Si count es mayor que RPMSG_MAX_SIZE, se envía un máximo de RPMSG_MAX_SIZE bytes. 
		* Esto se hace para asegurarse de que el mensaje no exceda el tamaño máximo permitido por el protocolo RPMsg.
		*/
		ret = rpmsg_send(rpdev->ept, (void *)tbuf,
			count > RPMSG_MAX_SIZE ? RPMSG_MAX_SIZE : count);
		if (ret) {
			// Si ocurre un error durante el envío del mensaje, se muestra un mensaje de error 
			// y se devuelve el código de error al llamador de la función.
			dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
			return ret;
		}

		if (count > RPMSG_MAX_SIZE) {
			count -= RPMSG_MAX_SIZE;
			tbuf += RPMSG_MAX_SIZE;
		} else {
			count = 0;
		}
	} while (count > 0);

	return total;
}

/*
* informar sobre la cantidad de espacio disponible en el buffer de envío
* de un puerto TTY., la función devuelve el valor de RPMSG_MAX_SIZE, 
* que es una constante definida 
*/
static int rpmsgtty_write_room(struct tty_struct *tty)  // Originalmente unsigned por RJCG
{
	/* report the space in the rpmsg buffer */
	return RPMSG_MAX_SIZE;
}

static const struct tty_operations imxrpmsgtty_ops = {
	.install		= rpmsgtty_install,
	.open			= rpmsgtty_open,
	.close			= rpmsgtty_close,
	.write			= rpmsgtty_write,
	.write_room		= rpmsgtty_write_room,   // Descomentado por RJCG
};

/*
* Inicializa y registra un controlador TTY para el dispositivo rpmsg, 
* lo que permitirá la comunicación con el dispositivo a través de la 
* interfaz de línea de comandos (TTY).
*/
static int rpmsg_tty_probe(struct rpmsg_device *rpdev)
{
/*
* Registra el controlador de dispositivo TTY llamando a "tty_register_driver", 
* lo que permite que los procesos del usuario se comuniquen con el dispositivo 
* a través de la interfaz TTY. Si la llamada a esta función falla, se cancela 
* el registro y se libera la memoria asignada al controlador de dispositivo.
*/	
	int ret;
	
	// Crea un objeto "cport" de tipo "rpmsgtty_port", utilizado para mantener
	// el estado de la comunicación TTY con el dispositivo rpmsg.
	struct rpmsgtty_port *cport;
	// Crea un controlador de dispo. TTY sin numerar. Utiliza "tty_alloc_driver".
	struct tty_driver *rpmsgtty_driver;
	
	// Inicializa estructura que representa la terminal TTY, junto con su bloqueo
	// de recepción y el objeto "rpdev" que representa el dispositivo rpmsg.
	dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n",
			rpdev->src, rpdev->dst);
			
	// Inicializa "cport->port", estructura que repre. la terminal TTY, junto con su
	// bloqueo de recepción y el objeto "rpdev" que repre. el dispositivo rpmsg.
	cport = devm_kzalloc(&rpdev->dev, sizeof(*cport), GFP_KERNEL);
	if (!cport)
		return -ENOMEM;

	// Registra el controlador de dispo. TTY llamando a "tty_register_driver", 
	// que permite que los procesos de usuario se comuniquen con el dispositivo
	// a través de la interfaz TTY. Si la llamada a esta función falla, se cancela 
	// el registro y se libera la memoria asignada al controlador de dispositivo.
	rpmsgtty_driver = tty_alloc_driver(1, TTY_DRIVER_UNNUMBERED_NODE);
	if (IS_ERR(rpmsgtty_driver)) {
		kfree(cport);
		return PTR_ERR(rpmsgtty_driver);
	}
	
	// nombre del controlador de dispositivo como "rpmsg_tty".
	rpmsgtty_driver->driver_name = "rpmsg_tty";
	// Establece el nombre del dispositivo TTY creado por el controlador como "ttyRPMSG" 
	// seguido de un número que identifica 
	rpmsgtty_driver->name = kasprintf(GFP_KERNEL, "ttyRPMSG%d", rpdev->dst);
	// Número principal de dispositivo a utilizar. Dinámicamente en tiempo de ejecución
	rpmsgtty_driver->major = UNNAMED_MAJOR;
	// Número menor de dispositivo a utilizar. En este caso, se utiliza el valor 0.
	rpmsgtty_driver->minor_start = 0;
	// Tipo de dispositivo TTY que se está creando. En este caso, un dispositivo de consola.
	rpmsgtty_driver->type = TTY_DRIVER_TYPE_CONSOLE;
	// valores iniciales de la estructura termios, utiliza config. opciones de la línea del TTY.
	rpmsgtty_driver->init_termios = tty_std_termios;

	// Asigna las operac.del controlador de dispo. TTY, al control. de dispo. de TTY: rpmsgtty_driver
	tty_set_operations(rpmsgtty_driver, &imxrpmsgtty_ops);

	//  Inicializa el puerto TTY que se utiliza para la comunicación de mensajes remotos.
	tty_port_init(&cport->port);
	// signa las operaciones del puerto TTY definidas en rpmsgtty_port_ops al puerto TTY cport->port
	cport->port.ops = &rpmsgtty_port_ops;
	// Inicializa el bloqueo de spin utilizado, para proteger acceso a datos recib. del puerto TTY.
	spin_lock_init(&cport->rx_lock);
	// Asocia el dispositivo rpmsg al puerto TTY correspondiente.
	cport->rpdev = rpdev;
	// Establece controlador de dispo. TTY cport como datos privados del dispo. rpmsg indicado.
	dev_set_drvdata(&rpdev->dev, cport);
	// Establece el estado del controlador de dispositivo como el puerto TTY cport
	rpmsgtty_driver->driver_state = cport;
	// Asocia el controlador de dispositivo TTY rpmsgtty_driver al puerto TTY correspondiente.
	cport->rpmsgtty_driver = rpmsgtty_driver;

	// registra un controlador de dispositivo TTY para que el kernel 
	// lo asocie con los dispositivos TTY correspondientes.
	ret = tty_register_driver(cport->rpmsgtty_driver);
	// Si el registro falla, imprime mensaje de error y se salta a etiqueta error1.
	if (ret < 0) {
		pr_err("Couldn't install rpmsg tty driver: ret %d\n", ret);
		goto error1;
	// Si tiene éxito, se imprime un mensaje de información.
	} else {
		pr_info("Install rpmsg tty driver!\n");
	}

	/*
	* envía un mensaje al rpmsg al procesador remoto, 
	* informándole sobre el nuevo canal.
	 */
	ret = rpmsg_send(rpdev->ept, MSG, strlen(MSG));
	if (ret) {
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
		//  Si el envío del mensaje falla, salta a la etiqueta de error error.
		goto error;
	}

	return 0;

error:
	tty_unregister_driver(cport->rpmsgtty_driver);
error1:
	tty_driver_kref_put(cport->rpmsgtty_driver);
	tty_port_destroy(&cport->port);
	cport->rpmsgtty_driver = NULL;
	kfree(cport);

	return ret;
}

/*
 * manejador de evento para la eliminación de un dispositivo RPMsg (remoto). 
 * Lo que hace es eliminar el controlador de terminal TTY asociado al 
 * dispositivo RPMsg y destruir la estructura de puerto struct rpmsgtty_port
*/
static void rpmsg_tty_remove(struct rpmsg_device *rpdev)
{
	// Obtiene la estructura de puerto struct rpmsgtty_port
	struct rpmsgtty_port *cport = dev_get_drvdata(&rpdev->dev);

	// Imprime un mensaje de información en los registros del sistema.
	dev_info(&rpdev->dev, "rpmsg tty driver is removed\n");

	// Desregistra el controlador de terminal TTY
	tty_unregister_driver(cport->rpmsgtty_driver);
	
	// Libera la memoria asignada para el nombre del controlador de terminal TTY
	kfree(cport->rpmsgtty_driver->name);
	
	// Decrementa el contador de referencia del controlador de terminal TTY
	tty_driver_kref_put(cport->rpmsgtty_driver);
	
	// Destruye la estructura de puerto TTY
	tty_port_destroy(&cport->port);
	
	// Establece el puntero rpmsgtty_driver en estructura rpmsgtty_port como NULL.
	cport->rpmsgtty_driver = NULL;
}

/*
	* Lista de identificadores de dispositivos: contiene el nombre del canal de
	* comunicación RPMsg que se utilizará para la comunicación;  para que un
	* controlador de dispositivo pueda buscar y detectar esos dispositivos.
*/
static struct rpmsg_device_id rpmsg_driver_tty_id_table[] = {
	{ .name	= "rpmsg-virtual-tty-channel-1" },
	{ .name	= "rpmsg-virtual-tty-channel" },
	{ .name = "rpmsg-openamp-demo-channel" },
	{ },
};

/*
* Define los detalles del controlador de dispositivo RPMsg específico para la 
* consola de serie emulada a través de RPMsg. 
*/
static struct rpmsg_driver rpmsg_tty_driver = {
	.drv.name	= KBUILD_MODNAME,        		// nombre del controlador de dispositivo.
	.drv.owner	= THIS_MODULE,           		// propietario del controlador 
	.id_table	= rpmsg_driver_tty_id_table, 	// lista de identificadores soportados
	.probe		= rpmsg_tty_probe,       		// sonda, que se llama al encontrar disp. compati.
	.callback	= rpmsg_tty_cb,          		// la función de devolución al recivir mensa.
	.remove		= rpmsg_tty_remove,      		// Lllamada al eliminar un dispositivo.
};


static int __init init(void)
{
	return register_rpmsg_driver(&rpmsg_tty_driver);
}

static void __exit fini(void)
{
	unregister_rpmsg_driver(&rpmsg_tty_driver);
}
module_init(init);
module_exit(fini);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("iMX virtio remote processor messaging tty driver");
MODULE_LICENSE("GPL v2");
