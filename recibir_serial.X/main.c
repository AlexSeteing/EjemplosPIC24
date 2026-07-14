// Configuración serial mediante los pines 49 y 50 del PIC
// Fosc = 32MHz y Fcy = 16MHz (FRC con PLL activo)

#include "xc.h"

// =============================================================================
// CONFIGURACIÓN DE FUSIBLES (ESTRICTA)
// =============================================================================
#pragma config POSCMD = NONE     // Oscilador primario deshabilitado
#pragma config FNOSC = FRCPLL    // Activa el FRC con multiplicador PLL de fábrica
#pragma config OSCIOFCN = OFF   // CLKO deshabilitado
#pragma config FCKSM = CSDCMD    // Cambio de reloj desactivado
#pragma config SOSCSEL = 0       // SOSC deshabilitado

// Ajustes de seguridad obligatorios
#pragma config FWDTEN = 0        // Watchdog Timer apagado por completo en hardware
#pragma config ICS = PGx1        // Debug sobre los pines EMUC1/EMUD1
#pragma config GWRP = OFF        // Protección contra escritura desactivada
#pragma config JTAGEN = 0        // JTAG apagado para liberar pines digitales

#define FOSC 32000000UL
#define FCY (FOSC / 2)           // 16,000,000 Hz

#include <libpic30.h>

int main(void) 
{
    // =========================================================================
    // 1. AJUSTE DE RELOJ DE HARDWARE (Desbloqueo de registros críticos)
    // =========================================================================
    // En el PIC24F, para cambiar CLKDIV de forma efectiva se recomienda hacerlo 
    // mediante la secuencia de desbloqueo integrada o asegurando el borrado.
    __builtin_write_OSCCONH(0x01); // Selecciona FRC con PLL
    
    CLKDIVbits.RCDIV = 0;          // FRC sin dividir (8 MHz)
    
    // Esperar a que el PLL se estabilice físicamente en el silicio
    while(OSCCONbits.LOCK == 0); 

    // Apagamos el Watchdog por completo también en el registro de control
    RCONbits.SWDTEN = 0; 

    // =========================================================================
    // 2. CONFIGURACIÓN DE PINES (PPS)
    // =========================================================================
    TRISDbits.TRISD2 = 0;        // Pin físico 49 como salida digital TX
    TRISDbits.TRISD3 = 1;        // Pin físico 50 como entrada digital RX
    
    __builtin_write_OSCCONL(OSCCON & 0xBF); // Desbloqueo PPS
    RPOR5bits.RP10R = 3;         // Asigna U1TX al pin RP10 (Pin físico 49)
    RPINR18bits.U1RXR = 17;      // Asigna U1RX al pin RP17 (Pin físico 50)
    __builtin_write_OSCCONL(OSCCON | 0x40); // Bloqueo PPS

    // =========================================================================
    // 3. CONFIGURACIÓN DE UART1 (Sondeo puro)
    // =========================================================================
    U1MODEbits.UARTEN = 0;       // Apagar módulo para cambios limpios
    U1MODEbits.BRGH = 1;         // Alta velocidad activa (Divisor entre 4)
    U1MODEbits.PDSEL = 0b00;     // 8 bits de datos, sin paridad
    U1MODEbits.STSEL = 0;        // 1 bit de parada
    
    // Si el reloj está a 16MHz reales, 416 da 9600 baudios.
    // Si por alguna razón tu FRC interno no enganchó el PLL y corre a 4MHz (2MIPS),
    // el valor para 9600 sería 51. Dejamos 416 que es el correcto para el PLL.
    U1BRG = 416;                 
    
    U1STA = 0;                   // Limpiar estados iniciales
    
    // Deshabilitar interrupciones para evitar caídas de sistema
    IEC0bits.U1RXIE = 0;
    IEC0bits.U1TXIE = 0;
    
    // Limpieza del buffer receptor
    char dummy;
    while(U1STAbits.URXDA) {
        dummy = U1RXREG;
    }
    
    U1MODEbits.UARTEN = 1;       // Encender el módulo UART1 global
    U1STAbits.UTXEN = 1;         // Activar pin de transmisión
    
    __delay_ms(200);             // Estabilización

    // =========================================================================
    // 4. BUCLE PRINCIPAL (ECO INTEGRAL)
    // =========================================================================
    // =========================================================================
    // 4. BUCLE DE DIAGNÓSTICO EN TIEMPO REAL (IGNORAMOS RX)
    // =========================================================================
    while(1) 
    {
        // Enviamos 'O'
        while(U1STAbits.UTXBF); 
        U1TXREG = 'O';
        
        // Enviamos 'K'
        while(U1STAbits.UTXBF); 
        U1TXREG = 'K';
        
        // Salto de línea
        while(U1STAbits.UTXBF); 
        U1TXREG = '\r';
        while(U1STAbits.UTXBF); 
        U1TXREG = '\n';
        
        // Esperamos 2 segundos antes de volver a enviar
        __delay_ms(2000); 
    }
    
    return 0;
}