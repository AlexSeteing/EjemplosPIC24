#ifndef ADC_H
#define ADC_H

#include <xc.h>
#include <stdint.h>
#include "pantalla.h" // Necesario para la función DACAI_Set_Text

// =============================================================================
// DEFINICIONES Y UMBRALES DE CONTROL
// =============================================================================
#define NUM_MUESTRAS            16    // Tamaño del filtro promedio móvil
#define ADC_MAX_CALIBRADO_MPX   4095
#define PRESION_CONSIGNA_PSI    24    // 24.000 psi
#define HISTERESIS_PSI          3     // Histeresis de control

#define UMBRAL_APAGADO_PSI      (PRESION_CONSIGNA_PSI + HISTERESIS_PSI) // 27 PSI
#define UMBRAL_ENCENDIDO_PSI    (PRESION_CONSIGNA_PSI - HISTERESIS_PSI) // 21 PSI
#define PRESION_MINIMA_INICIO_PSI 24

// =============================================================================
// ESTRUCTURAS Y VARIABLES GLOBALES
// =============================================================================
typedef struct {
    uint16_t historial[NUM_MUESTRAS];
    uint8_t indice;
    uint32_t suma;
} Sensor_Filter_t;

extern volatile int16_t  sensorCamara;
extern volatile int16_t  sensorCamisa;
extern volatile uint16_t sensorTemperatura;
extern volatile uint16_t sensorExtra;

extern volatile uint8_t pagina_actual;
extern volatile uint8_t solicitar_id_pantalla;
extern volatile uint8_t sensorNivelAguaOK;
extern volatile uint8_t sensorCamisaOK;

// Variables locales de filtrado e interrupción
volatile Sensor_Filter_t f_tempCamara, f_presCamara, f_presCamisa, f_sensorExtra;

volatile uint8_t bandera100ms = 0;
volatile uint8_t bandera1seg = 0;
static uint8_t contador_100ms = 0;


// =============================================================================
// PROTOTIPOS DE FUNCIONES
// =============================================================================
void ADC_Init(void);
uint16_t ADC_Read_Channel(uint8_t canal);
uint16_t Filtrar_Sensor(volatile Sensor_Filter_t *f, uint16_t nueva_lectura);
void Leer_Sensores(void);
void Controlar_Presion_Camisa(void);
void Timer2_Init(void);

// =============================================================================
// INTERRUPCIÓN DE TIMER 2 (Temporización Base)
// =============================================================================
void __attribute__((interrupt, no_auto_psv)) _T2Interrupt(void)
{
    IFS0bits.T2IF = 0; // Limpiar bandera de interrupción de Timer2

    bandera100ms = 1;  // Tu bandera actual de 100ms

    // --- GENERACIÓN DE BASE DE TIEMPO DE 1 SEGUNDO ---
    contador_100ms++;
    if (contador_100ms >= 10) 
    {
        contador_100ms = 0;
        bandera1seg = 1; // Flag de 1 segundo listo
    }
}

// =============================================================================
// IMPLEMENTACIONES DE FUNCIONES ADC Y TEMPORIZADOR
// =============================================================================
void ADC_Init(void) 
{
    // Configurar pines de entrada analógica AN0, AN1, AN2, AN3
    ANSBbits.ANSB0 = 1; 
    ANSBbits.ANSB1 = 1; 
    ANSBbits.ANSB2 = 1;
    ANSBbits.ANSB3 = 1;
    
    TRISBbits.TRISB0 = 1;
    TRISBbits.TRISB1 = 1;
    TRISBbits.TRISB2 = 1;
    TRISBbits.TRISB3 = 1;
    
    // Configuración del módulo ADC1 en 12-bits
    AD1CON1 = 0x0000;  
    AD1CON1bits.MODE12 = 1; // Modo 12-bits (0 a 4095)
    AD1CON1bits.FORM = 0;   // Formato entero absoluto
    AD1CON1bits.SSRC = 7;   // Auto-conversión por timer interno
 
    AD1CON2 = 0x0000;       // VREF+ = AVDD, VREF- = AVSS
    AD1CON3bits.ADRC = 0;   // Reloj de conversión proveniente de FCY
    
    AD1CON3bits.SAMC = 31;  // Máximo tiempo de muestreo (31 TAD)
    AD1CON3bits.ADCS = 7;   
    
    AD1CSSL = 0x0000;       // Desactivar escaneo automático
    AD1CON1bits.ADON = 1;   // Encender ADC globalmente
}

uint16_t ADC_Read_Channel(uint8_t canal) 
{
    AD1CHS = canal;             // Selecciona canal (AN0 - AN3)
    AD1CON1bits.SAMP = 1;       // Inicia muestreo
    while (!AD1CON1bits.DONE);  // Espera finalización de conversión
    return ADC1BUF0;            
}

uint16_t Filtrar_Sensor(volatile Sensor_Filter_t *f, uint16_t nueva_lectura) 
{
    f->suma -= f->historial[f->indice];
    f->historial[f->indice] = nueva_lectura;
    f->suma += nueva_lectura;
    f->indice = (f->indice + 1) % NUM_MUESTRAS;
    
    return (uint16_t)(f->suma / NUM_MUESTRAS);
}

void Leer_Sensores(void) 
{
    // 1. Lecturas analógicas crudas
    uint16_t adc_presCamara  = ADC_Read_Channel(3); // AN3
    uint16_t adc_presCamisa  = ADC_Read_Channel(2); // AN2
    uint16_t adc_tempCamara  = ADC_Read_Channel(1); // AN1
    uint16_t adc_sensorExtra = ADC_Read_Channel(0); // AN0
    
    // 2. Filtrado con promedio móvil
    uint16_t filtrado_presCamara  = Filtrar_Sensor(&f_presCamara, adc_presCamara);
    uint16_t filtrado_presCamisa  = Filtrar_Sensor(&f_presCamisa, adc_presCamisa);
    uint16_t filtrado_tempCamara  = Filtrar_Sensor(&f_tempCamara, adc_tempCamara);
    uint16_t filtrado_sensorExtra = Filtrar_Sensor(&f_sensorExtra, adc_sensorExtra);
    
    // 3. Conversión de ingeniería (Mantiene el rango real positivo y negativo)
    sensorCamara      = (int16_t)(((int32_t)filtrado_presCamara * 321) / 10000 - 54);
    sensorCamisa      = (int16_t)(((int32_t)filtrado_presCamisa * 321) / 10000 - 54);
    sensorTemperatura = (int16_t)((((int32_t)filtrado_tempCamara * 330) - 204750) / 4095);
    sensorExtra       = filtrado_sensorExtra; 
}

void Controlar_Presion_Camisa(void) 
{
    static uint8_t listo_mostrado = 0;

    // --- 1. SEGURIDAD: NIVEL DE AGUA ---
    if (NIVEL_BAJO == 0) {
        RESISTENCIAS = 0; 
        DACAI_Set_Text(pagina_actual, 42, " ");
        sensorCamisaOK = 0;
        listo_mostrado = 0;
        return; 
    }

    // --- 2. DETERMINACIÓN DE UMBRALES (INICIO A 24 PSI / 32 PSI) ---
    uint16_t presion_minima_inicio; // La presión que EXIGE el sistema para iniciar
    uint16_t umbral_encendido;      // Dónde vuelve a encender la resistencia
    uint16_t umbral_apagado;        // Dónde apaga la resistencia

    if (autoclave.parametros_activos.temp_objetivo >= 132) 
    {
        // Ciclo 132°C
        presion_minima_inicio = 32; // Exige 32 PSI exactos para habilitar
        umbral_encendido      = 32; // Enciende si cae de 32 PSI
        umbral_apagado        = 34; // Apaga al subir a 34 PSI (Histéresis +2 PSI)
    } 
    else 
    {
        // Ciclo 121°C / Estándar
        presion_minima_inicio = 24; // Exige 24 PSI exactos para habilitar
        umbral_encendido      = 24; // Enciende si cae de 24 PSI
        umbral_apagado        = 26; // Apaga al subir a 26 PSI (Histéresis +2 PSI)
    }

    // --- 3. ALERTA DE PRESIÓN CRÍTICA BAJA (< 15 PSI) ---
    if (sensorCamisa < 15) {
        DACAI_Set_Text(pagina_actual, 42, "Baja Presion!");
        listo_mostrado = 0;
    }

    // --- 4. HISTÉRESIS Y MENSAJES DE PANTALLA ---

    // A. Llegó o superó la meta superior (ej. 26 PSI o 34 PSI)
    if (sensorCamisa >= umbral_apagado) 
    {
        RESISTENCIAS = 0; 
        sensorCamisaOK = 1;

        if (listo_mostrado == 0) {
            DACAI_Set_Text(pagina_actual, 42, "Listo");
            listo_mostrado = 1; 
        } else if (listo_mostrado == 1) {
            DACAI_Set_Text(pagina_actual, 42, "Listo");
            listo_mostrado = 2; // Mensaje borrado
        }
    }
    // B. Cae por debajo o igual a la presión mínima requerida (ej. <= 24 PSI)
    else if (sensorCamisa <= umbral_encendido) 
    {
        RESISTENCIAS = 1; 

        if (sensorCamisa < presion_minima_inicio) {
            sensorCamisaOK = 0; // Aún no llega a 24 PSI
            
            if (sensorCamisa >= 15) {
                DACAI_Set_Text(pagina_actual, 42, "Calentando...");
            }
            listo_mostrado = 0; 
        } 
        else {
            // Caso frontera: Está en exactamente 24 PSI
            sensorCamisaOK = 1; 
        }
    }
    // C. Zona intermedia (ej. entre 24 y 26 PSI)
    else 
    {
        sensorCamisaOK = 1; // Ya superó los 24 PSI mínimos, es seguro operar

        if (listo_mostrado == 1) {
            listo_mostrado = 2;
        }
    }
    
    if(autoclave.flag_iniciar == 1)
    {
        DACAI_Set_Text(pagina_actual, 42, " ");
    }
    
}

void Timer2_Init(void) 
{
    T2CON = 0x0000;       // Detiene Timer 2
    T2CONbits.TCKPS = 3;  // Prescaler 1:256
    
    // Configuración para 100 ms exactos (Fcy = 16 MHz):
    // PR2 = (0.100s * 16,000,000) / 256 = 6250
    PR2 = 6250; 
    
    TMR2 = 0x0000;        
    
    IPC1bits.T2IP = 4;    // Prioridad 4
    IFS0bits.T2IF = 0;    // Limpiar bandera
    IEC0bits.T2IE = 1;    // Habilitar interrupción de Timer 2
    T2CONbits.TON = 1;    // Encender Timer 2
}

void Precargar_Filtros_Iniciales(void) {
    uint8_t i;
    
    // Realiza 16 lecturas inmediatas al arrancar para llenar el buffer del filtro
    for (i = 0; i < 16; i++) {
        Leer_Sensores();
        __delay_ms(5); // Da tiempo al ADC para estabilizar lecturas
    }
}

#endif // ADC_H