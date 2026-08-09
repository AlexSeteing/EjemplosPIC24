#ifndef CONFIG_H
#define CONFIG_H

// Configuración serial y recepción dinámica por interrupción para PIC24FJ128GA310
#include "xc.h"
#include <stdint.h>
#include "pantalla.h"

#define FOSC 32000000UL
#define FCY (FOSC / 2)           // FCY = 16,000,000 Hz

#include <libpic30.h>

// DECLARACIONES EXTERN (Sin definición directa en el .h)
extern char buffer_texto_Camara[10];
extern char buffer_texto_Camisa[10];
extern char buffer_texto_Temperatura[10];
extern char buffer_texto_Auxiliar[10];

/*Sección para la configuración de lo que se recibirá de la pantalla*/
#define BUFFER_MAX_LARGO 64
extern volatile uint8_t rxBuffer[BUFFER_MAX_LARGO];
extern volatile uint16_t bytesRecibidos;

// CORREGIDO: Declaraciones extern limpias sin inicialización (= 0)
extern volatile uint8_t solicitar_id_pantalla;
extern volatile uint8_t pagina_actual;
extern volatile uint8_t estadoLlenado;
extern volatile uint8_t sensorNivelAguaOK;
extern volatile uint8_t sensorCamisaOK;

#define BOMBA_AGUA          LATGbits.LATG15
#define RESISTENCIAS        LATEbits.LATE5
#define VAL_ENTRADA         LATEbits.LATE7
#define VAL_ESC_RAPIDO      LATCbits.LATC1
#define VAL_ESC_LENTO       LATCbits.LATC2
#define VAL_SECADO          LATCbits.LATC3
#define VAL_ENTRADA_AIRE    LATCbits.LATC4
#define BUZZER              LATAbits.LATA7

#define PUERTA              PORTGbits.RG7 // Switch RG7 controla LED RG15
#define NIVEL_ALTO          PORTGbits.RG8 // Switch RG8 controla LED RE5
#define NIVEL_BAJO          PORTGbits.RG9 // Switch RG9 controla LED RE7

// ==========================================
// CONTROL IDs PANTALLA MANUAL (VisualTFT)
// ==========================================
#define BTN_MAN_INICIO         0x12
#define BTN_MAN_ENTRADA_VAPOR  0x13
#define BTN_MAN_ESCAPE_RAPIDO  0x14
#define BTN_MAN_ESCAPE_LENTO   0x15
#define BTN_MAN_SECADO         0x16
#define BTN_MAN_ENTRADA_AIRE   0x27
#define BTN_MAN_FIN            0x28

void PINES_Init(void);

typedef enum {
    ESTADO_INIT = 0,     // Estado inicial al encender el equipo
    ESTADO_LLENANDO,     // La bomba está activa llenando hasta el tope
    ESTADO_ESPERA_VACIO  // El agua bajó del nivel alto, pero no iniciamos hasta que toque el fondo
} EstadoTanque_t;

void Init_Control_Nivel(void);
void Controlar_Nivel_Tanque(void);

#endif // CONFIG_H