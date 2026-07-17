// Configuración serial mediante los pines 49 y 50 del PIC
// REPARADO: Reloj interno con PLL activo -> Fosc = 32MHz y Fcy = 16MHz

#include "xc.h"

// =============================================================================
// CONFIGURACIÓN CORREGIDA DEL RELOJ (FRC CON PLL CORRIENDO A 32 MHz)
// =============================================================================
#pragma config POSCMD = NONE     // Oscilador primario deshabilitado
#pragma config FNOSC = FRCPLL    // Activa el FRC con multiplicador PLL
#pragma config OSCIOFCN = OFF   // CLKO habilitado
#pragma config FCKSM = CSDCMD    // Cambio de reloj desactivado
#pragma config SOSCSEL = 0       // SOSC en OFF deshabilitado

// Ajustes de seguridad obligatorios
#pragma config FWDTEN = 0        // Watchdog Timer desactivado
#pragma config ICS = PGx1        // Debug sobre los pines EMUC1/EMUD1
#pragma config GWRP = OFF        // Protección contra escritura desactivada
#pragma config JTAGEN = 0        // JTAG apagado para liberar pines digitales

// DEFINICIONES DE RECOJ REALES DE 16 MIPS
#define FOSC 32000000UL
#define FCY (FOSC / 2)           // FCY = 16,000,000 Hz

#include <libpic30.h>

void System_Clock_Init(void);
void UART1_Pins_Init(void);
void UART1_Init(unsigned long baudrate);
void UART1_Write(char TxByte);
void UART1_Write_Text(const char* text);
void UART1_Write_Ln(void);
void Check_UART1_Errors(void);

int main(void) 
{
    System_Clock_Init(); // Fija el divisor de reloj interno a 1:1 para activar los 32MHz
    UART1_Pins_Init();   // Mapea los pines lógicos RP10 y RP17 (Pines físicos 49 y 50)
    
    // Configuramos los 9600 baudios exactos y estables
    UART1_Init(9600);    
    
    __delay_ms(100);
    
    UART1_Write_Ln();
    UART1_Write_Text("==============================================");
    UART1_Write_Ln();
    UART1_Write_Text("      SISTEMA DE DEPURACION UART OK           ");
    UART1_Write_Ln();
    UART1_Write_Text(" Microcontrolador: PIC24FJ128GA310            ");
    UART1_Write_Ln();
    UART1_Write_Text(" Frecuencia Real: 16 MIPS (32MHz con PLL)     ");
    UART1_Write_Ln();
    UART1_Write_Text(" Velocidad Bus: 9600 Baudios                  ");
    UART1_Write_Ln();
    UART1_Write_Text("==============================================");
    UART1_Write_Ln();
    
    uint16_t ciclo_contador = 0;

    while(1) {
        Check_UART1_Errors(); 
        
        DACAI_Send_Instruction(); 
        //DACAI_Send_Text("hola\0"); 
        __delay_ms(1000); 
    }
    return 0;
}

void System_Clock_Init(void) 
{
    // RCDIV = 0 asegura que los 8MHz nativos del FRC entren completos al PLL 
    // Logrando la multiplicación exacta: 8MHz * 4 = 32MHz.
    CLKDIVbits.RCDIV = 0; 
    __delay_us(20); 
}

void UART1_Pins_Init(void) 
{
    TRISDbits.TRISD2 = 0; // Pin 49 como salida digital TX
    TRISDbits.TRISD3 = 1; // Pin 50 como entrada digital RX
    
    __builtin_write_OSCCONL(OSCCON & 0xBF); // Desbloqueo de registros PPS
    
    RPOR5bits.RP10R = 3;    // Asigna U1TX al pin RP10 (Pin físico 49)
    RPINR18bits.U1RXR = 17; // Asigna U1RX al pin RP17 (Pin físico 50)
    
    __builtin_write_OSCCONL(OSCCON | 0x40); // Bloqueo de registros PPS
}

void UART1_Init(unsigned long baudrate) 
{
    U1MODEbits.UARTEN = 0;  // Apagar módulo para cambios limpios
    
    U1MODEbits.BRGH = 1;    // Alta velocidad activa (Divisor de hardware entre 4)
    U1MODEbits.PDSEL = 0b00;// 8 bits de datos, sin paridad
    U1MODEbits.STSEL = 0;   // 1 bit de parada
    
    // ¡CORREGIDO!: La fórmula matemática real para el modo BRGH = 1 usa el divisor 4.0
    U1BRG = (unsigned int)((FCY / (4.0 * baudrate)) - 0.5);
    
    U1STA = 0;              
    U1MODEbits.UARTEN = 1;  // Encender el módulo UART1 de forma global
    U1STAbits.UTXEN = 1;    // Activar pin físico de transmisión
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

void Check_UART1_Errors(void) 
{
    if(U1STAbits.OERR == 1) {
        U1STAbits.OERR = 0; 
    }
}

/**
 * @brief Envía la trama de comando específica a la pantalla TFT Dacai.
 * Instrucción: EE 61 0A FF FC FF FF
 */
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

/**
 * @brief Envía un texto dinámico a un componente específico de la pantalla Dacai.
 * @param text Puntero a la cadena de texto que deseas mostrar (Ej: "hola", "25 C", etc.)
 */
void DACAI_Send_Text(const char* text) 
{
    // 1. Asegurar que no existan errores previos en la UART
    Check_UART1_Errors(); 
    
    // 2. Enviar el encabezado obligatorio del comando (EE B1 10 00 00 00 01)
    UART1_Write(0xEE);  // Inicio
    UART1_Write(0xB1);  // Comando de texto
    UART1_Write(0x10);  // Parámetros de ID fijados en tu instrucción
    UART1_Write(0x00);  
    UART1_Write(0x00);  
    UART1_Write(0x00);  
    UART1_Write(0x01);  
    
    // 3. Enviar el texto carácter por carácter (el "hola")
    // El bucle recorrerá tu texto automáticamente hasta encontrar el final '\0'
    while(*text) 
    {
        UART1_Write(*text++); 
    }
    
    // 4. Enviar el cierre de trama obligatorio de Dacai (FF FC FF FF)
    UART1_Write(0xFF);  
    UART1_Write(0xFC);  
    UART1_Write(0xFF);  
    UART1_Write(0xFF);  
}