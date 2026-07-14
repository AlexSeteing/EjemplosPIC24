#include "system_clock.h"

unsigned long actual_fcy = 16000000UL;//Inicializamos por defecto

void System_Clock_Init(pic24_osc_t speed)
{
    switch(speed)
    {
        // =====================================================================
        // FUENTES INTERNAS (FRC)
        // =====================================================================
        case CLK_INT_32MHZ_PLL:
            __builtin_write_OSCCONH(0x01); // Fuente: FRC con PLL
            CLKDIVbits.RCDIV = 0;          // 8 MHz
            CLKDIVbits.CPDIV = 0;          // Salida PLL activa (32 MHz)
            actual_fcy = 16000000UL;       // 16 MIPS
            while(OSCCONbits.LOCK == 0);   // Esperar candado del PLL
            break;
    }
}