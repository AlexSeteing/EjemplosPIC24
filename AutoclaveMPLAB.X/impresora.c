#include <xc.h>
#include "serial.h"     // Ofrece UART2_Write_Text y las macros de sistema
#include "impresora.h"

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
    UART2_Write_Text("  AUTOCLAVE HOSPITALARIA\r\n");
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
    
    Printer_Set_Align(0); // Izquierda
    sprintf(buf, "%s %s [%s]\r\n", fecha, hora, etapa);
    UART2_Write_Text(buf);
    
    sprintf(buf, " T: %u.%u C | P.CAM: %u.%u\r\n", temp / 10, temp % 10, pres_camara / 10, pres_camara % 10);
    UART2_Write_Text(buf);
    
    sprintf(buf, " P.CMS: %u.%u PSI\r\n", pres_camisa / 10, pres_camisa % 10);
    UART2_Write_Text(buf);
    UART2_Write_Text("------------------------\r\n");
}

void Imprimir_Pie_Ticket(uint8_t exito) {
    Printer_Set_Align(1); // Centrado
    Printer_Set_Bold(1);
    if(exito) {
        UART2_Write_Text("*** CICLO CORRECTO ***\r\n");
    } else {
        UART2_Write_Text("! CICLO ABORTADO !\r\n");
    }
    Printer_Set_Bold(0);
    
    Printer_Feed(3); // Avance de papel para fácil corte
}