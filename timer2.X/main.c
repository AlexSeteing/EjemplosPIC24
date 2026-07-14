#include <xc.h>
#include <stdint.h>

// Bits de Configuración del Sistema
#pragma config FNOSC = FRCPLL   // Oscilador: FRC con PLL (32 MHz)
//#pragma config SOSCSEL = WUT    // Oscilador secundario en modo bajo consumo
#pragma config POSCMD = NONE    // Oscilador primario deshabilitado
#pragma config FWDTEN = 0    // Perro guardián apagado
#pragma config JTAGEN = 0     // Desactivar JTAG para liberar pines

void Timer2_16Bit_Init(void) {
    T2CONbits.TON = 0;      // 1. Detener el Timer2 para configurarlo de forma segura
    T2CONbits.T32 = 0;      // 2. Operar en modo independiente de 16 bits (No cascada con Timer3)
    T2CONbits.TCS = 0;      // 3. Fuente de reloj: Interna (Fcy = 16 MHz)
    T2CONbits.TGATE = 0;    // 4. Deshabilitar el modo Gate (acumulación por pin externo)
    
    // 5. Configurar Pre-escalador a 1:64 (Bits TCKPS <1:0> = 10)
    T2CONbits.TCKPS = 0b10; 
    
    TMR2 = 0;               // 6. Inicializar el contador en cero
    PR2 = 24999;            // 7. Cargar el valor de periodo para 100 ms
    
    // 8. Preparación de la bandera de hardware (Sin habilitar la interrupción IEC)
    IFS0bits.T2IF = 0;      // Asegurar que la bandera comience limpia
    
    T2CONbits.TON = 1;      // 9. Arrancar el conteo del Timer2
}

int main(void) {
    // FORZAR RECOPE DE RELOJ: Elimina cualquier división del oscilador interno (FRC)
    // Esto asegura que si usas FRCPLL, el sistema funcione exactamente a 32 MHz (16 MIPS)
    CLKDIVbits.RCDIV = 0b000; // División 1:1 (Máxima velocidad)
    
    // Configurar el Pin RC2 como Salida Digital para el LED
    //ANSCEbits.ANCE2 = 0;    
    TRISCbits.TRISC2 = 0;   
    LATCbits.LATC2 = 0;     

    // Inicializar el Timer2
    Timer2_16Bit_Init();

    while (1) {
        if (IFS0bits.T2IF == 1) {
            LATCbits.LATC2 = ~LATCbits.LATC2; 
            IFS0bits.T2IF = 0; 
        }
    }
    return 0;
}