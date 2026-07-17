#define NUM_MUESTRAS 16     // Tamaño del filtro promedio móvi
#define ADC_MAX_CALIBRADO_MPX   4095
#define PRESION_CONSIGNA_PSI     24000  // 24.000 psi
#define HISTERESIS_PSI           500    // 0.500 psi (Evita el golpeteo del contactor)

#define UMBRAL_APAGADO_PSI       (PRESION_CONSIGNA_PSI + HISTERESIS_PSI) // 24500 (24.5 psi)
#define UMBRAL_ENCENDIDO_PSI     (PRESION_CONSIGNA_PSI - HISTERESIS_PSI) // 23500 (23.5 psi)

// Estructura para el filtrado digital
typedef struct {
    uint16_t historial[NUM_MUESTRAS];
    uint8_t indice;
    uint32_t suma;
} Sensor_Filter_t;

// Variables Globales Volátiles (ISR/Main Compartido)
volatile Sensor_Filter_t f_tempCamara, f_presCamara, f_presCamisa, f_sensorExtra;
volatile uint8_t bandera100ms = 0; // Bandera de tiempo controlada por hardware

void ADC_Init(void);

void __attribute__((__interrupt__, __auto_psv__)) _T2Interrupt(void) {
    bandera100ms = 1;       // Levanta la bandera para procesar en el while(1)
    IFS0bits.T2IF = 0;      // Limpia la bandera de interrupción de hardware
    solicitar_id_pantalla == 1;
}

void ADC_Init(void) 
{
    //Configurar pines de entrada
    //Sensor 1 Pin 22 RB3
    //Sensor 2 Pin 23 RB2
    //Sensor 3 Pin 24 RB1
    //Sensor 4 Pin 25 RB0
    ANSBbits.ANSB0 = 1; 
    ANSBbits.ANSB1 = 1; 
    ANSBbits.ANSB2 = 1;
    ANSBbits.ANSB3 = 1;
    
    TRISBbits.TRISB0= 1;
    TRISBbits.TRISB1= 1;
    TRISBbits.TRISB2= 1;
    TRISBbits.TRISB3= 1;
    
    // Configuración del ADC
    AD1CON1 = 0x0000;  
    AD1CON1bits.MODE12 = 1;
    AD1CON1bits.FORM = 0;   // Formato entero absoluto derecho
    
    // SSRC = 7 (111): El temporizador interno del ADC termina de manera automática el 
    // tiempo de muestreo (Sample) y arranca la conversión de forma autónoma y precisa.
    AD1CON1bits.SSRC = 7; 
 
    AD1CON2 = 0x0000;       // Referencias estándar: VREF+ = AVDD (3.3V), VREF- = AVSS (GND)
    AD1CON3bits.ADRC = 0;   // Usar el reloj de instrucciones del sistem
    
    // SAMC = 31: Configuramos el máximo tiempo de muestreo automático (31 * TAD).
    // Esto le da al circuito un tiempo sumamente generoso y robusto para cargar el capacitor interno.
    AD1CON3bits.SAMC = 31;
    AD1CON3bits.ADCS = 7;   
    
    AD1CSSL = 0x0000;       // Desactivamos el escaneo automático (lo haremos por software para control total)
    AD1CON1bits.ADON = 1;   // ¡Encender el módulo ADC de forma global
}

uint16_t ADC_Read_Channel(uint8_t canal) 
{
    AD1CHS = canal;             // Selecciona el canal analógico a muestrear (AN0, AN1, AN2 o AN3)
    AD1CON1bits.SAMP = 1;       // Inicia el tiempo de muestreo automático
    while (!AD1CON1bits.DONE);  // El hardware borra DONE cuando la conversión de 12 bits finaliza
    return ADC1BUF0;             // Retorna el resultado del buffer   
}

uint16_t Filtrar_Sensor(volatile Sensor_Filter_t *f, uint16_t nueva_lectura) 
{
    // 1. Restar la muestra más antigua que va a ser reemplazada de la suma total
    f->suma -= f->historial[f->indice];
    
    // 2. Almacenar la nueva lectura en la posición actual del búfer
    f->historial[f->indice] = nueva_lectura;
    
    // 3. Sumar el nuevo valor al total acumulado
    f->suma += nueva_lectura;
    
    // 4. Avanzar el índice del buffer circular de forma segura (0 a 15)
    f->indice = (f->indice + 1) % NUM_MUESTRAS;
    
    // 5. Retornar el promedio matemático
    return (uint16_t)(f->suma / NUM_MUESTRAS);
}

void Leer_Sensores(void) 
{
    // --- 1. LECTURA CRUDAS DEL ADC ---
    uint16_t adc_presCamara = ADC_Read_Channel(3); // AN3
    uint16_t adc_presCamisa = ADC_Read_Channel(2); // AN2
    uint16_t adc_tempCamara = ADC_Read_Channel(1); // AN1
    uint16_t adc_sensorExtra = ADC_Read_Channel(0); // AN0
    
    // --- 2. FILTRADO DIGITAL (16 MUESTRAS) ---
    uint16_t filtrado_presCamara = Filtrar_Sensor(&f_presCamara, adc_presCamara);
    uint16_t filtrado_presCamisa = Filtrar_Sensor(&f_presCamisa, adc_presCamisa);
    uint16_t filtrado_tempCamara = Filtrar_Sensor(&f_tempCamara, adc_tempCamara);
    uint16_t filtrado_sensorExtra = Filtrar_Sensor(&f_sensorExtra, adc_sensorExtra);
    
    // --- 3. APLICACIÓN DE ECUACIONES FÍSICAS EN PSI (16 BITS) ---
    
    // Presión Cámara: Convertido a PSI * 1000 (Ej: 24150 = 24.15 psi)
    sensorCamara = (uint16_t)(((uint32_t)filtrado_presCamara * 29000) / ADC_MAX_CALIBRADO_MPX);
    
    // Presión Camisa: Convertido a PSI * 1000
    sensorCamisa = (uint16_t)(((uint32_t)filtrado_presCamisa * 29000) / ADC_MAX_CALIBRADO_MPX);
    
    // Temperatura Cámara (Mantiene Celsius * 10. Ej: 1215 = 121.5 °C)
    sensorTemperatura = (uint16_t)(((uint32_t)filtrado_tempCamara * 3300) / 4095);
    
    sensorExtra = filtrado_sensorExtra; 
}

void Controlar_Presion_Camisa(void) 
{
    
    // REGLA DE SEGURIDAD ABSOLUTA:
    // Si el nivel bajo NO detecta agua (SENSOR_NIVEL_BAJO == 0),
    // apagamos las resistencias INMEDIATAMENTE por hardware, sin importar la presión.
    if (NIVEL_BAJO == 0) {
        RESISTENCIAS = 0; // Corta el MOC3020 -> Apaga contactor
        DACAI_Set_Text(pagina_actual, 42, " ");
        
        // Opcional: Aquí podrías activar una alerta UART a la Dacai
        // DACAI_Set_Text(1, ERR_ID, "ERROR: Falta Agua");
        return; // Salimos de la función de inmediato para bloquear el resto de la lógica
    }
    
    // --- LÓGICA DE CONTROL DE PRESIÓN (Solo se ejecuta si hay agua suficiente) ---
    
    // Condición 1: Si la presión cae por debajo del umbral Y hay agua encima del nivel bajo
    if (sensorCamisa <= UMBRAL_ENCENDIDO_PSI && NIVEL_BAJO == 1) {
        RESISTENCIAS = 1; // Activa contactor (Enciende resistencias)
        DACAI_Set_Text(pagina_actual, 42, "Calentando...");
    }
    
    // Condición 2: Si la presión alcanza o supera el límite máximo
    else if (sensorCamisa >= UMBRAL_APAGADO_PSI) 
    {
        RESISTENCIAS = 0; // Corta contactor (Apaga resistencias)
        DACAI_Set_Text(pagina_actual, 42, " ");
    }
}



void Timer2_Init(void) 
{
    T2CON = 0x0000;       // Detiene el Timer 2 y limpia la configuración
    T2CONbits.TCKPS = 3;  // Prescaler 1:256
    
    // Cálculo del período (PR2) para 100 ms exactos:
    // PR2 = (Tiempo deseado * Fcy) / Prescaler
    // PR2 = (0.100s * 16,000,000) / 256 = 6250
    //PR2 = 6250;    //Para 100ms
    //PR2 = 31250;  //Para 500ms
    PR2 = 62500;  //Para 1 s
    
    TMR2 = 0x0000;        
    
    IPC1bits.T2IP = 4; 
    IFS0bits.T2IF = 0; 
    IEC0bits.T2IE = 1; // Controla T3
    T2CONbits.TON = 1;
    
}