#include "oled_ssd1306.h"
#include "ciclos.h"

// Diccionario tipográfico básico (5 columnas de ancho x 8 píxeles de alto)
// Indexado de forma simple para caracteres comunes. Puedes expandirlo después.
const unsigned char Font_Basic[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // [Espacio] (ASCII 32)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0 (ASCII 48)
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A (ASCII 65)
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

void I2C1_Init(void) {
    TRISGbits.TRISG2 = 1; 
    TRISGbits.TRISG3 = 1; 
    I2C1BRG = 39;                // ~100 kHz estándar con FCY = 4 MHz
    I2C1CONbits.I2CEN = 1;    
    I2C1CONbits.DISSLW = 1;   
    I2C1STAT = 0x0000;        
}

unsigned char I2C1_Start_Timeout(void) {
    unsigned int timeout = 0xFFFF;
    I2C1CONbits.SEN = 1;        
    while(I2C1CONbits.SEN) {
        if (--timeout == 0) return 0;
    }
    return 1;
}

void I2C1_Stop_Timeout(void) {
    unsigned int timeout = 0xFFFF;
    I2C1CONbits.PEN = 1;        
    while(I2C1CONbits.PEN) {
        if (--timeout == 0) break;
    }
}

unsigned char I2C1_Write_Timeout(unsigned char data) {
    unsigned int timeout = 0xFFFF;
    I2C1TRN = data;             
    while(I2C1STATbits.TRSTAT) { 
        if (--timeout == 0) return 2; 
    }
    return I2C1STATbits.ACKSTAT; 
}

void OLED_Write_Command(unsigned char cmd) {
    I2C1_Start_Timeout();
    I2C1_Write_Timeout(OLED_I2C_ADDRESS);
    I2C1_Write_Timeout(0x00); 
    I2C1_Write_Timeout(cmd);
    I2C1_Stop_Timeout();
    for(volatile int delay = 0; delay < 300; delay++);
}

void OLED_Write_Data(unsigned char data) {
    I2C1_Start_Timeout();
    I2C1_Write_Timeout(OLED_I2C_ADDRESS);
    I2C1_Write_Timeout(0x40); 
    I2C1_Write_Timeout(data);
    I2C1_Stop_Timeout();
}

void OLED_Init(void) {
    for(volatile long i = 0; i < 80000; i++); // Espera de encendido

    OLED_Write_Command(0xAE); // Apagar Display
    OLED_Write_Command(0xD5); // Configurar Reloj
    OLED_Write_Command(0x80); 
    OLED_Write_Command(0xA8); // Multiplexado
    OLED_Write_Command(0x3F); // 64 Altura
    OLED_Write_Command(0xD3); // Offset
    OLED_Write_Command(0x00); 
    OLED_Write_Command(0x40); // Línea de inicio 0
    OLED_Write_Command(0x8D); // Bomba de carga
    OLED_Write_Command(0x14); // Habilitar interna
    OLED_Write_Command(0x20); // Direccionamiento
    OLED_Write_Command(0x02); // Modo de página
    OLED_Write_Command(0xA1); // Modo espejo desactivado
    OLED_Write_Command(0xC8); // Re-mapeo COM
    OLED_Write_Command(0xDA); // Configuración de pines COM
    OLED_Write_Command(0x12); 
    OLED_Write_Command(0x81); // Contraste
    OLED_Write_Command(0xCF); 
    OLED_Write_Command(0xD9); // Pre-carga
    OLED_Write_Command(0xF1); 
    OLED_Write_Command(0xDB); // VCOMH
    OLED_Write_Command(0x40); 
    OLED_Write_Command(0xA4); // RAM completa encendida
    OLED_Write_Command(0xA6); // Modo Normal
    OLED_Write_Command(0xAF); // Encender Display
    
    for(volatile long i = 0; i < 40000; i++);
}

void OLED_Clear_To_Black(void) {
    unsigned char pagina, columna;
    for (pagina = 0; pagina < 8; pagina++) {
        OLED_Set_Cursor(pagina, 0);
        for (columna = 0; columna < 128; columna++) {
            OLED_Write_Data(0x00);
        }
    }
}

void OLED_Set_Cursor(unsigned char pagina, unsigned char columna) {
    OLED_Write_Command(0xB0 + pagina);
    OLED_Write_Command(0x00 + (columna & 0x0F));
    OLED_Write_Command(0x10 + ((columna >> 4) & 0x0F));
}

void OLED_Print_Char(char c) {
    unsigned char i;
    unsigned int index;
    
    // Convertir de tabla ASCII (Inicia en Espacio = 32) al índice de la matriz interna
    if (c >= 32 && c <= 90) { // Maneja desde ' ' hasta letras Mayúsculas 'Z'
        index = c - 32;
    } else if (c >= 97 && c <= 122) { // Convierte letras minúsculas a mayúsculas automáticamente
        index = (c - 32) - 32;
    } else {
        index = 0; // Por defecto imprime un espacio si es un carácter desconocido
    }
    
    // Imprimir los 5 bytes horizontales de la fuente
    for (i = 0; i < 5; i++) {
        OLED_Write_Data(Font_Basic[index][i]);
    }
    OLED_Write_Data(0x00); // 1 columna de espacio en negro de separación
}

void OLED_Print_String(const char* str) {
    // Recorrer la cadena de caracteres usando punteros hasta hallar el carácter de fin de cadena '\0'
    while (*str) {
        OLED_Print_Char(*str++);
    }
}

void OLED_Config(void)
{
    // Asegurar que el reloj corra a 8 MHz exactos sin divisiones
    //CLKDIVbits.RCDIV = 0; 
    
    // Desactivar módulos analógicos y periféricos redundantes
    ANSELG = 0x0000;      //CAmbiarlos
    CM2CONbits.CON = 0;   
    LCDCONbits.LCDEN = 0; 

    // Inicializaciones de sistema
    I2C1_Init();          // Encender el periférico I2C1 del PIC
    OLED_Init();          // Despertar la pantalla OLED
    OLED_Clear_To_Black(); // Borrar pantalla para iniciar limpia

    // --- EJEMPLO DE ESCRITURA EN DIFERENTES SECCIONES ---
    
    // Fila superior (Página 1), Iniciando en columna 10
    OLED_Set_Cursor(1, 10);
    OLED_Print_String("Prueba Autoclave");

    // Fila inferior (Página 5), Iniciando en columna 30
    OLED_Set_Cursor(5, 30);
    OLED_Print_String("OLED");
}

void OLED_Mostrar_Parametros_Activos(void) 
{
    char buf_temp[16];
    char buf_prevacios[16];
    char buf_esterilizado[16];
    char buf_secado[16];

    // 1. Formateamos cada dato de la estructura activa
    // Convertimos tiempos de segundos a minutos para mejor lectura
    uint16_t min_est = autoclave.parametros_activos.tiempo_esterilizado / 60;
    uint16_t min_sec = autoclave.parametros_activos.tiempo_secado / 60;

    sprintf(buf_temp,        "Temp Obj:  %u C", (unsigned int)autoclave.parametros_activos.temp_objetivo);
    sprintf(buf_prevacios,   "Prevacios: %u",   (unsigned int)autoclave.parametros_activos.num_prevacios);
    sprintf(buf_esterilizado,"Esteril:  %u min", (unsigned int)min_est);
    sprintf(buf_secado,      "Secado:   %u min", (unsigned int)min_sec);

    // 2. Limpiamos pantalla y escribimos línea por línea
    OLED_Clear_To_Black();

    OLED_Set_Cursor(1, 0);
    OLED_Print_String("--- PARAMETROS ---");

    OLED_Set_Cursor(3, 0);
    OLED_Print_String(buf_temp);

    OLED_Set_Cursor(4, 0);
    OLED_Print_String(buf_prevacios);

    OLED_Set_Cursor(5, 0);
    OLED_Print_String(buf_esterilizado);

    OLED_Set_Cursor(6, 0);
    OLED_Print_String(buf_secado);
}