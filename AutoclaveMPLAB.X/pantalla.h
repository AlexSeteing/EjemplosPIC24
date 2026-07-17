void DACAI_Show_Screen(uint16_t screen_id) 
{
    // Aseguramos que no existan errores previos en el módulo antes de transmitir
    Check_UART1_Errors(); 
    
    UART1_Write(0xEE);  // Byte de inicio de trama (Frame Header)
    UART1_Write(0xB1);  // Código de comando (Control de Pantalla)
    UART1_Write(0x00);  // Subcomando / Parámetro fijo
    
    // Descomponemos el ID de 16 bits en dos bytes independientes
    UART1_Write((uint8_t)(screen_id >> 8));   // Byte Alto del ID de la pantalla
    UART1_Write((uint8_t)(screen_id & 0xFF));  // Byte Bajo del ID de la pantalla
    
    UART1_Write(0xFF);  // Inicio de fin de trama (Frame Tail)
    UART1_Write(0xFC);  
    UART1_Write(0xFF);  
    UART1_Write(0xFF);  // Fin de la instrucción
}

void DACAI_Buzzer(void) 
{
    // Aseguramos que no existan errores previos en el módulo antes de transmitir
    Check_UART1_Errors(); 
    
    UART1_Write(0xEE);  // Byte de inicio de trama (Frame Header)
    UART1_Write(0x61);  // Código de comando
    UART1_Write(0x0A);  // Parámetro / Datos
    UART1_Write(0xFF);  // Inicio de fin de trama (Frame Tail)
    UART1_Write(0xFC);  
    UART1_Write(0xFF);  
    UART1_Write(0xFF);  // Fin de la instrucción
}

void DACAI_Set_Slider_Value(uint16_t screen_id, uint16_t control_id, uint32_t value) 
{
    // Aseguramos que no existan errores previos en el módulo antes de transmitir
    Check_UART1_Errors(); 
    
    UART1_Write(0xEE);  // Byte de inicio de trama (Frame Header)
    UART1_Write(0xB1);  // Código de comando base
    UART1_Write(0x10);  // Subcomando: Escribir valor en componente (Set Value)
    
    // Descomponemos el Screen ID (16 bits) en 2 bytes
    UART1_Write((uint8_t)(screen_id >> 8));   // Screen ID High Byte
    UART1_Write((uint8_t)(screen_id & 0xFF));  // Screen ID Low Byte
    
    // Descomponemos el Control ID (16 bits) en 2 bytes
    UART1_Write((uint8_t)(control_id >> 8));  // Control ID High Byte
    UART1_Write((uint8_t)(control_id & 0xFF)); // Control ID Low Byte
    
    // Descomponemos el Valor (32 bits) en 4 bytes de forma consecutiva (Big-Endian)
    UART1_Write((uint8_t)(value >> 24)); // Byte 3 (Más significativo)
    UART1_Write((uint8_t)(value >> 16)); // Byte 2
    UART1_Write((uint8_t)(value >> 8));  // Byte 1
    UART1_Write((uint8_t)(value & 0xFF)); // Byte 0 (Menos significativo)
    
    UART1_Write(0xFF);  // Inicio de fin de trama (Frame Tail)
    UART1_Write(0xFC);  
    UART1_Write(0xFF);  
    UART1_Write(0xFF);  // Fin de la instrucción
}

void DACAI_Solicitar_Screen_ID(void) {
    
    // 1. Aseguramos que no existan errores de hardware previos en la UART antes de transmitir
    Check_UART1_Errors(); 
    
    // 2. Encabezado de la trama (Frame Header) y grupo de comandos
    UART1_Write(0xEE);  // byte de inicio
    UART1_Write(0xB1);  // Grupo de control
    
    // 3. Código de comando específico: 0x01 (Get Screen ID)
    UART1_Write(0x01);  
    
    // 4. Cierre reglamentario y obligatorio de la trama Dacai (Frame Tail)
    UART1_Write(0xFF);  
    UART1_Write(0xFC);   
    UART1_Write(0xFF);   
    UART1_Write(0xFF);  
}

void DACAI_Set_Text(uint16_t screenID, uint16_t controlID, const char* texto) 
{
    
    // 1. Verificar y limpiar posibles errores previos en la UART antes de transmitir
    Check_UART1_Errors(); 
    
    // 2. Encabezado de trama y comando Set Text
    UART1_Write(0xEE);  // Inicio de trama
    UART1_Write(0xB1);  // Comando de control
    UART1_Write(0x10);  // Subcomando: Set Text
    
    // 3. Enviar Screen ID (Byte Alto y luego Byte Bajo)
    UART1_Write((uint8_t)((screenID >> 8) & 0xFF));
    UART1_Write((uint8_t)(screenID & 0xFF));
    
    // 4. Enviar Control ID (Byte Alto y luego Byte Bajo)
    UART1_Write((uint8_t)((controlID >> 8) & 0xFF));
    UART1_Write((uint8_t)(controlID & 0xFF));
    
    // 5. Enviar la cadena de caracteres (ASCII) hasta encontrar el fin de cadena '\0'
    while (*texto != '\0') {
        UART1_Write((uint8_t)(*texto));
        texto++;
    }
    
    // 6. Cierre reglamentario y obligatorio de la trama Dacai
    UART1_Write(0xFF);
    UART1_Write(0xFC);
    UART1_Write(0xFF);
    UART1_Write(0xFF);
}