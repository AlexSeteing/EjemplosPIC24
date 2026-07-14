// Interrupciones externas - INT0 en RF6

#include <xc.h>

// Configuraciones del reloj
#pragma config FNOSC = FRC        // Oscilador Interno Rápido (FRC a 8MHz)
#pragma config POSCMD = XT        // Modo XT (Cristal externo si se requiere, aunque FNOSC usa FRC)
#pragma config OSCIOFCN = OFF     // CLKO deshabilitado
#pragma config FCKSM = CSDCMD     // Cambio de reloj desactivado

// Otros ajustes de seguridad
#pragma config FWDTEN = 0         // Watchdog Timer desactivado
#pragma config ICS = PGx1         // Debug sobre los pines EMUC1/EMUD1
#pragma config GWRP = OFF         // Protección contra escritura desactivada

// Definición para cálculos de tiempo (8MHz / 2 = 4 MIPS)
#define FOSC 8000000UL
#define FCY (FOSC / 2) 

#include <libpic30.h>
#include <stdio.h>

// --- Rutina de Servicio de Interrupción para INT0 ---
void __attribute__((interrupt, no_auto_psv)) _INT0Interrupt(void) {
    
    // Cambiamos el estado de RG15 (Toggle)
    LATGbits.LATG15 = ~LATGbits.LATG15; 
    
    // CORRECCIÓN 1: Limpiar la bandera correcta correspondiente a INT0
    IFS0bits.INT0IF = 0; 
}

void Configurar_Interrupcion_INT0_RG15(void) {
    // CORRECCIÓN 2: Desactivar la función analógica (AN11) en el puerto F
    // 0 = Configurado como pin digital / 1 = Configurado como pin analógico
    //ANSELFbits.ANSF6 = 0;      // Asegurar que RF6 sea completamente DIGITAL

    // 1. Configurar el pin físico 55 (RF6/INT0) como entrada digital
    TRISFbits.TRISF6 = 1;      // RF6 = Entrada
    
    // 2. Configurar el pin RG15 como salida digital
    TRISGbits.TRISG15 = 0;     // RG15 = Salida
    LATGbits.LATG15 = 0;       // Inicializamos la salida en BAJO (0v)

    // 3. Configurar los parámetros de la interrupción fija INT0
    INTCON2bits.INT0EP = 1;    // 1 = Disparo por flanco de bajada (Falling edge)
    IPC0bits.INT0IP = 5;       // Asignamos una prioridad alta (5)
    
    IFS0bits.INT0IF = 0;       // Limpiamos la bandera de INT0 por seguridad
    IEC0bits.INT0IE = 1;       // Habilitamos la interrupción externa INT0
}

int main(void) 
{
    Configurar_Interrupcion_INT0_RG15(); // Activamos la configuración
    
    while(1) 
    {
        // El bucle se queda libre para tus otros procesos (como el ADC u OLED)
    }
    return 0;
}