#include "xc.h"
#include "config.h"
#include "serial.h"
#include "adc.h"
#include "oled_ssd1306.h"
#include "pantalla.h"
#include "ciclos.h"
#include <stdio.h>


// =============================================================================
// CONFIGURACIÓN DEL RELOJ (FRC CON PLL CORRIENDO A 32 MHz)
// =============================================================================
#pragma config POSCMD = NONE     // Oscilador primario deshabilitado
#pragma config FNOSC = FRCPLL    // Activa el FRC con multiplicador PLL
#pragma config OSCIOFCN = OFF   // CLKO habilitado
#pragma config FCKSM = CSDCMD    // Cambio de reloj desactivado
#pragma config SOSCSEL = 0       // SOSC en OFF deshabilitado

#pragma config FWDTEN = 0        // Watchdog Timer desactivado
#pragma config ICS = PGx1        // Debug sobre los pines EMUC1/EMUD1
#pragma config GWRP = OFF        // Protección contra escritura desactivada
#pragma config JTAGEN = 0        // JTAG apagado para liberar pines digitales
#pragma config IOL1WAY = OFF

volatile int16_t  sensorCamara = 0;      // int16_t admite presiones negativas de vacío
volatile int16_t  sensorCamisa = 0;      // int16_t admite presiones negativas
volatile uint16_t sensorTemperatura = 0;
volatile uint16_t sensorExtra = 0;

volatile uint8_t  solicitar_id_pendiente = 0;
volatile uint16_t ticks_pantalla = 0;

volatile uint8_t pagina_actual = 0;
volatile uint8_t solicitar_id_pantalla = 0;
volatile uint8_t estadoLlenado = 0;
volatile uint8_t sensorNivelAguaOK = 0;
volatile uint8_t sensorCamisaOK = 0;

char buffer_texto_Camara[10] = {0};
char buffer_texto_Camisa[10] = {0};
char buffer_texto_Temperatura[10] = {0};
char buffer_texto_Auxiliar[10] = {0};

volatile uint8_t estado_puerta_actual = 0;

volatile RTC_Dacai_t fecha_hora_actual;
volatile RTC_Dacai_t fecha_hora_encendido;
volatile uint8_t flag_encendido_registrado = 0;

volatile uint16_t contador_segundos_impresion = 0;

int main(void) 
{
    System_Clock_Init(); // Configura reloj a 32MHz
    
    PINES_Init();
    
    UART1_Pins_Init();   // Mapea pines RP10 y RP17
    UART1_Init(115200);    // Inicializa UART a 9600 baudios
    UART2_Init(9600);
    
    __delay_ms(20);
    
    ADC_Init();             // Inicializa entradas analógicas
    Timer2_Init();
    OLED_Config();
    
    __delay_ms(2000);
    DACAI_Show_Screen(0);
    __delay_ms(2000);     // Estabilización del sistema
    UART1_Flush();
    
    DACAI_Buzzer(); 
    pagina_actual = 1;
    DACAI_Show_Screen(1);
    DACAI_Limpiar_Etiquetas(1, 6);
    __delay_ms(2000);
    
    //DACAI_Request_RTC();
    __delay_ms(200);
    
    configurar_pines_led();  
    Init_Control_Nivel();
    
    autoclave.estado_actual = ESTADO_REPOSO;
    autoclave.flag_iniciar = 0;
    autoclave.flag_cancelar = 0;
    autoclave.contador_prevacios = 0;
    
    Precargar_Filtros_Iniciales();
    
    while(1) 
    {   
        
        Check_UART1_Errors(); // Limpieza constante de desbordamientos
        
        // Si la interrupción detectó el cierre de trama (FF FC FF FF)
        if (tramaLista == 1) 
        {
            Procesar_Comando_Dacai();
            
            // Limpieza manual y reset para prepararse a recibir el siguiente comando
            tramaLista = 0; 
            rxIndex = 0;
        }
        //DACAI_Buzzer(); 
        
        if(bandera100ms == 1)
        {
            Leer_Sensores();
            Controlar_Nivel_Tanque();
            Controlar_Presion_Camisa();
            
            Procesar_Maquina_Estados();

            sprintf(buffer_texto_Camara, "%d", sensorCamara);
            sprintf(buffer_texto_Camisa, "%d", sensorCamisa);
            sprintf(buffer_texto_Temperatura, "%u.%u   ", sensorTemperatura / 10, sensorTemperatura % 10);
            //sprintf(buffer_texto_Auxiliar, "%u  ", pagina_actual);
            sprintf(buffer_texto_Auxiliar, "%u  ", fecha_hora_actual.anio);
            
            /*OLED_Set_Cursor(2, 10);
            OLED_Print_String(buffer_texto_Camara);
            OLED_Set_Cursor(3,10);
            OLED_Print_String("Presion Camisa");
            OLED_Set_Cursor(4, 10);
            OLED_Print_String(buffer_texto_Camisa);
            OLED_Set_Cursor(5,10);
            OLED_Print_String("Temperatura");
            //OLED_Print_String("Pagina");
            OLED_Set_Cursor(6, 10);
            //OLED_Print_String(buffer_texto_Temperatura);
            
            
            //OLED_Set_Cursor(6, 10);
            OLED_Print_String(buffer_texto_Temperatura);
            //DACAI_Buzzer(); 
            */
            
            DACAI_Solicitar_Screen_ID();
            
            if (PUERTA == 0) 
            {
                // Puerta Cerrada: Cambia el icono/indicador a Verde / Seguro
                DACAI_Show_Control(pagina_actual, 23); // 1 = Icono Cerrada/Verde
            } 
            else 
            {
                // Puerta Abierta: Cambia el icono a Rojo / Advertencia
                DACAI_Hide_Control(pagina_actual, 23); // 0 = Icono Abierta/Rojo
            }
            
            DACAI_Set_Slider_Value(pagina_actual,26,sensorCamara);
            DACAI_Set_Slider_Value(pagina_actual,27,sensorCamisa);
            DACAI_Set_Text(pagina_actual,24,buffer_texto_Temperatura);  
            DACAI_Set_Text(pagina_actual,28,buffer_texto_Camara);  
            DACAI_Set_Text(pagina_actual,29,buffer_texto_Camisa);
            
            bandera100ms = 0;
            
        }
        
        if (bandera1seg == 1) 
        {
            
            OLED_Clear_To_Black(); 

            OLED_Set_Cursor(1, 10);
            OLED_Print_String("Autoclave");
            //PRUEBA = !PRUEBA;
            DACAI_Request_RTC();
            bandera1seg = 0; // Limpia la bandera
            // Control de descuento para ESTADO_ESTERILIZACION
            if (autoclave.estado_actual == ESTADO_ESTERILIZACION) 
            {
                if (sensorTemperatura >= autoclave.parametros_activos.temp_objetivo) 
                {
                    if (autoclave.temporizador_etapa_seg > 0) 
                    {
                        autoclave.temporizador_etapa_seg--;
                        contador_segundos_impresion++;

                        // Cada 120 segundos (2 minutos), imprime lectura periódica
                        if (contador_segundos_impresion >= 120) 
                        {
                            contador_segundos_impresion = 0;
                            Impresora_Imprimir_Lectura("ESTERILIZANDO");
                        }
                    }
                }
            }

            // Control de descuento para ESTADO_SECADO
            else if (autoclave.estado_actual == ESTADO_SECADO) 
            {
                if (autoclave.temporizador_etapa_seg > 0) 
                {
                    autoclave.temporizador_etapa_seg--;
                    contador_segundos_impresion++;

                    // Cada 120 segundos (2 minutos), imprime lectura periódica
                    if (contador_segundos_impresion >= 120) 
                    {
                        contador_segundos_impresion = 0;
                        Impresora_Imprimir_Lectura("SECANDO");
                    }
                }
            }
        }
    }
    return 0;
}
