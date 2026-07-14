//Configuraciones del reloj
#pragma config FNOSC = FRC       // Oscilador Primario (Directo, sin PLL)
//#pragma config POSCMD = XT        // Modo XT (Cristal de 3MHz a 10MHz)
#pragma config OSCIOFCN = OFF     // CLKO habilitado: Fosc/2 sale por el pin OSCO
#pragma config FCKSM = CSDCMD    // Cambio de reloj y Monitor de reloj desactivados

#pragma config SOSCSEL = 0       // ¡CRÍTICAL! SOSC en OFF deshabilita el oscilador secundario y libera RG2/SCL1 para uso digital/I2C
#pragma config POSCMD = 0        // Oscilador primario deshabilitado

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
#include "oled_ssd1306.h"

int main(void) {
    // Asegurar que el reloj corra a 8 MHz exactos sin divisiones
    CLKDIVbits.RCDIV = 0; 
    
    // Desactivar módulos analógicos y periféricos redundantes
    ANSELG = 0x0000;      
    CM2CONbits.CON = 0;   
    LCDCONbits.LCDEN = 0; 

    // Configurar LED testigo de RC2
    TRISCbits.TRISC2 = 0;
    LATCbits.LATC2 = 0; 

    // Inicializaciones de sistema
    I2C1_Init();          // Encender el periférico I2C1 del PIC
    OLED_Init();          // Despertar la pantalla OLED
    OLED_Clear_To_Black(); // Borrar pantalla para iniciar limpia

    // --- EJEMPLO DE ESCRITURA EN DIFERENTES SECCIONES ---
    
    // Fila superior (Página 1), Iniciando en columna 10
    OLED_Set_Cursor(1, 10);
    OLED_Print_String("HOLA DESDE PIC24F");

    // Fila inferior (Página 5), Iniciando en columna 30
    OLED_Set_Cursor(5, 30);
    OLED_Print_String("8 MHZ OK");

    // Indicar éxito de fin de programa en el LED de la placa
    LATCbits.LATC2 = 1; 

    while(1) {
        // Tu código de aplicación puede ejecutarse libremente aquí...
    }
}