#include "xc.h"
// Definiciones para la gestión dinámica de la trama Dacai
#define BUFFER_MAX_LARGO 64      // Capacidad máxima de almacenamiento
#define BYTE_HEADER      0xEE    // Inicio de comando

// =============================================================================
// VARIABLES GLOBALES PARA LA RECEPCIÓN DINÁMICA
// =============================================================================
volatile uint8_t rxBuffer[BUFFER_MAX_LARGO]; // Búfer de datos
volatile uint16_t rxIndex = 0;               // Índice de escritura del búfer
volatile uint16_t bytesRecibidos = 0;        // Guarda la longitud final de la trama
volatile uint8_t tramaLista = 0;             // Bandera: 1 indica comando completo listo


void System_Clock_Init(void);
void UART1_Pins_Init(void);
void UART1_Init(unsigned long baudrate);
void Check_UART1_Errors(void);
void Procesar_Comando_Dacai(void);
void UART1_Write(char TxByte);
void UART1_Write_Text(const char* text);
void UART1_Write_Ln(void);
void configurar_pines_led(void);


// =============================================================================
// CONTROLADOR DE INTERRUPCIÓN DE RECEPCIÓN DINÁMICA (UART1 RX)
// =============================================================================
void __attribute__((__interrupt__, __auto_psv__)) _U1RXInterrupt(void)
{
    IFS0bits.U1RXIF = 0; // Limpiar la bandera de interrupción de hardware
    
    while(U1STAbits.URXDA) // Leer mientras existan datos en el FIFO
    {
        uint8_t incomingByte = U1RXREG;
        
        // Si ya hay una trama procesándose en el main, descartamos bytes nuevos por seguridad
        if (tramaLista == 1)
        {
            continue;
        }
        
        // MÁQUINA DE ESTADOS COMPORTAMIENTO DINÁMICO
        if (rxIndex == 0)
        {
            // Estado de Sincronización: Buscando estrictamente el byte de inicio (0xEE)
            if (incomingByte == BYTE_HEADER)
            {
                rxBuffer[rxIndex++] = incomingByte;
            }
        }
        else
        {
            // Estado de Captura: Almacenar el byte actual en el búfer
            rxBuffer[rxIndex++] = incomingByte;
            
            // Protección contra desbordamiento de memoria (Buffer Overflow)
            if (rxIndex >= BUFFER_MAX_LARGO)
            {
                rxIndex = 0; // Descartar datos corruptos y reiniciar búsqueda
                continue;
            }
            
            // EVALUACIÓN CONTINUA DE LA SECUENCIA DE ESCAPE (Mínimo 5 bytes en total)
            if (rxIndex >= 5)
            {
                // Evaluamos si los últimos 4 bytes capturados corresponden al cierre: FF FC FF FF
                if (rxBuffer[rxIndex - 4] == 0xFF &&
                    rxBuffer[rxIndex - 3] == 0xFC &&
                    rxBuffer[rxIndex - 2] == 0xFF &&
                    rxBuffer[rxIndex - 1] == 0xFF)
                {
                    // ¡Secuencia de fin detectada con éxito!
                    bytesRecibidos = rxIndex; // Guardamos la longitud exacta de lo recibido
                    tramaLista = 1;           // Se avisa al ciclo principal para procesar
                }
            }
        }
    }
}



void Procesar_Comando_Dacai(void)
{
    // Evaluamos el tipo de entrada
    if (rxBuffer[0] == 0xEE)
    {
        //Botones
        if (rxBuffer[1] == 0xB1 && rxBuffer[2] == 0x01 && rxBuffer[3] == 0x00)
        {
            //Para verificar en que pagina estamos (7 paginas)
            pagina_actual = rxBuffer[4];
        }
    }
}

void System_Clock_Init(void) 
{
    CLKDIVbits.RCDIV = 0; // FRC completo al PLL para 32MHz
    __delay_us(20); 
}

void UART1_Pins_Init(void) 
{
    TRISDbits.TRISD2 = 0;   // Pin 49 como TX (RP10)
    TRISDbits.TRISD3 = 1;   // Pin 50 como RX (RP17)
    
    __builtin_write_OSCCONL(OSCCON & 0xBF); // Desbloqueo PPS
    RPOR5bits.RP10R = 3;    // Asigna U1TX a RP10
    RPINR18bits.U1RXR = 17; // Asigna U1RX a RP17
    __builtin_write_OSCCONL(OSCCON | 0x40); // Bloqueo PPS
}

void UART1_Init(unsigned long baudrate) 
{
    U1MODEbits.UARTEN = 0;  // Apagar módulo
    U1MODEbits.BRGH = 1;    // Alta velocidad activa
    U1MODEbits.PDSEL = 0b00;// 8 bits, sin paridad
    U1MODEbits.STSEL = 0;   // 1 bit stop
    
    U1BRG = (unsigned int)((FCY / (4.0 * baudrate)) - 0.5); // División por hardware entre 4
    
    U1STA = 0;
    
    // Configuración de la Interrupción de Recepción UART1
    IPC2bits.U1RXIP = 5;    // Nivel de prioridad 5
    IFS0bits.U1RXIF = 0;    // Limpiar bandera
    IEC0bits.U1RXIE = 1;    // Habilitar la interrupción
    
    U1MODEbits.UARTEN = 1;  // Encender módulo globalmente
    U1STAbits.UTXEN = 1;    // Habilitar pin de transmisión
}

void Check_UART1_Errors(void) 
{
    if(U1STAbits.OERR == 1) {
        U1STAbits.OERR = 0; // Evita el congelamiento de la UART por Overflow
    }
}

void UART1_Write(char TxByte) 
{
    while(U1STAbits.UTXBF); // Esperar si el buffer de transmisión está lleno
    U1TXREG = TxByte;
}

void UART1_Write_Text(const char* text) 
{
    while(*text) 
    {
        UART1_Write(*text++); 
    }
}

void UART1_Write_Ln(void) 
{
    UART1_Write('\r'); 
    UART1_Write('\n'); 
}

void configurar_pines_led(void)
{
    ANSEbits.ANSE5 = 0;
    TRISEbits.TRISE5 = 0;
}
