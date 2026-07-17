// Configuración serial y recepción dinámica por interrupción para PIC24FJ128GA310
#include "xc.h"
#include <stdint.h>


// =============================================================================
// CONFIGURACIÓN DEL RELOJ (FRC CON PLL CORRIENDO A 32 MHz)
// =============================================================================
#pragma config POSCMD = NONE     // Oscilador primario deshabilitado
#pragma config FNOSC = FRCPLL    // Activa el FRC con multiplicador PLL
#pragma config OSCIOFCN = OFF   // CLKO habilitado
#pragma config FCKSM = CSDCMD    // Cambio de reloj desactivado
#pragma config SOSCSEL = 0       // SOSC en OFF deshabilitado

#pragma config FWDTEN = 0        // Watchdog Timer desactivado
#pragma config ICS = PGx1        // Debug sobre los pines EMUC1/EMUD1
#pragma config GWRP = OFF        // Protección contra escritura desactivada
#pragma config JTAGEN = 0        // JTAG apagado para liberar pines digitales

#define FOSC 32000000UL
#define FCY (FOSC / 2)           // FCY = 16,000,000 Hz

#include <libpic30.h>

volatile uint16_t sensorCamara = 0; // AN3 - Presión Cámara
volatile uint16_t sensorCamisa = 0; // AN2 - Presión Camisa
volatile uint16_t sensorTemperatura = 0; // AN1 - Temperatura Cámara
volatile uint16_t sensorExtra = 0; // AN0 - Sensor Extra / Reserva

char buffer_texto_Camara[10];
char buffer_texto_Camisa[10];
char buffer_texto_Temperatura[10];
char buffer_texto_Auxiliar[10];

/*Sección para la configuración de lo que se recibirá de la pantalla*/
#define BUFFER_MAX_LARGO 64
extern volatile uint8_t rxBuffer[BUFFER_MAX_LARGO];
extern volatile uint16_t bytesRecibidos;

volatile uint8_t  solicitar_id_pendiente = 0; // 1 = Esperando tiempo para preguntar ID
volatile uint16_t ticks_pantalla = 0;          // Contador de interrupciones
uint16_t          pagina_actual = 0;           // Guarda la página REAL validada

volatile uint8_t solicitar_id_pantalla = 0;

#define BOMBA_AGUA          LATGbits.LATG15
#define RESISTENCIAS        LATEbits.LATE5
#define VAL_ENTRADA         LATEbits.LATE7
#define VAL_ESC_RAPIDO      LATCbits.LATC1
#define VAL_ESC_LENTO       LATCbits.LATC2
#define VAL_SECADO          LATCbits.LATC3
#define VAL_ENTRADA_AIRE    LATCbits.LATC4
#define BUZZER              LATGbits.LATG6

#define PUERTA              PORTGbits.RG7 // Switch RG7 controla LED RG15
#define NIVEL_ALTO          PORTGbits.RG8 // Switch RG8 controla LED RE5
#define NIVEL_BAJO          PORTGbits.RG9 // Switch RG9 controla LED RE7


void PINES_Init(void) 
{
    //LED1 RG15
    //LED2 RE5
    //LED3 RE7
    //LED4 RC1
    //LED5 RC2
    //LED6 RC3
    //LED7 RC4
    //LED8 RG6
    
    ANSEbits.ANSE5 = 0;
    ANSEbits.ANSE7 = 0;
    ANSCbits.ANSC4 = 0;
    ANSGbits.ANSG6 = 0;
    
    TRISGbits.TRISG15 = 0;
    TRISEbits.TRISE5 = 0;
    TRISEbits.TRISE7 = 0;
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    TRISCbits.TRISC3 = 0;
    TRISCbits.TRISC4 = 0;
    TRISGbits.TRISG6 = 0;
    
    BOMBA_AGUA = 0;
    RESISTENCIAS = 0;
    VAL_ENTRADA = 0;
    VAL_ESC_RAPIDO = 0;
    VAL_ESC_LENTO = 0;
    VAL_SECADO = 0;
    VAL_ENTRADA_AIRE = 0;
    BUZZER = 0;
    
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
}

typedef enum {
    ESTADO_INIT = 0,     // Estado inicial al encender el equipo
    ESTADO_LLENANDO,     // La bomba está activa llenando hasta el tope
    ESTADO_ESPERA_VACIO  // El agua bajó del nivel alto, pero no iniciamos hasta que toque el fondo
} EstadoTanque_t;

EstadoTanque_t estadoLlenado = ESTADO_INIT;

void Init_Control_Nivel(void) 
{
    // Configurar bomba como salida y asegurar que inicie apagada
    BOMBA_AGUA = 0;
    estadoLlenado = ESTADO_INIT;
}

void Controlar_Nivel_Tanque(void) 
{
    // Variable estática para detectar el momento exacto en que se apaga el nivel alto
    static uint8_t avisoNivelAltoDesactivado = 0; 

    switch (estadoLlenado) 
    {
        case ESTADO_INIT:
            // "Al iniciar se tienen que revisar los dos y llenar a tope"
            if (NIVEL_ALTO == 1 && NIVEL_BAJO == 1) 
            {
                BOMBA_AGUA = 0;
                estadoLlenado = ESTADO_ESPERA_VACIO;
                avisoNivelAltoDesactivado = 0; // Reseteamos la bandera
                //DACAI_Set_Text(pagina_actual, 41, "Lleno");
            } else 
            {
                BOMBA_AGUA = 1;
                estadoLlenado = ESTADO_LLENANDO;
                DACAI_Set_Text(pagina_actual, 41, "Llenando...");
            }
            break;

        case ESTADO_LLENANDO:
            BOMBA_AGUA = 1; 
            
            if (NIVEL_ALTO == 1) {
                BOMBA_AGUA = 0;
                estadoLlenado = ESTADO_ESPERA_VACIO;
                avisoNivelAltoDesactivado = 0; // Tanque lleno, reiniciamos detector
                DACAI_Set_Text(pagina_actual, 41, "Lleno");
            }
            break;

        case ESTADO_ESPERA_VACIO:
            // La bomba permanece estrictamente APAGADA mientras baja el agua
            BOMBA_AGUA = 0; 
            
            // NUEVO: Si el nivel alto se desactiva pero todavía hay agua en el nivel bajo
            if (NIVEL_ALTO == 0 && NIVEL_BAJO == 1) {
                if (avisoNivelAltoDesactivado == 0) {
                    DACAI_Set_Text(pagina_actual, 41, "Nivel Alto OFF");
                    avisoNivelAltoDesactivado = 1; // Bloqueamos el envío para que solo lo mande una vez
                }
            }
            
            // Si el nivel bajo se queda finalmente sin agua, mandamos a llenar a tope
            if (NIVEL_BAJO == 0) {
                BOMBA_AGUA = 1;
                estadoLlenado = ESTADO_LLENANDO;
                DACAI_Set_Text(pagina_actual, 41, "Llenando...");
            }
            break;

        default:
            BOMBA_AGUA = 0; 
            estadoLlenado = ESTADO_INIT;
            break;
    }
}

