#ifndef IMPRESORA_H
#define IMPRESORA_H

#include <xc.h>
#include <stdio.h>
#include <stdint.h>

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
// PROTOTIPOS DE FORMATO Y CONTROL DE TICKET
// =============================================================================

/**
 * @brief Resetea el formato de la impresora a sus valores por defecto.
 */
void Printer_Reset(void);

/**
 * @brief Activa o desactiva la negrita.
 * @param estado 1 = Activado, 0 = Desactivado
 */
void Printer_Set_Bold(uint8_t estado);

/**
 * @brief Establece la alineación del texto.
 * @param modo 0 = Izquierda, 1 = Centro, 2 = Derecha
 */
void Printer_Set_Align(uint8_t modo);

/**
 * @brief Avanza una cantidad determinada de líneas de papel.
 * @param lineas Número de saltos de línea
 */
void Printer_Feed(uint8_t lineas);

// =============================================================================
// PROTOTIPOS DE REPORTES DE LA AUTOCLAVE
// =============================================================================

/**
 * @brief Imprime el encabezado principal del ticket de esterilización.
 * @param nombre_programa Nombre del ciclo (Ej: "INSTRUMENTAL", "TEXTIL", etc.)
 * @param num_ciclo Número consecutivo de ciclo
 */
void Imprimir_Encabezado_Ticket(const char* nombre_programa, uint16_t num_ciclo);

/**
 * @brief Imprime un registro instantáneo de los sensores de la autoclave.
 * @param fecha Cadena con la fecha (Ej: "01/08/2026")
 * @param hora Cadena con la hora (Ej: "09:30")
 * @param etapa Nombre de la etapa ("LLENADO", "ESTERILIZANDO", "SECADO", etc.)
 * @param temp Temperatura en décimas de grado (1215 = 121.5°C)
 * @param pres_camara Presión de cámara en décimas de PSI (150 = 15.0 PSI)
 * @param pres_camisa Presión de camisa en décimas de PSI (200 = 20.0 PSI)
 */
void Imprimir_Lectura_Sensores(const char* fecha, const char* hora, 
                               const char* etapa, uint16_t temp, 
                               uint16_t pres_camara, uint16_t pres_camisa);

/**
 * @brief Imprime el cierre del reporte de esterilización.
 * @param exito 1 = Ciclo completado correctamente, 0 = Cancelado / Error
 */
void Imprimir_Pie_Ticket(uint8_t exito);

#endif /* IMPRESORA_H */