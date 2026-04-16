#ifndef UART_MODULE_H
#define UART_MODULE_H

#include <stdint.h>
#include <stddef.h>

// Defining the serial port struct, the definition is hidden in the
// c file as no one really needs to know this.
struct _SerialPort;
typedef struct _SerialPort SerialPort;


extern SerialPort USART1_PORT;

// baud rate constants are defined here
enum {
  BAUD_9600,
  BAUD_19200,
  BAUD_38400,
  BAUD_57600,
  BAUD_115200
};

/*
 * Initialize the serial port
 * This function needs to be called before using the serial port
 * Pass in a baud rate enum to this function to configure the baud rate
 * Completion function specifies the function called after a message buffer is received
 * The data buffer, the size of the data buffer and the message id is passed into the callback
 */
void serialInitialise(SerialPort *serial_port, uint32_t baudRate, void (*completion_function)(uint8_t*, uint8_t, uint8_t));


/*
 * Sends a string asynchronously over the serial port
 * There's 2 cases: the previous string buffer has been sent or it hasn't been sent
 * If it hasn't been sent, this function will spin until it sees that the previous buffer has been sent
 * If it has been sent, this function will then send the string asynchronously
 */
void sendString(SerialPort *serial_port, char* string);

/*
 * Sends a buffer asynchronously over the serial port
 * The behavior works similarly to sendString except that the message is placed in a data frame
 * The frame consists of a STX <message size> <message id> <buffer> ETX <checksum>
 * STX is 0x02, ETX is 0x03, the checksum used is BCC 8-bit XOR checksum
 *
 * To use this function make sure to cast the buffer to uint8_t*
 */
void sendMsg(SerialPort *serial_port, uint8_t *buffer, uint8_t size, uint8_t message_id);


/*
 * Receives a message from the buffer asynchronously
 * Asynchronously means that the message buffer is filled independent of the main program execution
 * This is possible due to the double buffer approach
 * There's 2 cases, the receive buffer has been filled or the receive buffer has not been filled
 * If the receive buffer has been filled, the complete function callback will immediately be called with the data
 * If the receive buffer has not been filled, it will wait until the buffer is filled then call the callback
 *
 * One way to think of this function is like a request to get the latest data
 * Note that the complete function callback will not be called every time new data comes in
 * It is only called in response to this function call
 */
void receiveMsg(SerialPort *serial_port);

/*
 * Test Functions
 */

void testSerial();

void testSerialString();

#endif
