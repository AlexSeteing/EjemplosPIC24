//ADC _ OLED
//Pin22 AN3

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
#include "oled_ssd1306.h"
#include <stdio.h>


void ADC_Init(void);
uint16_t ADC_ReadAN3(void);
void Pins_Output_Init(void);
void Display_8Bit(uint8_t value);
void config_oled(void);

void Pins_Output_Init(void) {
    // Apagar explícitamente el módulo LCD interno que interfiere con el puerto B y E
    LCDCONbits.LCDEN = 0;   
    
    // Configurar los 8 pines como salidas digitales
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    TRISCbits.TRISC3 = 0;
    TRISCbits.TRISC4 = 0;
    TRISEbits.TRISE5 = 0;
    TRISEbits.TRISE7 = 0;
    TRISGbits.TRISG6 = 0;
    TRISGbits.TRISG15 = 0;
    
    // Deshabilitar funciones analógicas en estos puertos de salida para asegurar lógica digital pura
    ANSC &= ~0x001E; // Desactiva analógico en RC1, RC2, RC3, RC4
    ANSE &= ~0x00A0; // Desactiva analógico en RE5, RE7
    ANSG &= ~0x8040; // Desactiva analógico en RG6, RG15
    
    // Inicializar salidas en 0
    LATCbits.LATC1 = 0;
    LATCbits.LATC2 = 0;
    LATCbits.LATC3 = 0;
    LATCbits.LATC4 = 0;
    LATEbits.LATE5 = 0;
    LATEbits.LATE7 = 0;
    LATGbits.LATG6 = 0;
    LATGbits.LATG15 = 0;
}

void ADC_Init(void) {
    // Configurar físicamente el Pin 22 (AN3 / RB3)
    TRISBbits.TRISB3 = 1;   // Entrada física
    ANSBbits.ANSB3 = 1;     // Activar el canal analógico
    
    // Configuración del ADC
    AD1CON1 = 0x0000;       
    AD1CON1bits.MODE12 = 1; // Modo de precisión a 12 bits (Muestra de 0 a 4095)
    AD1CON1bits.FORM = 0;   // Formato entero absoluto derecho
    
    // SSRC = 7 (111): El temporizador interno del ADC termina de manera automática el 
    // tiempo de muestreo (Sample) y arranca la conversión de forma autónoma y precisa.
    AD1CON1bits.SSRC = 7;   
    
    AD1CON2 = 0x0000;       // Referencias estándar: VREF+ = AVDD (3.3V), VREF- = AVSS (GND)
    
    // CALIBRACIÓN DE TIEMPOS PARA FCY = 4 MHz (Reloj de 8 MHz)
    AD1CON3bits.ADRC = 0;   // Usar el reloj de instrucciones del sistema (4 MHz)
    
    // SAMC = 31: Configuramos el máximo tiempo de muestreo automático (31 * TAD).
    // Esto le da al circuito un tiempo sumamente generoso y robusto para cargar el capacitor interno.
    AD1CON3bits.SAMC = 31;  
    
    // ADCS = 7: Divide el reloj del sistema para generar el reloj interno del ADC (TAD).
    // Con FCY = 4 MHz, un TAD = TCY * (ADCS + 1) -> 250ns * 8 = 2.0 microsegundos.
    // Cumple perfectamente con los requerimientos eléctricos mínimos del PIC24F.
    AD1CON3bits.ADCS = 7;   
    
    // Configurar multiplexor analógico
    AD1CHSbits.CH0SA = 3;   // Seleccionar canal positivo AN3
    AD1CHSbits.CH0NA = 0;   // Seleccionar canal negativo a masa (AVSS)
    
    AD1CON1bits.ADON = 1;   // Encender oficialmente el módulo ADC
}

uint16_t ADC_ReadAN3(void) {
    // Iniciar la secuencia automática de muestreo
    AD1CON1bits.SAMP = 1;   
    
    // El hardware mantendrá el muestreo durante 31 TADs, luego bajará automáticamente 
    // el bit SAMP a '0' e iniciará la conversión de datos. Esperamos a que el bit DONE sea 1.
    while (!AD1CON1bits.DONE); 
    
    return ADC1BUF0;        // Retornar el valor de 12 bits medido
}

void Display_8Bit(uint8_t value) {
    // Enmascarar bit por bit hacia tus pines asignados
    LATCbits.LATC1 = (value >> 0) & 0x01; // Bit 0 (LSB)
    LATCbits.LATC2 = (value >> 1) & 0x01; // Bit 1
    LATCbits.LATC3 = (value >> 2) & 0x01; // Bit 2
    LATCbits.LATC4 = (value >> 3) & 0x01; // Bit 3
    LATEbits.LATE5 = (value >> 4) & 0x01; // Bit 4
    LATEbits.LATE7 = (value >> 5) & 0x01; // Bit 5
    LATGbits.LATG6 = (value >> 6) & 0x01; // Bit 6
    LATGbits.LATG15 = (value >> 7) & 0x01; // Bit 7 (MSB)
}

void config_oled(void)
{
    // Asegurar que el reloj corra a 8 MHz exactos sin divisiones
    CLKDIVbits.RCDIV = 0; 
    
    // Desactivar módulos analógicos y periféricos redundantes
    ANSELG = 0x0000;      
    CM2CONbits.CON = 0;   
    LCDCONbits.LCDEN = 0; 

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
}

int main(void) {
    uint16_t raw_adc = 0;
    uint8_t output_8bit = 0;
    
    // Inicializar hardware
    Pins_Output_Init();
    ADC_Init();    
    config_oled();
    
    while(1) {
        // 1. Capturar la conversión analógica de AN3 (0 - 4095)
        raw_adc = ADC_ReadAN3();
        char buffer_texto[10];
        // 2. Desplazar 4 posiciones a la derecha para truncar los 12 bits a 8 bits (0 - 255)
        output_8bit = (uint8_t)(raw_adc >> 4);
        sprintf(buffer_texto, "%u", output_8bit);
        // 3. Escribir en los LEDs en base al resultado real
        Display_8Bit(output_8bit);
        
        // Retardo estabilizador de visualización utilizando la nueva macro calculada a 4MHz
        OLED_Clear_To_Black(); 
        OLED_Set_Cursor(1, 10);
        OLED_Print_String("Valor ADC");
        OLED_Set_Cursor(5, 30);
        OLED_Print_String(buffer_texto);
        
        __delay_ms(100); 
    }
    
    return 0;
}

