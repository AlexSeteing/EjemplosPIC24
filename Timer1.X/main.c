#include <xc.h>
#include <stdint.h>

#pragma config FNOSC = FRCPLL   // Oscilador Inicial: FRC con PLL activo
//#pragma config SOSCSEL = WUT    // Tipo de Oscilador Secundario (Bajo consumo)
#pragma config POSCMD = NONE    // Oscilador Principal Desactivado
#pragma config FWDTEN = 0     // Perro Guardián (Watchdog) Apagado
#pragma config JTAGEN = 0     // Puerto JTAG Desactivado (Libera Pines)

void Timer1_Init(void) {
    T1CONbits.TON = 0;      // 1. Detener el Timer1 antes de configurar
    T1CONbits.TCS = 0;      // 2. Fuente de reloj: Interna (Fcy = 16 MHz)
    T1CONbits.TSIDL = 0;    // Continúa operando en modo Idle
    T1CONbits.TGATE = 0;    // Desactivar acumulación por compuerta externa
    
    // 3. Configurar Pre-escalador a 1:256 (Bits TCKPS <1:0> = 11)
    T1CONbits.TCKPS = 0b11; 
    
    TMR1 = 0;               // 4. Limpiar el contador de 16 bits
    PR1 = 31249;            // 5. Cargar el periodo calculado para 500 ms
    
    // 6. Configuración del Controlador de Interrupciones
    IFS0bits.T1IF = 0;      // Limpiar bandera de interrupción del Timer1
    IPC0bits.T1IP = 4;      // Establecer prioridad de interrupción (4 = media/default)
    IEC0bits.T1IE = 1;      // Habilitar la interrupción del Timer1
    
    T1CONbits.TON = 1;      // 7. Encender el Timer1
}

int main(void) 
{
    // Configuración Inicial de Pines del Sistema
    //ANSCEbits.ANCE2 = 0;    // Desactivar funciones analógicas en el Puerto C (Pin RC2)
    TRISCbits.TRISC2 = 0;   // Configurar el Pin RC2 como Salida Digital (LED)
    LATCbits.LATC2 = 0;     // Inicializar el LED apagado

    // Inicializar el Temporizador
    Timer1_Init();

    // Bucle Principal Vacío 
    // La CPU se queda libre ya que el hardware y la interrupción manejan el LED.
    while (1) 
    {
    }
    
    return 0;
}

// =============================================================================
// Vector de Interrupción (ISR) del Timer1
// =============================================================================
void __attribute__((__interrupt__, __auto_psv__)) _T1Interrupt(void) {
    LATCbits.LATC2 = ~LATCbits.LATC2; // Conmutar/Invertir el estado del LED en RC2
    IFS0bits.T1IF = 0;                // Limpiar la bandera por software
}