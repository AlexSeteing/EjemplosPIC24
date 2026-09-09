#ifndef PANTALLA_H
#define PANTALLA_H

#include <stdint.h>
#include "xc.h"

//#ifndef FCY
//#define FCY 16000000UL 
//#endif

#include <stdio.h>
#include <libpic30.h>

// ==========================================
// ESTRUCTURA Y VARIABLES DEL RTC
// ==========================================

typedef struct {
    uint16_t anio;
    uint16_t mes;
    uint16_t dia;
    uint16_t hora;
    uint16_t min;
    uint16_t sec;
} RTC_Dacai_t;

extern volatile uint8_t pagina_actual;

// Variable global extern (definida en main.c)
extern volatile RTC_Dacai_t fecha_hora_actual;

extern volatile RTC_Dacai_t fecha_hora_encendido;
extern volatile uint8_t flag_encendido_registrado;

// Prototipos necesarios declarados en otros módulos (.c / .h)
void Check_UART1_Errors(void);
void UART1_Write(char TxByte);
void Imprimir_Registro_Encendido(void);

// ==========================================
// FUNCIONES INLINE PARA PANTALLA DACAI
// ==========================================

static inline uint8_t BCD_To_Int(uint8_t bcd_val) {
    return ((bcd_val >> 4) * 10) + (bcd_val & 0x0F);
}

static inline void DACAI_Request_RTC(void) {
    Check_UART1_Errors();
    
    UART1_Write(0xEE);  // Header
    UART1_Write(0x82);  // Comando para solicitar RTC
    
    // Tail
    UART1_Write(0xFF);
    UART1_Write(0xFC);
    UART1_Write(0xFF);
    UART1_Write(0xFF);
}

static inline void DACAI_Show_Screen(uint16_t screen_id) 
{
    //DACAI_Wait_Not_Busy();
    Check_UART1_Errors(); 
    
    UART1_Write(0xEE);  
    UART1_Write(0xB1);  
    UART1_Write(0x00);  
    
    UART1_Write((uint8_t)(screen_id >> 8));   
    UART1_Write((uint8_t)(screen_id & 0xFF));  
    
    UART1_Write(0xFF);  
    UART1_Write(0xFC);  
    UART1_Write(0xFF);  
    UART1_Write(0xFF);  
}

static inline void DACAI_Buzzer(void) 
{
    //DACAI_Wait_Not_Busy();
    Check_UART1_Errors(); 
    
    UART1_Write(0xEE);  
    UART1_Write(0x61);  
    UART1_Write(0x0A);  
    UART1_Write(0xFF);  
    UART1_Write(0xFC);  
    UART1_Write(0xFF);  
    UART1_Write(0xFF);  
}

static inline void DACAI_Set_Slider_Value(uint16_t screen_id, uint16_t control_id, uint32_t value) 
{
    //DACAI_Wait_Not_Busy();
    Check_UART1_Errors(); 
    
    UART1_Write(0xEE);  
    UART1_Write(0xB1);  
    UART1_Write(0x10);  
    
    UART1_Write((uint8_t)(screen_id >> 8));   
    UART1_Write((uint8_t)(screen_id & 0xFF));  
    
    UART1_Write((uint8_t)(control_id >> 8));  
    UART1_Write((uint8_t)(control_id & 0xFF)); 
    
    UART1_Write((uint8_t)(value >> 24)); 
    UART1_Write((uint8_t)(value >> 16)); 
    UART1_Write((uint8_t)(value >> 8));  
    UART1_Write((uint8_t)(value & 0xFF)); 
    
    UART1_Write(0xFF);  
    UART1_Write(0xFC);  
    UART1_Write(0xFF);  
    UART1_Write(0xFF);  
}

static inline void DACAI_Solicitar_Screen_ID(void) {
    //DACAI_Wait_Not_Busy();
    Check_UART1_Errors(); 
    
    UART1_Write(0xEE);  
    UART1_Write(0xB1);  
    UART1_Write(0x01);  
    
    UART1_Write(0xFF);  
    UART1_Write(0xFC);   
    UART1_Write(0xFF);   
    UART1_Write(0xFF);  
}

static inline void DACAI_Set_Text(uint16_t screenID, uint16_t controlID, const char* texto) 
{
    //DACAI_Wait_Not_Busy();
    Check_UART1_Errors(); 
    
    UART1_Write(0xEE);  
    UART1_Write(0xB1);  
    UART1_Write(0x10);  
    
    UART1_Write((uint8_t)((screenID >> 8) & 0xFF));
    UART1_Write((uint8_t)(screenID & 0xFF));
    
    UART1_Write((uint8_t)((controlID >> 8) & 0xFF));
    UART1_Write((uint8_t)(controlID & 0xFF));
    
    while (*texto != '\0') {
        UART1_Write((uint8_t)(*texto));
        texto++;
    }
    
    UART1_Write(0xFF);
    UART1_Write(0xFC);
    UART1_Write(0xFF);
    UART1_Write(0xFF);
}

static inline void DACAI_Show_Control(uint16_t screenID, uint16_t controlID)
{
    //DACAI_Wait_Not_Busy();
    Check_UART1_Errors();

    UART1_Write(0xEE);
    UART1_Write(0xB1);
    UART1_Write(0x03);
    UART1_Write(0x00);
    UART1_Write(screenID);
    UART1_Write(0x00);
    UART1_Write(controlID);
    
    UART1_Write(0x01);
    UART1_Write(0xFF);
    UART1_Write(0xFC);
    UART1_Write(0xFF);
    UART1_Write(0xFF);
}

static inline void DACAI_Hide_Control(uint16_t screenID, uint16_t controlID)
{
    //DACAI_Wait_Not_Busy();
    Check_UART1_Errors();

    UART1_Write(0xEE);
    UART1_Write(0xB1);
    UART1_Write(0x03);
    UART1_Write(0x00);
    UART1_Write(screenID);
    UART1_Write(0x00);
    UART1_Write(controlID);
    
    UART1_Write(0x00);
    UART1_Write(0xFF);
    UART1_Write(0xFC);
    UART1_Write(0xFF);
    UART1_Write(0xFF);
}

static inline void Desempaquetar_Respuesta_RTC(volatile uint8_t *buffer) {
    // 1. Decodificación de los datos BCD provenientes de la pantalla Dacai
    fecha_hora_actual.anio = BCD_To_Int(buffer[2]);
    fecha_hora_actual.mes  = BCD_To_Int(buffer[3]);
    fecha_hora_actual.dia  = BCD_To_Int(buffer[5]);
    fecha_hora_actual.hora = BCD_To_Int(buffer[6]);
    fecha_hora_actual.min  = BCD_To_Int(buffer[7]);
    fecha_hora_actual.sec  = BCD_To_Int(buffer[8]);

    // 2. Registro e impresión en el arranque del equipo
    if (flag_encendido_registrado == 0) {
        fecha_hora_encendido = fecha_hora_actual;
        flag_encendido_registrado = 1; // Evita impresiones repetidas en arranques subsecuentes

        // Imprimir el ticket formal de encendido
        Imprimir_Registro_Encendido();
    }
}

static inline void DACAI_Limpiar_Etiquetas(uint16_t pag_inicio, uint16_t pag_fin) {
    uint16_t pag;
    // Arreglo con los IDs de los controles/etiquetas a limpiar
    const uint16_t etiquetas[] = {35, 41, 42, 43}; 
    uint8_t num_etiquetas = sizeof(etiquetas) / sizeof(etiquetas[0]);
    uint8_t i;

    // Recorre todas las páginas indicadas
    for (pag = pag_inicio; pag <= pag_fin; pag++) {
        for (i = 0; i < num_etiquetas; i++) {
            DACAI_Set_Text(pag, etiquetas[i], " ");
            __delay_ms(5); // Pequeño retardo entre transmisiones UART para no saturar
        }
    }
}

static inline void DACAI_Set_Button_Enable(uint16_t screen_id, uint16_t control_id, uint8_t estado) 
{
    // Espera si el pin BUSY está en alto (RA2) antes de transmitir
    Check_UART1_Errors();

    UART1_Write(0xEE);
    UART1_Write(0xB1);
    UART1_Write(0x04); // Comando Enable / Disable Control

    // Screen ID (16 bits)
    UART1_Write((uint8_t)(screen_id >> 8));
    UART1_Write((uint8_t)(screen_id & 0xFF));

    // Control ID (16 bits)
    UART1_Write((uint8_t)(control_id >> 8));
    UART1_Write((uint8_t)(control_id & 0xFF));

    // Estado: 0x00 = Deshabilitar / 0x01 = Habilitar
    UART1_Write(estado ? 0x01 : 0x00);

    // Fin de trama reglamentario
    UART1_Write(0xFF);
    UART1_Write(0xFC);
    UART1_Write(0xFF);
    UART1_Write(0xFF);
}

// Desactiva el botón (Bloquea el touch)
static inline void DACAI_Disable_Button(uint16_t screen_id, uint16_t control_id) {
    DACAI_Set_Button_Enable(screen_id, control_id, 0);
}

// Activa el botón (Permite el touch)
static inline void DACAI_Enable_Button(uint16_t screen_id, uint16_t control_id) {
    DACAI_Set_Button_Enable(screen_id, control_id, 1);
}

#endif // PANTALLA_H