#include <xc.h>
#include <stdint.h>

// Declaramos un búfer en la RAM para almacenar las lecturas del ADC.
// El DMA escribirá directamente en esta dirección de memoria.
volatile uint16_t adc_buffer[16]; 
volatile uint8_t dma_bloque_listo = 0; // Bandera lógica para el bucle principal

void DMA_Channel0_ADC_Init(void) 
{
    // --- PASO 1 y 2: Configuración Global del DMA ---
    DMACONbits.DMAEN = 1;       // Habilitar el controlador general de DMA
    DMACONbits.PRSSEL = 0;      // Esquema de prioridad Fija (Canal 0 tiene la prioridad más alta)
    
    // Delimitamos la "jaula de protección" de la RAM (opcional pero seguro)
    DMAL = 0x0800;              // Dirección de inicio de la RAM de datos
    DMAH = 0xFFFF;              // Dirección límite superior de la RAM
    
    // --- PASOS 3 al 8: Configuración Fina del Canal 0 ---
    DMACH0bits.CHEN = 0;        // 3. Apagar el canal 0 antes de reconfigurarlo (Obligatorio)
    
    // 4. Direcciones físicas de Origen y Destino de los datos
    DMASRC0 = (uint16_t)&ADC1BUF0; // Origen: El registro donde el ADC guarda su resultado
    DMADST0 = (uint16_t)&adc_buffer; // Destino: La dirección de nuestro arreglo en la RAM
    
    // 5. Conteo de transacciones (Le pedimos 16 muestras)
    DMACNT0 = 15;               // Se escribe (N - 1), por lo tanto, 15 significa 16 datos.
    
    // 6. Tamaño del dato: Cada lectura del ADC mide 16 bits en este micro (Modo Word)
    DMACH0bits.BYTE = 0;        // 0 = Word (16 bits), 1 = Byte (8 bits)
    
    // 7. Modo de Transferencia: Repeated One-Shot 
    // (1 trigger = 1 dato. Al llenar las 16 celdas, se reinicia solo a la celda cero de forma infinita)
    DMACH0bits.TRMODE = 0b10;   // 0b10 = Modo Continuo/Repetido por pulsos
    
    // 8. Modos de Direccionamiento (REGLA DE LOS PUNTEROS)
    DMACH0bits.SAMODE = 0b00;   // Origen Fijo (No queremos que el origen avance, siempre lee ADC1BUF0)
    DMACH0bits.DAMODE = 0b01;   // Destino en Bloque con Auto-Incremento (+2 bytes por cada palabra)
                                // Esto hace que se guarden en arreglo[0], arreglo[1], arreglo[2]...
    
    // Seleccionamos quién es el motor (Trigger). El ADC1 tiene asignado el código de disparo número 13.
    DMAINT0bits.CHSEL = 47;
    //DMAREQ0bits.IRQSEL = 13;    // 13 = Evento de conversión finalizada del ADC1
    
    // --- PASO 9: Habilitación del Canal ---
    DMACH0bits.CHEN = 1;        // El Canal 0 queda armado y vigilando el bus
    
    // Configuramos la interrupción del DMA para que nos avise únicamente cuando se junten los 16 datos
    IFS0bits.DMA0IF = 0;        // Limpiar bandera de interrupción del DMA Canal 0
    IEC0bits.DMA0IE = 1;        // Habilitar interrupción del DMA Canal 0
}

void ADC_Init_Para_DMA(void) 
{
    // Configuración básica del pin AN3 (RB3) como entrada analógica
    TRISBbits.TRISB3 = 1;
    ANSBbits.ANSB3 = 1;         // Activamos la función analógica respetando el bit específico
    
    AD1CON1bits.ADON = 0;       // Apagar módulo para configuración
    AD1CON1bits.ADSIDL = 0;     // Continuar operando en modo Idle
    AD1CON1bits.MODE12 = 0;     // Modo de 10 bits de resolución (valores de 0 a 1023)
    AD1CON1bits.FORM = 0;       // Formato entero común
    
    // --- CONFIGURACIÓN CRÍTICA PARA EL DMA ---
    AD1CON1bits.SSRC = 0b011;   // El Timer3 interno disparará automáticamente la conversión del ADC
                                // Así logramos un muestreo síncrono perfecto por hardware.
    
    AD1CON1bits.ASAM = 1;       // Auto-muestreo habilitado (termina una conversión e inicia la siguiente)
    
    AD1CON2bits.PVCFG = 0;      // Referencias de voltaje: VDD y VSS
    AD1CON2bits.NVCFG0 = 0;
    AD1CON2bits.CSCNA = 0;      // No escanear múltiples canales
    
    // ¡REGLA DE ORO!: Le decimos al ADC que NO genere interrupciones hacia la CPU en cada muestra.
    AD1CON2bits.SMPI = 0b00000; // 00000 = Generar pulso de trigger en cada conversión individual.
                                // Este pulso va directo hacia el DMA, sin molestar a la CPU.
    
    AD1CHS0bits.CH0SA = 3;      // Conectar el canal positivo del ADC al pin AN3
    
    AD1CON3bits.ADRC = 0;       // Reloj derivado del reloj del sistema (Fcy)
    AD1CON3bits.ADCS = 5;       // Tiempo de conversión calibrado (Tad)
    
    // Configurar Timer3 para que marque el ritmo de muestreo del ADC (Ej: Cada 1 ms)
    T3CONbits.TON = 0;
    T3CONbits.TCKPS = 0b01;     // Prescaler 1:8
    PR3 = 2000;                 // Periodo de tiempo para el disparo
    T3CONbits.TON = 1;          // Encender Timer3
    
    AD1CON1bits.ADON = 1;       // Encender el ADC
}

// Rutina de Interrupción del DMA (Solo se ejecuta al completar las 16 muestras)
void __attribute__((__interrupt__, auto_psv)) _DMA0Interrupt(void) 
{
    dma_bloque_listo = 1;       // Le avisamos al bucle principal que el arreglo está lleno
    IFS0bits.DMA0IF = 0;        // Limpiar la bandera de interrupción por hardware
}

int main(void) 
{
    uint32_t suma;
    uint16_t promedio;
    uint8_t i;
    
    DMA_Channel0_ADC_Init();    // Inicializar el Mensajero (DMA)
    ADC_Init_Para_DMA();        // Inicializar el periférico (ADC) + Reloj (Timer3)
    
    while(1) 
    {
        if (dma_bloque_listo == 1) 
        {
            suma = 0;
            for(i = 0; i < 16; i++) 
            {
                suma += adc_buffer[i]; 
            }
            promedio = suma / 16; // Obtenemos el promedio filtrado del sensor
            
            // Aquí usarías tu "promedio" para ajustar el PWM o imprimirlo en la OLED
            
            dma_bloque_listo = 0; // Borramos la bandera para esperar el siguiente bloque
        }
    }
    
    return 0;
}
