#ifndef SERIAL_H
#define SERIAL_H

#include "xc.h"
#include <stdint.h>
#include "ciclos.h"
#include "oled_ssd1306.h"
#include <stdio.h>

// =============================================================================
// COMANDOS DE CONTROL ESC/POS (MÓDULO TÉRMICO DE EMBUTIR)
// =============================================================================
#define IMP_ESC_INIT           "\x1B\x40"      // Inicializar / Reset de la impresora
#define IMP_ALIGN_LEFT         "\x1B\x61\x00"  // Alineación a la izquierda
#define IMP_ALIGN_CENTER       "\x1B\x61\x01"  // Alineación al centro
#define IMP_ALIGN_RIGHT        "\x1B\x61\x02"  // Alineación a la derecha
#define IMP_BOLD_ON            "\x1B\x45\x01"  // Activar texto en negrita
#define IMP_BOLD_OFF           "\x1B\x45\x00"  // Desactivar texto en negrita
#define IMP_DOUBLE_ON          "\x1D\x21\x11"  // Tamaño de texto doble
#define IMP_DOUBLE_OFF         "\x1D\x21\x00"  // Tamaño de texto normal

// =============================================================================
// DEFINICIONES DE CONSTANTES
// =============================================================================
#define BUFFER_MAX_LARGO 64      // Capacidad máxima de almacenamiento
#define BYTE_HEADER      0xEE    // Inicio de comando

// --- Asignación de IDs de Páginas en VisualTFT ---
#define PAGINA_INICIO           0
#define PAGINA_ROPA             1
#define PAGINA_INSTRUMENTAL     2
#define PAGINA_BOWIE            3
#define PAGINA_LIQUIDOS         4
#define PAGINA_ESPECIAL         5
#define PAGINA_MANUAL           6

// --- Definiciones de Control IDs ---
#define BTN_CANCELAR         0x0C
#define BTN_INICIAR          0x0B
#define BTN_OK_ALARMA        0x24

#define DACAI_BUSY_PIN          PORTAbits.RA2
#define DACAI_BUSY_TRIS         TRISAbits.TRISA2

// =============================================================================
// VARIABLES GLOBALES (Con guardas y vinculación adecuada)
// =============================================================================
// Variables externas declaradas en otros módulos (ej. main.c)
extern volatile uint8_t pagina_actual;

// Variables propias del módulo serie
volatile uint8_t rxBuffer[BUFFER_MAX_LARGO]; // Búfer de datos
volatile uint16_t rxIndex = 0;               // Índice de escritura del búfer
volatile uint16_t bytesRecibidos = 0;        // Guarda la longitud final de la trama
volatile uint8_t tramaLista = 0;             // Bandera: 1 indica comando completo listo

extern volatile int16_t  sensorCamara;      // int16_t admite presiones negativas de vacío
extern volatile int16_t  sensorCamisa;      // int16_t admite presiones negativas
extern volatile uint16_t sensorTemperatura;
extern volatile uint16_t sensorExtra;

extern volatile RTC_Dacai_t fecha_hora_actual;

extern volatile RTC_Dacai_t fecha_hora_encendido;
// =============================================================================
// PROTOTIPOS DE FUNCIONES
// =============================================================================
void System_Clock_Init(void);
void UART1_Pins_Init(void);
void UART1_Init(unsigned long baudrate);
void Check_UART1_Errors(void);
void Procesar_Comando_Dacai(void);
void UART1_Write(char TxByte);
void UART1_Write_Text(const char* text);
void UART1_Write_Ln(void);
void configurar_pines_led(void);
void UART2_Init(unsigned long baudrate);
void UART2_Write(char TxByte);
void UART2_Write_Text(const char* text);
void Printer_Write_Text(const char* text);
void Printer_Reset(void);
void Printer_Set_Bold(uint8_t estado);
void Impresora_Imprimir_Lectura(const char* fase);
// Declaración/Prototipo para evitar la declaración implícita
void Imprimir_Registro_Encendido(void);
void DACAI_Wait_Not_Busy(void);


// =============================================================================
// CONTROLADOR DE INTERRUPCIÓN DE RECEPCIÓN DINÁMICA (UART1 RX)
// =============================================================================
void __attribute__((__interrupt__, __auto_psv__)) _U1RXInterrupt(void)
{
    IFS0bits.U1RXIF = 0; // Limpiar la bandera de interrupción de hardware
    
    while(U1STAbits.URXDA) // Leer mientras existan datos en el FIFO
    {
        uint8_t incomingByte = U1RXREG;
        
        // Si ya hay una trama procesándose en el main, descartamos bytes nuevos por seguridad
        if (tramaLista == 1)
        {
            continue;
        }
        
        // MÁQUINA DE ESTADOS COMPORTAMIENTO DINÁMICO
        if (rxIndex == 0)
        {
            // Estado de Sincronización: Buscando estrictamente el byte de inicio (0xEE)
            if (incomingByte == BYTE_HEADER)
            {
                rxBuffer[rxIndex++] = incomingByte;
            }
        }
        else
        {
            // Estado de Captura: Almacenar el byte actual en el búfer
            rxBuffer[rxIndex++] = incomingByte;
            
            // Protección contra desbordamiento de memoria (Buffer Overflow)
            if (rxIndex >= BUFFER_MAX_LARGO)
            {
                rxIndex = 0; // Descartar datos corruptos y reiniciar búsqueda
                continue;
            }
            
            // EVALUACIÓN CONTINUA DE LA SECUENCIA DE ESCAPE (Mínimo 5 bytes en total)
            if (rxIndex >= 5)
            {
                // Evaluamos si los últimos 4 bytes capturados corresponden al cierre: FF FC FF FF
                if (rxBuffer[rxIndex - 4] == 0xFF &&
                    rxBuffer[rxIndex - 3] == 0xFC &&
                    rxBuffer[rxIndex - 2] == 0xFF &&
                    rxBuffer[rxIndex - 1] == 0xFF)
                {
                    // ¡Secuencia de fin detectada con éxito!
                    bytesRecibidos = rxIndex; // Guardamos la longitud exacta de lo recibido
                    tramaLista = 1;           // Se avisa al ciclo principal para procesar
                }
            }
        }
    }
}

// =============================================================================
// IMPLEMENTACIÓN DE FUNCIONES DE PROCESAMIENTO Y UART
// =============================================================================
void Procesar_Comando_Dacai(void)
{
    if (rxBuffer[0] != 0xEE)
    {
        return;
    }
    
    // Trama B1: Cambio de página o evento de toque
    if (rxBuffer[1] == 0xB1)
    {
        // CASO A: Actualización de la página activa (solicitud o cambio automático)
        if (rxBuffer[2] == 0x01 && rxBuffer[3] == 0x00)
        {
            pagina_actual = rxBuffer[4];
        }
        // CASO B: Pulsación de un botón (evento touch 0x10 o 0x11)
        else if (rxBuffer[2] == 0x11 && rxBuffer[3] == 0x00)
        {
            // Reconstruimos el Control ID enviado por la Dacai (16 bits)
            uint16_t control_id = (uint16_t)rxBuffer[6];
            
            switch (control_id)
            {
                case BTN_OK_ALARMA:
                    if (autoclave.estado_actual == ESTADO_FIN_CICLO || autoclave.estado_actual == ESTADO_ALARMA) 
                    {
                        // 1. Limpieza de banderas de control
                        autoclave.flag_fin_notificado = 0;  // Permite que el buzzer/mensaje funcione en el próximo ciclo
                        autoclave.flag_vacio_alcanzado = 0; // Limpia la validación de secado
                        autoclave.subestado_prevacio = 0;   // Reinicia los prevacíos [cite: 88]

                        // 2. Transición de estado
                        autoclave.estado_actual = ESTADO_REPOSO;

                        // 3. Opcional: Actualizar texto o página en Dacai si es necesario
                        DACAI_Set_Text(pagina_actual, 35, "Listo");
                    }
                    break;

                case BTN_CANCELAR:
                    // Parada de emergencia / Cancelación en cualquier pantalla
                    autoclave.flag_cancelar = 1;
                    break;

                case BTN_INICIAR:
                    
                    if (PUERTA == 0) 
                    {
                        // La puerta está abierta: NO inicia el ciclo
                        DACAI_Buzzer(); // Alerta sonora de error
                        DACAI_Set_Text(pagina_actual, 35, "PUERTA ABIERTA");
                    }
                    
                    
                    
                    // El botón Iniciar hace cosas distintas dependiendo de la página visible
                    switch (pagina_actual)
                    {
                        case PAGINA_ROPA:
                            OLED_Print_String("Ropa");
                            Seleccionar_Programa(PROGRAMA_ROPA);
                            autoclave.flag_iniciar = 1;
                             
                            break;

                        case PAGINA_INSTRUMENTAL:
                            OLED_Print_String("Instrumental");
                            Seleccionar_Programa(PROGRAMA_INSTRUMENTAL);
                            autoclave.flag_iniciar = 1;
                            break;

                        case PAGINA_BOWIE:
                            OLED_Print_String("Bowie");
                            Seleccionar_Programa(PROGRAMA_BOWIE_DICK);
                            autoclave.flag_iniciar = 1;
                            break;

                        case PAGINA_LIQUIDOS:
                            OLED_Print_String("Liquidos");
                            Seleccionar_Programa(PROGRAMA_LIQUIDOS);
                            autoclave.flag_iniciar = 1;
                            break;

                        case PAGINA_ESPECIAL:
                            OLED_Print_String("Especial");
                            Seleccionar_Programa(PROGRAMA_ESPECIAL);
                            autoclave.flag_iniciar = 1;
                            break;

                        case PAGINA_MANUAL:
                            OLED_Print_String("Modo Manual");
                            Seleccionar_Programa(PROGRAMA_MANUAL);

                            // Entramos oficialmente al estado manual dentro de la FSM
                            autoclave.estado_actual = ESTADO_MANUAL; 
                            autoclave.flag_iniciar = 0; // En manual no usamos el arranque de ciclo automático
                            
                            break;

                        case PAGINA_INICIO:
                        default:
                            break;
                    }
                    break;
                case BTN_MAN_INICIO:
                case BTN_MAN_ENTRADA_VAPOR:
                case BTN_MAN_ESCAPE_RAPIDO:
                case BTN_MAN_ESCAPE_LENTO:
                case BTN_MAN_SECADO:
                case BTN_MAN_ENTRADA_AIRE:
                case BTN_MAN_FIN:
                    if (autoclave.estado_actual == ESTADO_MANUAL) {
                        Procesar_Boton_Manual(control_id);
                    }
                    break;
                    
                    
                default:
                    break;
            }
        }
    }
    else if(rxBuffer[1] == 0xF7)
    {
        Desempaquetar_Respuesta_RTC(rxBuffer);
    }
}

void System_Clock_Init(void) 
{
    CLKDIVbits.RCDIV = 0; // FRC completo al PLL para 32MHz
    __delay_us(20); 
}

void UART1_Pins_Init(void) 
{
    //TRISDbits.TRISD2 = 0;   // Pin 49 como TX (RP10)
    //TRISDbits.TRISD3 = 1;   // Pin 50 como RX (RP17)
    
    DACAI_BUSY_TRIS = 1; //Definimos el pin como entrada digital
    
    __builtin_write_OSCCONL(OSCCON & 0xBF); // Desbloqueo PPS
    RPOR5bits.RP10R = 3;    // Asigna U1TX a RP10
    RPINR18bits.U1RXR = 17; // Asigna U1RX a RP17
    __builtin_write_OSCCONL(OSCCON | 0x40); // Bloqueo PPS
}

void UART1_Init(unsigned long baudrate) 
{
    U1MODEbits.UARTEN = 0;  // Apagar módulo
    U1MODEbits.BRGH = 1;    // Alta velocidad activa
    U1MODEbits.PDSEL = 0b00;// 8 bits, sin paridad
    U1MODEbits.STSEL = 0;   // 1 bit stop
    
    U1BRG = (unsigned int)((FCY / (4.0 * baudrate)) - 0.5); // División por hardware entre 4
    
    U1STA = 0;
    
    // Configuración de la Interrupción de Recepción UART1
    IPC2bits.U1RXIP = 5;    // Nivel de prioridad 5
    IFS0bits.U1RXIF = 0;    // Limpiar bandera
    IEC0bits.U1RXIE = 1;    // Habilitar la interrupción
    
    U1MODEbits.UARTEN = 1;  // Encender módulo globalmente
    U1STAbits.UTXEN = 1;    // Habilitar pin de transmisión
}

void Check_UART1_Errors(void) 
{
    if(U1STAbits.OERR == 1) {
        U1STAbits.OERR = 0; // Evita el congelamiento de la UART por Overflow
    }
}

void UART1_Write(char TxByte) 
{
    while(U1STAbits.UTXBF); // Esperar si el buffer de transmisión está lleno
    U1TXREG = TxByte;
}

void UART1_Write_Text(const char* text) 
{
    while(*text) 
    {
        UART1_Write(*text++); 
    }
}

void UART1_Write_Ln(void) 
{
    UART1_Write('\r'); 
    UART1_Write('\n'); 
}

void configurar_pines_led(void)
{
    ANSEbits.ANSE5 = 0;
    TRISEbits.TRISE5 = 0;
}



void UART1_Flush(void)
{
    // Vacía el buffer de entrada por hardware del módulo UART1
    while (U1STAbits.URXDA) 
    {
        volatile uint8_t dummy = U1RXREG;
        (void)dummy;
    }
    
    // Si la UART se bloqueó por un Overrun Error, lo reseteamos
    if (U1STAbits.OERR) 
    {
        U1STAbits.OERR = 0;
    }
}

void DACAI_Wait_Not_Busy(void) 
{
    uint16_t timeout = 50000; // Límite de seguridad anti-bloqueo (~10-15 ms a 16 MIPS)
    
    // Mientras la pantalla esté ocupada (BUSY = 1) y no se haya agotado el contador
    while (DACAI_BUSY_PIN == 1 && timeout > 0) 
    {
        timeout--;
    }
}


// Al final de tu archivo serial.c:

void UART2_Init(unsigned long baudrate) {
    // 1. Deshabilitar UART2 para cambios limpios
    TRISFbits.TRISF13 = 0;
    U2MODEbits.UARTEN = 0;

    // 2. Mapear el Pin Físico 39 (RF13 / RP31) como TX2 mediante Peripheral Pin Select (PPS)
    TRISFbits.TRISF13 = 0; // Pin 39 definido como salida digital

    __builtin_write_OSCCONL(OSCCON & 0xBF); // Desbloquear registros PPS
    RPOR15bits.RP31R = 5;                  // Asignar función U2TX al pin RP31 (Pin 39)
    __builtin_write_OSCCONL(OSCCON | 0x40); // Bloquear registros PPS

    // 3. Configurar Baud Rate y formato (BRGH = 1 -> Modo Alta Velocidad)
    U2MODEbits.BRGH = 1;
    U2MODEbits.PDSEL = 0b00; // 8 bits, sin paridad
    U2MODEbits.STSEL = 0;    // 1 bit de parada

    // Cálculo del registro U2BRG para 9600 baudios a FCY = 16MHz
    U2BRG = (unsigned int)((FCY / (4.0 * baudrate)) - 0.5);

    // 4. Encender el módulo UART2 y activar Transmisión
    U2STA = 0;
    U2MODEbits.UARTEN = 1;
    U2STAbits.UTXEN = 1;
}

// Envía un byte a la impresora
void UART2_Write(char TxByte) {
    while(U2STAbits.UTXBF); // Esperar si el buffer de transmisión está lleno
    U2TXREG = TxByte;
}

void UART2_Write_Text(const char* text) {
    // Recorre la cadena caracter por caracter hasta encontrar el fin de cadena ('\0')
    while(*text != '\0') {
        UART2_Write(*text);
        text++; // Avanza al siguiente carácter
    }
}

// Envía texto a la impresora
void Printer_Write_Text(const char* text) {
    while(*text) {
        UART2_Write(*text++);
    }
}

void Printer_Reset(void) {
    UART2_Write_Text(IMP_ESC_INIT);
}

void Printer_Set_Bold(uint8_t estado) {
    if(estado) {
        UART2_Write_Text(IMP_BOLD_ON);
    } else {
        UART2_Write_Text(IMP_BOLD_OFF);
    }
}

void Printer_Set_Align(uint8_t modo) {
    switch(modo) {
        case 0: UART2_Write_Text(IMP_ALIGN_LEFT); break;
        case 1: UART2_Write_Text(IMP_ALIGN_CENTER); break;
        case 2: UART2_Write_Text(IMP_ALIGN_RIGHT); break;
        default: UART2_Write_Text(IMP_ALIGN_LEFT); break;
    }
}

void Printer_Feed(uint8_t lineas) {
    uint8_t i;
    for(i = 0; i < lineas; i++) {
        UART2_Write_Text("\r\n");
    }
}

void Imprimir_Encabezado_Ticket(const char* nombre_programa, uint16_t num_ciclo) {
    char buf[32];
    Printer_Reset();
    Printer_Set_Align(1); // Centrado
    Printer_Set_Bold(1);
    UART2_Write_Text("========================\r\n");
    UART2_Write_Text("  AUTOCLAVE HOSPITAL NAVAL DE ALTA ESPECIALIDAD DE VERACRUZ\r\n");
    UART2_Write_Text("========================\r\n");
    Printer_Set_Bold(0);
    
    Printer_Set_Align(0); // Izquierda
    sprintf(buf, "CICLO N: %04u\r\n", num_ciclo);
    UART2_Write_Text(buf);
    
    sprintf(buf, "PROG   : %s\r\n", nombre_programa);
    UART2_Write_Text(buf);
    UART2_Write_Text("------------------------\r\n");
}

void Imprimir_Lectura_Sensores(const char* fecha, const char* hora, 
                               const char* etapa, uint16_t temp, 
                               uint16_t pres_camara, uint16_t pres_camisa) {
    char buf[32];
    Printer_Set_Align(0);
    sprintf(buf, "%s %s [%s]\r\n", fecha, hora, etapa);
    UART2_Write_Text(buf);
    
    sprintf(buf, " T: %u.%u C | P.CAM: %u.%u\r\n", temp / 10, temp % 10, pres_camara / 10, pres_camara % 10);
    UART2_Write_Text(buf);
    
    sprintf(buf, " P.CMS: %u.%u PSI\r\n", pres_camisa / 10, pres_camisa % 10);
    UART2_Write_Text(buf);
    UART2_Write_Text("------------------------\r\n");
}

void Imprimir_Pie_Ticket(uint8_t exito) {
    Printer_Set_Align(1);
    Printer_Set_Bold(1);
    if(exito) {
        UART2_Write_Text("*** CICLO CORRECTO ***\r\n");
    } else {
        UART2_Write_Text("! CICLO ABORTADO !\r\n");
    }
    Printer_Set_Bold(0);
    Printer_Feed(3);
}

/**
 * @brief Imprime un texto simple terminado en nulo en la impresora térmica.
 */
void Impresora_Print(const char *str) {
    while (*str) {
        UART2_Write(*str); // Envía cada carácter por la UART de la impresora
        str++;
    }
}

/**
 * @brief Envía comandos ESC/POS básicos a la impresora (Avanzar papel, corte, etc.)
 */
void Impresora_Feed(uint8_t lineas) {
    for (uint8_t i = 0; i < lineas; i++) {
        UART2_Write('\n');
    }
}

/**
 * @brief Genera e imprime el ticket de registro de encendido de la autoclave.
 */
void Imprimir_Registro_Encendido(void) {
    char buffer[32];

    // Encabezado del Ticket
    Impresora_Print("==============================\n");
    Impresora_Print("      AUTOCLAVE REGISTRO      \n");
    Impresora_Print("     ENCENDIDO DE EQUIPO      \n");
    Impresora_Print("==============================\n");

    // Fecha (con casteo explícito a unsigned int para XC16)
    sprintf(buffer, "FECHA: %02u/%02u/20%02u\n", 
            (unsigned int)fecha_hora_encendido.dia, 
            (unsigned int)fecha_hora_encendido.mes, 
            (unsigned int)fecha_hora_encendido.anio);
    Impresora_Print(buffer);

    // Hora (aplicado el mismo casteo para evitar distorsiones en el stack)
    sprintf(buffer, "HORA : %02u:%02u:%02u\n", 
            (unsigned int)fecha_hora_encendido.hora, 
            (unsigned int)fecha_hora_encendido.min, 
            (unsigned int)fecha_hora_encendido.sec);
    Impresora_Print(buffer);

    // Pie de ticket
    Impresora_Print("ESTADO: SISTEMA LISTO\n");
    Impresora_Print("==============================\n\n\n");
    
    // Avanza 3 líneas para poder cortar o rasgar el papel
    Impresora_Feed(3); 
}

void Impresora_Imprimir_Lectura(const char* fase) {
    char buffer_linea[64];
    
    // Convierte variables de punto fijo para visualización con decimales
    float pres_camara = (float)sensorCamara / 1.0f;  // ej. 2415 -> 24.15 PSI
    float pres_camisa = (float)sensorCamisa / 1.0f;  // ej. 2415 -> 24.15 PSI
    float temp = (float)sensorTemperatura / 1.0f;     // ej. 1215 -> 121.5 °C
    
    // Cabecera de la etapa/fase
    sprintf(buffer_linea, "--- %s ---\r\n", fase);
    UART2_Write_Text(buffer_linea);
    
    // Impresión de Fecha y Hora capturadas del RTC Dacai
    sprintf(buffer_linea, "Fecha: %02u/%02u/20%02u  Hora: %02u:%02u:%02u\r\n",
            fecha_hora_actual.dia, fecha_hora_actual.mes, fecha_hora_actual.anio,
            fecha_hora_actual.hora, fecha_hora_actual.min, fecha_hora_actual.sec);
    UART2_Write_Text(buffer_linea);
    
    // Impresión de las variables de proceso
    sprintf(buffer_linea, "P. Cam: %.2f PSI | P. Camisa: %.2f PSI\r\n", pres_camara, pres_camisa);
    UART2_Write_Text(buffer_linea);
    
    sprintf(buffer_linea, "Temp: %.1f C\r\n", temp);
    UART2_Write_Text(buffer_linea);
    UART2_Write_Text("--------------------------------\r\n\r\n");
}

#endif // SERIAL_H