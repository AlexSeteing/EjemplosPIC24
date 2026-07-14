#include <xc.h>
#include <stdint.h>

// Bits de Configuración del Sistema
#pragma config FNOSC = FRCPLL   // Oscilador: FRC con PLL (32 MHz)
#pragma config POSCMD = NONE    // Oscilador primario deshabilitado
#pragma config FWDTEN = 0       // Perro guardián apagado
#pragma config JTAGEN = 0       // Desactivar JTAG para liberar pines

#define FOSC 32000000UL
#define FCY (FOSC / 2) 

#include <libpic30.h>

void PWM_RG15_Init(void) {
    // ==========================================
    // PASO 1: Liberar el Pin RG15 y Forzar Salida
    // ==========================================
    LCDCONbits.LCDEN = 0;       // Apagar el controlador LCD para liberar el pin RG15 (SEG10)
    TRISGbits.TRISG6 = 0;      // Configurar RG6 como salida digital
    LATGbits.LATG6 = 0;        // Inicializar el pin en nivel bajo

    // ==========================================
    // PASO 2: Mapeo de Pines con PPS (RP30 -> OC1)
    // ==========================================
    __builtin_write_OSCCONL(OSCCON & 0xBF); // Abrir candado de seguridad PPS
    RPOR10bits.RP21R = 18;                  // Asignar la función 18 (OC1) al pin RP21/RG6
    __builtin_write_OSCCONL(OSCCON | 0x40); // Cerrar candado de seguridad PPS

    // ==========================================
    // PASO 3: Configurar la Base de Tiempo (Timer2)
    // ==========================================
    T2CONbits.TON = 0;       // Detener el Timer2 para configurarlo
    T2CONbits.T32 = 0;       // Operar en modo independiente de 16 bits
    T2CONbits.TCS = 0;       // Fuente de reloj: Interna (Fcy = 16 MHz)
    T2CONbits.TGATE = 0;     // Deshabilitar modo Gate
    T2CONbits.TCKPS = 0b10;  // Configurar Pre-escalador a 1:64
    TMR2 = 0;                // Inicializar el contador en cero
    
    // Cálculo para Frecuencia PWM = 1 kHz con Fcy = 16 MHz y Prescaler 1:64
    // PR2 = (16,000,000 / (1,000 * 64)) - 1 = 250 - 1 = 249
    PR2 = 249;               

    // ==========================================
    // PASO 4: Configurar el Módulo Output Compare 1 (OC1)
    // ==========================================
    OC1CON1bits.OCM = 0b000;     // Asegurar que el módulo esté apagado durante la config.
    
    // Configurar ciclos de trabajo iniciales (Ejemplo: 50% de Duty Cycle)
    // Si PR2 es 249, el 50% es aproximadamente 125
    OC1R = 5;                  
    OC1RS = 5;                 // Registro buffer sombra

    OC1CON1bits.OCTSEL = 0b000;  // Seleccionar Timer2 como la base de tiempos para OC1
    
    // IMPORTANTE: Modo libre (Free-Running) para evitar trabas de hardware por trigger externo
    OC1CON2bits.SYNCSEL = 0x1F;  

    // ==========================================
    // PASO 5: Encender el Sistema en Orden Coherente
    // ==========================================
    OC1CON1bits.OCM = 0b110;     // Activar OC1 en modo PWM estándar (Edge-aligned)
    T2CONbits.TON = 1;           // Arrancar el conteo del Timer2
}

int main(void) {
    // Configuración del reloj del sistema (Sin división para operar a 32 MHz)
    CLKDIVbits.RCDIV = 0b000; 
    
    // Inicializar el periférico PWM en el pin RG15
    PWM_RG15_Init();

    // Bucle principal: Modificación dinámica del Duty Cycle a modo de prueba
    while (1) {
        // Ejemplo para cambiar el brillo a lo largo del tiempo de manera manual si se desea,
        // o puedes dejarlo fijo. Por ahora, el hardware mantendrá de forma autónoma
        // el pin RG15 oscilando a 1 kHz con un 50% de ciclo de trabajo.
        
         // Descomenta este bloque si quieres ver el LED "respirar" (atenuarse y brillar)
        uint16_t i;
        for(i = 0; i < 250; i++) {
            OC1RS = i; // Cambiar el ancho del pulso dinámicamente
            __delay_ms(5); 
        }
    }
    return 0;
}