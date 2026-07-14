//Configuraciones del reloj
#pragma config FNOSC = FRC       // Oscilador Primario (Directo, sin PLL)
//#pragma config POSCMD = XT        // Modo XT (Cristal de 3MHz a 10MHz)
#pragma config OSCIOFCN = OFF     // CLKO habilitado: Fosc/2 sale por el pin OSCO
#pragma config FCKSM = CSDCMD    // Cambio de reloj y Monitor de reloj desactivados

// Otros ajustes de seguridad (Recomendados para que el micro arranque)
#pragma config FWDTEN = 0      // Watchdog Timer desactivado
#pragma config ICS = PGx1        // Debug sobre los pines EMUC1/EMUD1
#pragma config GWRP = OFF        // Protección contra escritura de Flash desactivada
#pragma config JTAGEN = 0       //Este es para que funcione el pin A0

// Definición para cálculos de tiempo (8MHz / 2 = 4 MIPS)
#define FOSC 8000000UL
#define FCY (FOSC / 2) 


#include <libpic30.h>

#include "xc.h"

void config_leds(void)
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
    
    //Inicializamos en bajo
    LATGbits.LATG15 = 0;
    LATEbits.LATE5 = 0;
    LATEbits.LATE7 = 0;
    LATCbits.LATC1 = 0;
    LATCbits.LATC2 = 0;
    LATCbits.LATC3 = 0;
    LATCbits.LATC4 = 0;
    LATGbits.LATG6 = 0;
}

#include "xc.h"

int main(void) 
{
    config_leds();
    
    //configuramos las entradas del dip switch
    //Boton1 RG7
    //Boton2 RG8
    //Boton3 RG9
    //Boton4 RA0
    //Btono5 RE8
    //Boton6 RE9
    //Boton7 RB4
    //Boton8 RA9
    
    //Configuramos las pines como digitales
    ANSGbits.ANSG7 = 0;
    ANSGbits.ANSG8 = 0;
    ANSGbits.ANSG9 = 0;
    //ANSAbits.ANS0 = 0;
    //ANSEbits.ANSE8 = 0;
    ANSEbits.ANSE9 = 0;
    ANSBbits.ANSB4 = 0;
    ANSAbits.ANSA9 = 0;
    
    //Configuramos los pines como entrada
    TRISGbits.TRISG7 = 1;
    TRISGbits.TRISG8 = 1;
    TRISGbits.TRISG9 = 1;
    TRISAbits.TRISA0 = 1;
    TRISEbits.TRISE8 = 1;
    TRISEbits.TRISE9 = 1;
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA9 = 1;
    
    while(1)
    {
        
        LATGbits.LATG15 = PORTGbits.RG7; // Switch RG7 controla LED RG15
        LATEbits.LATE5  = PORTGbits.RG8; // Switch RG8 controla LED RE5
        LATEbits.LATE7  = PORTGbits.RG9; // Switch RG9 controla LED RE7
        LATCbits.LATC1  = PORTAbits.RA0; // Switch RA0 controla LED RC1
        LATCbits.LATC2  = PORTEbits.RE8; // Switch RE8 controla LED RC2
        LATCbits.LATC3  = PORTEbits.RE9; // Switch RE9 controla LED RC3
        LATCbits.LATC4  = PORTBbits.RB4; // Switch RB4 controla LED RC4
        LATGbits.LATG6  = PORTAbits.RA9; // Switch RA9 controla LED RG6
    }
    
    return 0;
}
