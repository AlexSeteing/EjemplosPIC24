#ifndef CICLOS_H
#define CICLOS_H

#include <stdint.h>



typedef enum {
    ESTADO_REPOSO = 0,
    ESTADO_PREVACIO,
    ESTADO_CALENTAMIENTO,
    ESTADO_ESTERILIZACION,
    ESTADO_DESPRESURIZACION,
    ESTADO_SECADO,
    ESTADO_IGUALACION_AIRE,
    ESTADO_DES_VACIO,
    ESTADO_FIN_CICLO,
    ESTADO_ALARMA,
    ESTADO_MANUAL
} EstadoAutoclave_t;

// 2. Definición de Parámetros de un Ciclo
typedef struct {
    const char* nombre;
    uint8_t  num_prevacios;
    int16_t  temp_objetivo;       // °C * 10 (Ej: 1210 = 121.0 °C)
    int16_t  presion_esterilizado;// PSI * 100 (Ej: 1500 = 15.00 PSI)
    uint16_t tiempo_esterilizado; // En segundos
    uint16_t tiempo_secado;       // En segundos
} ParametrosCiclo_t;

// 3. Modos de Selección (Los 5 programas + Manual)
typedef enum {
    PROGRAMA_ROPA         = 0,  // Índice 0 en TABLA_PROGRAMAS
    PROGRAMA_INSTRUMENTAL = 1,  // Índice 1
    PROGRAMA_BOWIE_DICK   = 2,  // Índice 2
    PROGRAMA_LIQUIDOS     = 3,  // Índice 3
    PROGRAMA_ESPECIAL     = 4,  // Índice 4
    PROGRAMA_MANUAL       = 5   // Fuera de la tabla
} TipoPrograma_t;

// Estructura de Control Global del Sistema
typedef struct {
    EstadoAutoclave_t estado_actual;
    TipoPrograma_t    programa_seleccionado;
    ParametrosCiclo_t parametros_activos;
    uint32_t          temporizador_etapa_seg; // Contador regresivo de tiempo
    uint8_t           contador_prevacios;
    uint8_t           subestado_prevacio;
    uint8_t           flag_iniciar;
    uint8_t           flag_cancelar;
    uint8_t           flag_vacio_alcanzado;
    uint8_t           flag_fin_notificado;
    uint16_t          timer_mensaje_ms;
} ControlAutoclave_t;

typedef enum {
    MANUAL_INICIO = 0,
    MANUAL_ENTRADA_VAPOR,
    MANUAL_ESCAPE_RAPIDO,
    MANUAL_ESCAPE_LENTO,
    MANUAL_SECADO,
    MANUAL_ENTRADA_AIRE,
    MANUAL_FIN
} EstadoManual_t;

extern ControlAutoclave_t autoclave;
extern volatile uint8_t bandera1seg;

// Prototipos de funciones
void Seleccionar_Programa(TipoPrograma_t prog);
void Procesar_Maquina_Estados(void);
void Ejecutar_Fase_Manual(EstadoManual_t nueva_fase);
void Procesar_Boton_Manual(uint8_t control_id);
void UART2_Write_Text(const char* text);

#endif // CICLOS_H