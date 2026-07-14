#ifndef SYSTEM_CLOCK_H
#define	SYSTEM_CLOCK_H

#include <xc.h>

typedef enum 
{
    CLK_INT_32MHZ_PLL,   // FRC + PLL activo = 32 MHz (16 MIPS) - Ideal para UART y Pantallas
    CLK_INT_8MHZ_STD,    // FRC directo = 8 MHz (4 MIPS) - Balance consumo/velocidad
    CLK_INT_4MHZ_MED,    // FRC / 2 = 4 MHz (2 MIPS)
    CLK_INT_2MHZ_LOW,    // FRC / 4 = 2 MHz (1 MIPS)
    CLK_INT_1MHZ_X_LOW,  // FRC / 8 = 1 MHz (0.5 MIPS)
    CLK_INT_31KHZ_SAVE,   // FRC / 256 = 31.25 kHz (0.015 MIPS) - Modo hibernación extrema
            
    // ---- MODOS EXTERNOS PRIMARIOS (POSC - Cristal en pines OSCI/OSCO) 
    CLK_EXT_PRI_DIRECT,  // Cristal externo directo = Frecuencia del Cristal
    CLK_EXT_PRI_PLL,     // Cristal externo + PLL = (Frecuencia del Cristal * 4)
            
    // ---- MODOS DE ULTRA BAJO CONSUMO ----
    CLK_EXT_SOSC_32KHZ,  // Cristal de reloj externo (32.768 kHz) en pines SOSCI/SOSCO
    CLK_INT_LPRC_31KHZ   // Oscilador interno secundario de baja potencia (31 kHz)
} pic24_osc_t;

extern unsigned long actual_fcy;

void System_Clock_Init(pic24_osc_t speed);

#endif	/* SYSTEM_CLOCK_H */
