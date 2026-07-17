// Configuración serial y recepción dinámica por interrupción para PIC24FJ128GA310
#include "xc.h"
#include <stdint.h>


// =============================================================================
// CONFIGURACIÓN DEL RELOJ (FRC CON PLL CORRIENDO A 32 MHz)
// =============================================================================
#pragma config POSCMD = NONE     // Oscilador primario deshabilitado
#pragma config FNOSC = FRCPLL    // Activa el FRC con multiplicador PLL
#pragma config OSCIOFCN = OFF   // CLKO habilitado
#pragma config FCKSM = CSDCMD    // Cambio de reloj desactivado
#pragma config SOSCSEL = 0       // SOSC en OFF deshabilitado

#pragma config FWDTEN = 0        // Watchdog Timer desactivado
#pragma config ICS = PGx1        // Debug sobre los pines EMUC1/EMUD1
#pragma config GWRP = OFF        // Protección contra escritura desactivada
#pragma config JTAGEN = 0        // JTAG apagado para liberar pines digitales

#define FOSC 32000000UL
#define FCY (FOSC / 2)           // FCY = 16,000,000 Hz

#include <libpic30.h>

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

// Prototipos de funciones
void System_Clock_Init(void);
void UART1_Pins_Init(void);
void UART1_Init(unsigned long baudrate);
void Check_UART1_Errors(void);
void Procesar_Comando_Dacai(void);
void DACAI_Send_Instruction(void);
void UART1_Write(char TxByte);
void UART1_Write_Text(const char* text);
void UART1_Write_Ln(void);
void configurar_pines_led(void);




int main(void) 
{
    System_Clock_Init(); // Configura reloj a 32MHz
    UART1_Pins_Init();   // Mapea pines RP10 y RP17
    UART1_Init(9600);    // Inicializa UART a 9600 baudios
    
    __delay_ms(100);     // Estabilización del sistema
    
    DACAI_Send_Instruction(); 
    
    configurar_pines_led();
    
    
    while(1) 
    {
        Check_UART1_Errors(); // Limpieza constante de desbordamientos
        
        // Si la interrupción detectó el cierre de trama (FF FC FF FF)
        if (tramaLista == 1) 
        {
            Procesar_Comando_Dacai();
            
            // Limpieza manual y reset para prepararse a recibir el siguiente comando
            tramaLista = 0; 
            rxIndex = 0;
        }
        DACAI_Send_Instruction(); 
        __delay_ms(100);
    }
    return 0;
}

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

// =============================================================================
// FUNCIÓN PARA PROCESAR LOS DATOS INTERNOS DE LONGITUD VARIABLE
// =============================================================================
void Procesar_Comando_Dacai(void)
{
    // Evaluamos el tipo de comando en el índice 1 (Ej: 0xB1 para texto)
    if (rxBuffer[0] == 0xEE)
    {
        if (rxBuffer[1] == 0xB1)
        {
            if (rxBuffer[2] == 0x11)
            {
                LATEbits.LATE5 = !LATEbits.LATE5;
            }
        }
    }
    /*if (rxBuffer[1] == 0xB1)
    {
        // Ejemplo de extracción de datos:
        // Si la trama fue: EE B1 10 00 00 00 01 [DATOS...] FF FC FF FF
        // Los datos útiles de texto empiezan desde el índice 7
        
        uint16_t largoTexto = bytesRecibidos - 11; // Restamos los 7 bytes de cabecera y 4 de cierre
        
        // Aquí puedes copiar el texto recibido o tomar decisiones de control para la autoclave
        if (rxBuffer[7] == '1' && rxBuffer[8] == '2' && rxBuffer[9] == '1')
        {
            // Acción en caso de recibir un indicador específico (Ej: "121")
            Nop(); 
        }
    }*/
}

// =============================================================================
// CONFIGURACIÓN DE MÓDULOS DE HARDWARE
// =============================================================================
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

void DACAI_Send_Instruction(void) 
{
    // Aseguramos que no existan errores previos en el módulo antes de transmitir
    Check_UART1_Errors(); 
    
    UART1_Write(0xEE);  // Byte de inicio de trama (Frame Header)
    UART1_Write(0x61);  // Código de comando
    UART1_Write(0x0A);  // Parámetro / Datos
    UART1_Write(0xFF);  // Inicio de fin de trama (Frame Tail)
    UART1_Write(0xFC);  
    UART1_Write(0xFF);  
    UART1_Write(0xFF);  // Fin de la instrucción
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