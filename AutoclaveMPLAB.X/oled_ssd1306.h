#ifndef OLED_SSD1306_H
#define	OLED_SSD1306_H

#include <xc.h>
#include <stdio.h>
// Definición de frecuencia para tiempos de la librería (8 MHz / 2)
//#define FCY 4000000UL

// Dirección esclava de la OLED (Ajustar a 0x7A si tu pantalla usó esa)
#define OLED_I2C_ADDRESS    0x78

// Prototipos de funciones de Bajo Nivel I2C
void I2C1_Init(void);
unsigned char I2C1_Start_Timeout(void);
void I2C1_Stop_Timeout(void);
unsigned char I2C1_Write_Timeout(unsigned char data);

// Prototipos de funciones de la Pantalla OLED
void OLED_Write_Command(unsigned char cmd);
void OLED_Write_Data(unsigned char data);
void OLED_Init(void);
void OLED_Clear_To_Black(void);
void OLED_Set_Cursor(unsigned char pagina, unsigned char columna);
void OLED_Print_Char(char c);
void OLED_Print_String(const char* str);
void OLED_Config(void);
void OLED_Mostrar_Parametros_Activos(void);

#endif	/* OLED_SSD1306_H */