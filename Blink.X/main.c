//Este codigo se utiliza para probar los leds de la tarjeta de desarrollo
//Usando el reloj interno a 8MHz
#include <xc.h>
///declaramos las configuraciones de inicio
//Configuraciones del reloj
#pragma config FNOSC = FRC       // Oscilador Primario (Directo, sin PLL)
#pragma config POSCMD = XT        // Modo XT (Cristal de 3MHz a 10MHz)
#pragma config OSCIOFCN = OFF     // CLKO habilitado: Fosc/2 sale por el pin OSCO
#pragma config FCKSM = CSDCMD    // Cambio de reloj y Monitor de reloj desactivados

// Otros ajustes de seguridad (Recomendados para que el micro arranque)
#pragma config FWDTEN = 0      // Watchdog Timer desactivado
#pragma config ICS = PGx1        // Debug sobre los pines EMUC1/EMUD1
#pragma config GWRP = OFF        // Protección contra escritura de Flash desactivada

// Definición para cálculos de tiempo (8MHz / 2 = 4 MIPS)
#define FOSC 8000000UL
#define FCY (FOSC / 2) 

#include <libpic30.h>


#include "xc.h"

int main(void) 
{
    //LED1 RG15
    //LED2 RE5
    //LED3 RE7
    //LED4 RC1
    //LED5 RC2
    //LED6 RC3
    //LED7 RC4
    //LED8 RG6
    
    //Configuramos los leds como digital
    //ANSGbits.ANSG15 = 0;
    ANSEbits.ANSE5 = 0;
    ANSEbits.ANSE7 = 0;
    //ANSCbits.ANSC1 = 0;
    //ANSCbits.ANSC2 = 0;
    //ANSCbits.ANSC3 = 0;
    ANSCbits.ANSC4 = 0;
    ANSGbits.ANSG6 = 0;
    
    //Configuramos los pines como salida
    TRISGbits.TRISG15 = 0;
    TRISEbits.TRISE5 = 0;
    TRISEbits.TRISE7 = 0;
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    TRISCbits.TRISC3 = 0;
    TRISCbits.TRISC4 = 0;
    TRISGbits.TRISG6 = 0;
    
    while(1)
    {
        LATGbits.LATG15 = 1;
        LATEbits.LATE5 = 1;
        LATEbits.LATE7 = 1;
        LATCbits.LATC1 = 1;
        LATCbits.LATC2 = 1;
        LATCbits.LATC3 = 1;
        LATCbits.LATC4 = 1;
        LATGbits.LATG6 = 1;
        __delay_ms(500);
        LATGbits.LATG15 = 0;
        LATEbits.LATE5 = 0;
        LATEbits.LATE7 = 0;
        LATCbits.LATC1 = 0;
        LATCbits.LATC2 = 0;
        LATCbits.LATC3 = 0;
        LATCbits.LATC4 = 0;
        LATGbits.LATG6 = 0;
        __delay_ms(500);
    }
    
    
    return 0;
}
