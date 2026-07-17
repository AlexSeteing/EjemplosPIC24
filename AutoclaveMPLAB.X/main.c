#include "xc.h"
#include "config.h"
#include "serial.h"
#include "adc.h"
#include "oled_ssd1306.h"
#include "pantalla.h"
#include <stdio.h>


int main(void) 
{
    System_Clock_Init(); // Configura reloj a 32MHz
    
    PINES_Init();
    
    UART1_Pins_Init();   // Mapea pines RP10 y RP17
    UART1_Init(9600);    // Inicializa UART a 9600 baudios
    
    ADC_Init();             // Inicializa entradas analógicas
    Timer2_Init();
    OLED_Config();
    
    DACAI_Show_Screen(0);
    __delay_ms(500);     // Estabilización del sistema
    
    DACAI_Buzzer(); 
    DACAI_Show_Screen(1);
    
    
    configurar_pines_led();
    
    Init_Control_Nivel();

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

            sprintf(buffer_texto_Camara, "%u    ", sensorCamara);
            sprintf(buffer_texto_Camisa, "%u    ", sensorCamisa);
            sprintf(buffer_texto_Temperatura, "%u   ", sensorTemperatura);
            sprintf(buffer_texto_Auxiliar, "%u  ", pagina_actual);
            OLED_Clear_To_Black(); 

            OLED_Set_Cursor(1, 10);
            OLED_Print_String("Presion Camara");
            OLED_Set_Cursor(2, 10);
            OLED_Print_String(buffer_texto_Camara);
            OLED_Set_Cursor(3,10);
            OLED_Print_String("Presion Camisa");
            OLED_Set_Cursor(4, 10);
            OLED_Print_String(buffer_texto_Camisa);
            OLED_Set_Cursor(5,10);
            OLED_Print_String("Temperatura");
            //OLED_Print_String("Pagina");
            OLED_Set_Cursor(6, 10);
            OLED_Print_String(buffer_texto_Temperatura);
            //OLED_Set_Cursor(6, 10);
            //OLED_Print_String(buffer_texto_Auxiliar);

            DACAI_Buzzer(); 
            DACAI_Solicitar_Screen_ID();
            
            DACAI_Set_Slider_Value(pagina_actual,26,sensorCamara);
            DACAI_Set_Slider_Value(pagina_actual,27,sensorCamisa);
            DACAI_Set_Text(pagina_actual,24,buffer_texto_Temperatura);  
            DACAI_Set_Text(pagina_actual,28,buffer_texto_Camara);  
            DACAI_Set_Text(pagina_actual,29,buffer_texto_Camisa);
            
            bandera100ms = 0;
        }
    }
    return 0;
}
