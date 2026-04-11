#ifndef UART_MODULE_H
#define UART_MODULE_H

// Defining the serial port struct, the definition is hidden in the
// c file as no one really needs to know this.
struct _SerialPort;
typedef struct _SerialPort SerialPort;


extern SerialPort USART1_PORT;


enum {
  BAUD_9600,
  BAUD_19200,
  BAUD_38400,
  BAUD_57600,
  BAUD_115200
};

// initialize serial port
// call this before talking over serial
void SerialInitialise(uint32_t baudRate, SerialPort *serial_port);

// output a character over serial
void SerialOutputChar(uint8_t, SerialPort *serial_port);

// output a string over serial, must be null terminated
void SerialOutputString(uint8_t *pt, SerialPort *serial_port);


#endif
