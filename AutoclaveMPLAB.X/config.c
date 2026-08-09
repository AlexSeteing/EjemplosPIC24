#include "config.h"

void PINES_Init(void) 
{
    //LED1 RG15
    //LED2 RE5
    //LED3 RE7
    //LED4 RC1
    //LED5 RC2
    //LED6 RC3
    //LED7 RC4
    //LED8 RG6
    
    ANSEbits.ANSE5 = 0;
    ANSEbits.ANSE7 = 0;
    ANSCbits.ANSC4 = 0;
    ANSGbits.ANSG6 = 0;
    
    TRISGbits.TRISG15 = 0;
    TRISEbits.TRISE5 = 0;
    TRISEbits.TRISE7 = 0;
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    TRISCbits.TRISC3 = 0;
    TRISCbits.TRISC4 = 0;
    TRISGbits.TRISG6 = 0;
    
    TRISAbits.TRISA7 = 0;
    
    BOMBA_AGUA = 0;
    RESISTENCIAS = 0;
    VAL_ENTRADA = 0;
    VAL_ESC_RAPIDO = 0;
    VAL_ESC_LENTO = 0;
    VAL_SECADO = 0;
    VAL_ENTRADA_AIRE = 0;
    BUZZER = 0;
    
    ANSGbits.ANSG7 = 0;
    ANSGbits.ANSG8 = 0;
    ANSGbits.ANSG9 = 0;
    //ANSAbits.ANS0 = 0;
    //ANSEbits.ANSE8 = 0;
    ANSEbits.ANSE9 = 0;
    ANSBbits.ANSB4 = 0;
    ANSAbits.ANSA9 = 0;
    ANSAbits.ANSA7 = 0;
    
    //Configuramos los pines como entrada
    TRISGbits.TRISG7 = 1;
    TRISGbits.TRISG8 = 1;
    TRISGbits.TRISG9 = 1;
    TRISAbits.TRISA0 = 1;
    TRISEbits.TRISE8 = 1;
    TRISEbits.TRISE9 = 1;
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA9 = 1;
}

void Init_Control_Nivel(void) 
{
    // Configurar bomba como salida y asegurar que inicie apagada
    BOMBA_AGUA = 0;
    estadoLlenado = ESTADO_INIT;
}

void Controlar_Nivel_Tanque(void) 
{
    // Variable estática para detectar el momento exacto en que se apaga el nivel alto
    static uint8_t avisoNivelAltoDesactivado = 0; 

    switch (estadoLlenado) 
    {
        case ESTADO_INIT:
            // "Al iniciar se tienen que revisar los dos y llenar a tope"
            if (NIVEL_ALTO == 1 && NIVEL_BAJO == 1) 
            {
                BOMBA_AGUA = 0;
                estadoLlenado = ESTADO_ESPERA_VACIO;
                avisoNivelAltoDesactivado = 0; // Reseteamos la bandera
                //DACAI_Set_Text(pagina_actual, 41, "Lleno");
                DACAI_Set_Text(pagina_actual, 41, "Lleno");
                sensorNivelAguaOK = 1;
            } else 
            {
                BOMBA_AGUA = 1;
                estadoLlenado = ESTADO_LLENANDO;
                DACAI_Set_Text(pagina_actual, 41, "Llenando...");
            }
            break;

        case ESTADO_LLENANDO:
            BOMBA_AGUA = 1; 
            
            if (NIVEL_ALTO == 1) {
                BOMBA_AGUA = 0;
                estadoLlenado = ESTADO_ESPERA_VACIO;
                avisoNivelAltoDesactivado = 0; // Tanque lleno, reiniciamos detector
                DACAI_Set_Text(pagina_actual, 41, "Lleno");
                sensorNivelAguaOK = 1;
            }
            break;

        case ESTADO_ESPERA_VACIO:
            // La bomba permanece estrictamente APAGADA mientras baja el agua
            BOMBA_AGUA = 0; 
            
            // NUEVO: Si el nivel alto se desactiva pero todavía hay agua en el nivel bajo
            if (NIVEL_ALTO == 0 && NIVEL_BAJO == 1) {
                if (avisoNivelAltoDesactivado == 0) {
                    DACAI_Set_Text(pagina_actual, 41, "Nivel Alto OFF");
                    avisoNivelAltoDesactivado = 1; // Bloqueamos el envío para que solo lo mande una vez
                    sensorNivelAguaOK = 1;
                }
            }
            
            // Si el nivel bajo se queda finalmente sin agua, mandamos a llenar a tope
            if (NIVEL_BAJO == 0) {
                BOMBA_AGUA = 1;
                estadoLlenado = ESTADO_LLENANDO;
                DACAI_Set_Text(pagina_actual, 41, "Llenando...");
                sensorNivelAguaOK = 0;
            }
            break;

        default:
            BOMBA_AGUA = 0; 
            estadoLlenado = ESTADO_INIT;
            break;
    }
}

void ENCENDER_RESISTENCIAS_CAMARA(void)
{
    // Regla de seguridad opcional: No encender si hay falla crítica de nivel de agua
    if (NIVEL_BAJO == 1) 
    {
        RESISTENCIAS = 1; // Enciende relé/Triac
    }
    else 
    {
        RESISTENCIAS = 0; // Inhabilita por seguridad
    }
}