#include "ciclos.h"
#include "config.h"
#include <stdio.h>
#include "oled_ssd1306.h"
#include "pantalla.h"

extern volatile uint8_t sensorNivelAguaOK;
extern volatile uint8_t sensorCamisaOK;

extern volatile uint16_t sensorTemperatura;
extern volatile int16_t  sensorCamara;

extern volatile uint8_t flag_vacio_alcanzado;
extern uint16_t contador_segundos_impresion;


ControlAutoclave_t autoclave = {
    .estado_actual = ESTADO_REPOSO,
    .programa_seleccionado = PROGRAMA_ROPA,
    // Copiar los valores del elemento TABLA_PROGRAMAS correspondiente a ROPA:
    .parametros_activos = {"ROPA", 3, 1210, 24, 1200, 900}, // Ajusta tiempos/prevacíos según tu tabla
    .contador_prevacios = 0,
    .temporizador_etapa_seg = 0,
    .flag_iniciar = 0,
    .flag_cancelar = 0,
};

const ParametrosCiclo_t TABLA_PROGRAMAS[5] = {
    // [0] PROGRAMA_ROPA
    { .temp_objetivo = 1210, .num_prevacios = 2, .tiempo_esterilizado = 1800, .tiempo_secado = 1800 },
    
    // [1] PROGRAMA_INSTRUMENTAL
    { .temp_objetivo = 1320, .num_prevacios = 2, .tiempo_esterilizado = 300, .tiempo_secado = 600 },
    
    // [2] PROGRAMA_BOWIE
    { .temp_objetivo = 1320, .num_prevacios = 3, .tiempo_esterilizado = 210, .tiempo_secado = 300 },
    
    // [3] PROGRAMA_LIQUIDOS
    { .temp_objetivo = 1210, .num_prevacios = 0, .tiempo_esterilizado = 1200, .tiempo_secado = 0 },
    
    // [4] PROGRAMA_ESPECIAL
    { .temp_objetivo = 1210, .num_prevacios = 2, .tiempo_esterilizado = 900, .tiempo_secado = 600 }
};

void Seleccionar_Programa(TipoPrograma_t prog) {
    if (autoclave.estado_actual == ESTADO_REPOSO) {
        autoclave.programa_seleccionado = prog;
        
        if (prog != PROGRAMA_MANUAL) {
            // Copia completa de parámetros del programa seleccionado
            autoclave.parametros_activos = TABLA_PROGRAMAS[prog];
        }

        // --- REINICIO DE VARIABLES DE CICLO ---
        autoclave.contador_prevacios = 0;
        autoclave.temporizador_etapa_seg = 0;
        autoclave.subestado_prevacio = 0;
    }
}

void Procesar_Maquina_Estados(void)
{
    // Manejo de Cancelación de Emergencia
    if (autoclave.flag_cancelar) 
    {
        autoclave.flag_cancelar = 0;

        // 1. Apagar actuadores de proceso y abrir escape
        RESISTENCIAS_LED = 0;
        RESISTENCIAS = 0;
        VAL_ENTRADA_LED = 0;
        VAL_ENTRADA = 0;
        VAL_ESC_RAPIDO_LED = 1;      // Abre escape para aliviar presión retenida
        VAL_ESC_RAPIDO = 1;
        VAL_SECADO_LED = 0;
        VAL_SECADO = 0;
        VAL_ENTRADA_AIRE_LED = 0;
        VAL_ENTRADA_AIRE = 0;

        // 2. Cambiar estado
        autoclave.estado_actual = ESTADO_REPOSO;

        // 3. Mostrar mensaje y configurar temporizador (ejemplo: 3000 ms = 3 segundos)
        DACAI_Set_Text(pagina_actual, 35, "CANCELADO"); 
        autoclave.timer_mensaje_ms = 3000; // Mantiene el mensaje visible 3 segundos sin bloquear

        return;
    }

    switch (autoclave.estado_actual) 
    {
        
        // Variable estática para detectar CUÁNDO cambia el estado de la purga
        static uint8_t estaba_despresurizando = 0;

        case ESTADO_REPOSO:
            DACAI_Enable_Button(pagina_actual, 3);
            DACAI_Enable_Button(pagina_actual, 4);
            DACAI_Enable_Button(pagina_actual, 11);
            DACAI_Enable_Button(pagina_actual, 36);
            
            BUZZER = 0;
            DACAI_Set_Text(pagina_actual, 43,  " ");
            // -----------------------------------------------------------------
            // 1. EVALUACIÓN DE SEGURIDAD
            // -----------------------------------------------------------------
            
            if (autoclave.timer_mensaje_ms > 0) {
                // Decrementas el tiempo según tu base de tiempo (ej. si el bucle corre cada 10ms):
                // Nota: Es mejor decrementar esta variable dentro de la interrupción de un Timer.
                if (autoclave.timer_mensaje_ms > 10) {
                    autoclave.timer_mensaje_ms -= 10; 
                } else {
                    autoclave.timer_mensaje_ms = 0;
                    // ¡AQUÍ SE LIMPIA EL MENSAJE! 
                    DACAI_Set_Text(pagina_actual, 35, " "); // O pon "SISTEMA LISTO" si prefieres
                }
            }
            
            if (PUERTA == 1 && sensorCamara > 3) {
                VAL_ESC_RAPIDO_LED = 1; // Abrir válvula
                VAL_ESC_RAPIDO = 1;

                // Si no estaba despresurizando, envía el mensaje UNA SOLA VEZ
                if (estaba_despresurizando == 0) {
                    DACAI_Set_Text(pagina_actual, 42, "LIBERANDO PRESION...");
                    estaba_despresurizando = 1; // Registra que la alerta está activa
                }
            } 
            else {
                VAL_ESC_RAPIDO_LED = 0; // Cierra la válvula de escape
                VAL_ESC_RAPIDO = 0;

                VAL_ENTRADA_LED = 0;
                VAL_ENTRADA = 0;
                VAL_SECADO_LED = 0;
                VAL_SECADO = 0;
                VAL_ENTRADA_AIRE_LED = 0;
                VAL_ENTRADA_AIRE = 0;

                // -------------------------------------------------------------
                // AQUÍ SE BORRA EL MENSAJE:
                // Si venía de estar despresurizando y ya terminó (o abrieron la puerta),
                // borra la alerta escribiendo el estado normal del sistema.
                // -------------------------------------------------------------
                if (estaba_despresurizando == 1) {
                    DACAI_Set_Text(pagina_actual, 42, "SISTEMA LISTO"); 
                    // Tip: Si prefieres la etiqueta vacía, usa: DACAI_Set_Text(pagina_actual, 42, "");

                    estaba_despresurizando = 0; // Limpia la bandera
                }
            }

            // -----------------------------------------------------------------
            // 2. INICIO DE CICLO
            // -----------------------------------------------------------------
            if (autoclave.flag_iniciar == 1) {
                autoclave.flag_iniciar = 0;

                if (PUERTA == 0) {
                    DACAI_Set_Text(pagina_actual, 35, "ERR: PUERTA ABIERTA");
                    DACAI_Buzzer();
                } 
                else if (sensorCamara > 3) {
                    DACAI_Set_Text(pagina_actual, 35, "ESPERE: DESPRESURIZANDO");
                    DACAI_Buzzer();
                } 
                else {
                    autoclave.contador_prevacios = 0;
                    autoclave.temporizador_etapa_seg = 0;
                    autoclave.estado_actual = ESTADO_PREVACIO;

                    estaba_despresurizando = 0; // Resetea la bandera para el inicio de ciclo
                    DACAI_Set_Text(pagina_actual, 35, "INICIANDO CICLO");
                    //Aqui se agrega
                    Imprimir_Encabezado_Ticket();
                }
            }
            break;

        case ESTADO_PREVACIO:
            //Desactivar botones
            DACAI_Disable_Button(pagina_actual, 4);
            DACAI_Disable_Button(pagina_actual, 3);
            DACAI_Disable_Button(pagina_actual, 11);
            DACAI_Disable_Button(pagina_actual, 36);
            
            
            DACAI_Set_Text(pagina_actual, 35, "Prevacio");

            // --- REGLA DE SEGURIDAD / OMITIR SI ES 0 PREVACÍOS (ej. LÍQUIDOS) ---
            if (autoclave.parametros_activos.num_prevacios == 0) {
                VAL_ESC_RAPIDO_LED = 0;
                VAL_ESC_RAPIDO = 0;
                VAL_SECADO_LED = 0;
                VAL_SECADO = 0;
                VAL_ENTRADA_LED = 0;
                VAL_ENTRADA = 0;
                autoclave.subestado_prevacio = 0;

                Impresora_Imprimir_Lectura("OMITIDO PREVACIO -> CALENTAMIENTO");

                autoclave.estado_actual = ESTADO_CALENTAMIENTO;
                break;
            }

            // --- FASE 1: REALIZAR VACÍO ---
            if (autoclave.subestado_prevacio == 0) 
            {
                VAL_SECADO_LED = 1;   // Abrir válvula de vacío / bomba
                VAL_SECADO = 1;
                VAL_ENTRADA_LED = 0;  // Mantener vapor cerrado
                VAL_ENTRADA = 0;

                // Evaluamos si alcanzó la presión de vacío objetivo
                if (sensorCamara <= -3) { // Reducir aquí al valor objetivo (ej. -8 PSI o 3 PSI de prueba)
                    VAL_SECADO_LED = 0;                     // Cerramos vacío
                    VAL_SECADO = 0;
                    autoclave.subestado_prevacio = 1;   // Pasamos a fase de presurización
                }
            }
            // --- FASE 2: PRESURIZACIÓN DE PREVACÍO (ROMPIMIENTO CON VAPOR A 15 PSI) ---
            else if (autoclave.subestado_prevacio == 1) 
            {
                VAL_SECADO_LED = 0;   // Vacío cerrado
                VAL_SECADO = 0;
                VAL_ENTRADA_LED = 1;  // Inyectar vapor a la cámara
                VAL_ENTRADA = 1;

                // Evaluamos si subió a los 15 PSI del pulso de prevacío
                if (sensorCamara >= 15) {
                    VAL_ENTRADA_LED = 0; // Cerramos entrada de vapor
                    VAL_ENTRADA = 0;

                    // ¡Completamos 1 pulso de prevacío!
                    autoclave.contador_prevacios++; 

                    // Imprimir el pulso completado
                    char msg_pulso[40];
                    sprintf(msg_pulso, "PULSO PREVACIO %d/%d OK", 
                            autoclave.contador_prevacios, 
                            autoclave.parametros_activos.num_prevacios);
                    Impresora_Imprimir_Lectura(msg_pulso);

                    // Verificamos si ya cumplió con todos los prevacíos del programa
                    if (autoclave.contador_prevacios >= autoclave.parametros_activos.num_prevacios) {
                        autoclave.subestado_prevacio = 0; // Limpiamos subestado

                        Impresora_Imprimir_Lectura("FIN PREVACIO -> CALENTAMIENTO");

                        autoclave.estado_actual = ESTADO_CALENTAMIENTO; // Avanza a calentamiento final
                    } else {
                        autoclave.subestado_prevacio = 0; // Reinicia para hacer el siguiente vacío
                    }
                }
            }
            break;
        case ESTADO_CALENTAMIENTO:
            DACAI_Set_Text(pagina_actual, 35, "Calentamiento"); 

            // En calentamiento, abrimos vapor continuo hasta llegar a la temperatura/presión de esterilización
            VAL_ENTRADA_LED = 1;
            VAL_ENTRADA = 1;
            VAL_SECADO_LED = 0;
            VAL_SECADO = 0;

            // Evaluamos si la CÁMARA llegó al setpoint del ciclo activo
            if (sensorTemperatura >= autoclave.parametros_activos.temp_objetivo &&
                sensorCamara >= autoclave.parametros_activos.presion_esterilizado) 
            {
                // Cargar el tiempo de esterilización para la siguiente fase
                autoclave.temporizador_etapa_seg = autoclave.parametros_activos.tiempo_esterilizado;

                // Reiniciar el contador de 2 min para el monitoreo periódico
                contador_segundos_impresion = 0;

                // Registro de impresión de setpoint alcanzado
                Impresora_Imprimir_Lectura("SETPOINT ALCANZADO / INICIO ESTERILIZACION");

                // Avanzar a Esterilización
                autoclave.estado_actual = ESTADO_ESTERILIZACION;
            }
            break;
        case ESTADO_ESTERILIZACION:
            // 1. Mantenemos la válvula de inyección abierta desde la camisa
            DACAI_Set_Text(pagina_actual, 35, "Esterilizacion");
            VAL_ENTRADA_LED = 1;
            VAL_ENTRADA = 1;

            // 2. Formateamos y mostramos el tiempo restante en minutos/segundos en la pantalla Dacai
            uint16_t minutos = autoclave.temporizador_etapa_seg / 60;
            uint16_t segundos = autoclave.temporizador_etapa_seg % 60;
            char buffer_tiempo[16];

            // Variable estática para evitar impresiones repetidas si cae la temperatura
            static uint8_t bandera_alerta_temp = 0;

            // Validación visual si la temperatura cae
            if (sensorTemperatura < autoclave.parametros_activos.temp_objetivo) 
            {
                DACAI_Set_Text(pagina_actual, 35, "Estabilizando Temp...");

                // Imprime en papel la alerta una sola vez cuando cae la temperatura
                if (bandera_alerta_temp == 0) 
                {
                    bandera_alerta_temp = 1;
                    Impresora_Imprimir_Lectura("CAIDA DE TEMP / ESTABILIZANDO");
                }
            } 
            else 
            {
                bandera_alerta_temp = 0; // Se restablece la bandera si la temperatura recupera el setpoint
                sprintf(buffer_tiempo, "%02u:%02u", minutos, segundos);
                DACAI_Set_Text(pagina_actual, 43, buffer_tiempo); // Muestra ej: "04:59"
            }

            // 3. Verificación de finalización de la etapa
            if (autoclave.temporizador_etapa_seg == 0) 
            {
                VAL_ENTRADA_LED = 0; // Cerramos el paso de vapor a la cámara
                VAL_ENTRADA = 0;

                // Impresión del final de la esterilización
                Impresora_Imprimir_Lectura("FIN ESTERILIZACION");

                // Cargar el tiempo para la siguiente etapa (Secado)
                autoclave.temporizador_etapa_seg = autoclave.parametros_activos.tiempo_secado;

                // Avanzamos a la siguiente fase
                autoclave.estado_actual = ESTADO_DESPRESURIZACION;
            }
            break;

        case ESTADO_DESPRESURIZACION:
            DACAI_Set_Text(pagina_actual, 35, "Despresurizacion");
            VAL_ESC_RAPIDO_LED = 1;
            VAL_ESC_RAPIDO = 1;

            if (sensorCamara <= 1) { // Cerca de 1.00 PSI / Atmosférica
                VAL_ESC_RAPIDO_LED = 0;
                VAL_ESC_RAPIDO = 0;

                // Impresión de fin de despresurización
                Impresora_Imprimir_Lectura("FIN DESPRESURIZACION / PRESION SEGURA");

                if (autoclave.parametros_activos.tiempo_secado > 0) {
                    autoclave.temporizador_etapa_seg = autoclave.parametros_activos.tiempo_secado;

                    // Reiniciamos el contador para los registros periódicos de 2 min en Secado
                    contador_segundos_impresion = 0; 

                    Impresora_Imprimir_Lectura("INICIO SECADO");
                    autoclave.estado_actual = ESTADO_SECADO;
                } else {
                    Impresora_Imprimir_Lectura("OMITIDO SECADO -> FIN DE CICLO");
                    autoclave.estado_actual = ESTADO_FIN_CICLO;
                }
            }
            break;

        case ESTADO_SECADO:
            // --- 1. CASO ESPECIAL: CICLO SIN SECADO (ej. LÍQUIDOS con tiempo = 0) ---
            if (autoclave.parametros_activos.tiempo_secado == 0) 
            {
                VAL_ESC_RAPIDO_LED = 0;
                VAL_ESC_RAPIDO = 0;
                VAL_SECADO_LED = 0;
                VAL_SECADO = 0;
                autoclave.flag_vacio_alcanzado = 0; // Limpiar bandera de seguridad

                Impresora_Imprimir_Lectura("OMITIDO SECADO -> IGUALACION AIRE");

                autoclave.estado_actual = ESTADO_IGUALACION_AIRE;
                break;
            }

            // --- 2. ACTIVACIÓN DE VÁLVULAS DE SECADO / VACÍO ---
            VAL_ESC_RAPIDO_LED = 1;
            VAL_ESC_RAPIDO = 1;
            VAL_SECADO_LED = 1;
            VAL_SECADO = 1;

            // Banderas estáticas para evitar impresiones repetitivas en bucle
            static uint8_t bandera_impresion_vacio_ok = 0;
            static uint8_t bandera_alerta_perdio_vacio = 0;

            // --- 3. CONTROL Y VERIFICACIÓN DEL NIVEL DE VACÍO ---
            if (sensorCamara <= -3) 
            {
                // El vacío es correcto: se permite/mantiene el conteo
                if (autoclave.flag_vacio_alcanzado == 0) 
                {
                    autoclave.flag_vacio_alcanzado = 1;

                    // Si es la primera vez que alcanza el vacío objetivo
                    if (bandera_impresion_vacio_ok == 0) 
                    {
                        bandera_impresion_vacio_ok = 1;
                        bandera_alerta_perdio_vacio = 0; // Reset alerta por si se había perdido
                        contador_segundos_impresion = 0; // Reinicia reloj de 2 min para la fase de secado

                        Impresora_Imprimir_Lectura("VACIO ALCANZADO / INICIO CONTEO SECADO");
                    }
                }

                DACAI_Set_Text(pagina_actual, 35, "Secado");
            } 
            else 
            {
                // Aún no alcanza el vacío requerido o perdió el vacío
                if (autoclave.flag_vacio_alcanzado == 0) 
                {
                    autoclave.temporizador_etapa_seg = autoclave.parametros_activos.tiempo_secado;
                    DACAI_Set_Text(pagina_actual, 35, "Haciendo Vacio...");
                } 
                else 
                {
                    // Ya estaba en secado y cayó el nivel de vacío
                    DACAI_Set_Text(pagina_actual, 35, "Secado (Bajo Vacio)");

                    if (bandera_alerta_perdio_vacio == 0) 
                    {
                        bandera_alerta_perdio_vacio = 1;
                        bandera_impresion_vacio_ok = 0; // Permitir que vuelva a registrar cuando se recupere

                        Impresora_Imprimir_Lectura("ADVERTENCIA: BAJO VACIO EN SECADO");
                    }
                }
            }

            // --- 4. MOSTRAR TIEMPO RESTANTE EN PANTALLA DACAI ---
            uint16_t min_secado = autoclave.temporizador_etapa_seg / 60;
            uint16_t seg_secado = autoclave.temporizador_etapa_seg % 60;
            char buffer_tiempo_secado[16];

            sprintf(buffer_tiempo_secado, "%02u:%02u", min_secado, seg_secado);
            DACAI_Set_Text(pagina_actual, 43, buffer_tiempo_secado);

            // --- 5. VERIFICACIÓN DE FINALIZACIÓN DE LA ETAPA ---
            if (autoclave.temporizador_etapa_seg == 0 && autoclave.flag_vacio_alcanzado == 1) 
            {
                // Apagamos válvulas de secado y escape rápido
                VAL_ESC_RAPIDO_LED = 0;
                VAL_ESC_RAPIDO = 0;
                VAL_SECADO_LED = 0;
                VAL_SECADO = 0;

                // Limpiamos banderas de estado e impresión
                autoclave.flag_vacio_alcanzado = 0;
                bandera_impresion_vacio_ok = 0;
                bandera_alerta_perdio_vacio = 0;

                Impresora_Imprimir_Lectura("FIN SECADO");

                // Transición a la etapa de igualación de presión con aire limpio
                autoclave.estado_actual = ESTADO_IGUALACION_AIRE;
            }
            break;
    
        case ESTADO_IGUALACION_AIRE:
            DACAI_Set_Text(pagina_actual, 35, "Entrada aire");
            // Abrir la válvula de admisión/entrada de aire ambiental (con filtro)
            VAL_ENTRADA_AIRE_LED = 1;
            VAL_ENTRADA_AIRE = 1;

            // Esperar a que la cámara regrese a presión atmosférica segura (0 PSI)
            if (sensorCamara >= 0) 
            {
                VAL_ENTRADA_AIRE_LED = 0; // Cierra la válvula de aire
                VAL_ENTRADA_AIRE = 0;

                // Carga de estado final
                autoclave.estado_actual = ESTADO_FIN_CICLO;
            }
            break;

        case ESTADO_FIN_CICLO:
            DACAI_Enable_Button(pagina_actual, 36);
            // --- 1. NOTIFICACIÓN INICIAL EN PANTALLA E IMPRESIÓN DEL TICKET FINAL ---
            if (autoclave.flag_fin_notificado == 0) 
            {
                DACAI_Set_Text(pagina_actual, 35, "Ciclo Finalizado");
                autoclave.flag_fin_notificado = 1;

                // Impresión de lectura final del ciclo
                Impresora_Imprimir_Lectura("CICLO FINALIZADO EXITOSAMENTE");

                // Cierre visual y corte de reporte en el papel
                UART2_Write_Text("================================\r\n");
                UART2_Write_Text("      RESULTADO: EXITOSO        \r\n");
                UART2_Write_Text("================================\r\n\r\n\r\n\r\n"); // Avance de papel para fácil corte
            }

            // --- 2. CONMUTACIÓN DEL BUZZER EXTERNO (Cada 1 segundo) ---
            if (bandera1seg == 1) 
            {
                BUZZER = !BUZZER;  // Alterna: 1 seg ON / 1 seg OFF
            }

            // Se mantiene en este estado esperando que el operador presione OK
            break;

        case ESTADO_MANUAL:
            //DACAI_Set_Text(pagina_actual, 35, "MODO MANUAL");

            // Si el operador navega fuera de la página manual, regresamos a reposo y apagamos actuadores
            if (pagina_actual != 6) {
                VAL_ENTRADA_LED = 0;
                VAL_ENTRADA = 0;
                VAL_ESC_RAPIDO_LED = 0;
                VAL_ESC_RAPIDO = 0;
                VAL_SECADO_LED = 0;
                VAL_SECADO = 0;
                VAL_ENTRADA_AIRE_LED = 0;
                VAL_ENTRADA_AIRE = 0;
                autoclave.estado_actual = ESTADO_REPOSO;
            }
            break;

        case ESTADO_ALARMA:
            // 1. APAGADO DE POTENCIA Y CALENTAMIENTO
            //RESISTENCIAS = 0;                  // Corta calefacción de la camisa/generador
            //sensorCamisaOK = 0;               // Invalida la bandera de presión lista

            // 2. SEGURIDAD DE VÁLVULAS (Cerrar inyecciones, abrir escapes si es seguro)
            VAL_ENTRADA_LED = 0;     // Aísla la camisa de la cámara
            VAL_ENTRADA = 0;
            VAL_SECADO_LED = 0;                    // Apaga bomba o sistema de vacío
            VAL_SECADO = 0;
            VAL_ENTRADA_AIRE_LED = 0;              // Cierra entrada de aire
            VAL_ENTRADA_AIRE = 0;

            /* NOTA DE SEGURIDAD PARA LA PRESIÓN:
               Si la alarma ocurre a alta presión, la cámara debe despresurizarse 
               lentamente por la válvula de escape rápido/alivio */
            /*if (sensorCamara > 2) {          // Si la cámara tiene más de 2.00 PSI
                VAL_ESC_RAPIDO_LED = 1;            // Libera vapor de la cámara por seguridad
             * VAL_ESC_RAPIDO = 1;
            } else {
                VAL_ESC_RAPIDO_LED = 0;            // Cierra cuando sea segura
             * VAL_ESC_RAPIDO = 0;
                VAL_ENTRADA_AIRE_LED = 1;
             * VAL_ENTRADA_AIRE = 1;
            }*/

            // 3. INTERFAZ Y AUDITORÍA
            // Muestra el mensaje de error o detiene contadores
            DACAI_Set_Text(pagina_actual, 35, "Alerta Fin");

            // Si tienes un zumbador/buzzer, lo activas aquí
            // BUZZER = 1; 
            DACAI_Enable_Button(pagina_actual, 3);
            DACAI_Enable_Button(pagina_actual, 4);
            DACAI_Enable_Button(pagina_actual, 1);
            DACAI_Enable_Button(pagina_actual, 36);

            break;
            
            default:
            // 1. Desactivación de emergencia de todos los actuadores de salida
            VAL_ENTRADA_LED      = 0;
            VAL_ENTRADA = 0;
            VAL_ESC_RAPIDO_LED   = 0;
            VAL_ESC_RAPIDO = 0;
            VAL_ESC_LENTO_LED    = 0;
            VAL_SECADO_LED       = 0;
            VAL_SECADO = 0;
            VAL_ENTRADA_AIRE_LED = 0;
            VAL_ENTRADA_AIRE = 0;
            //RESISTENCIAS      = 0;

            // 2. Forzar el regreso al estado de reposo seguro
            autoclave.estado_actual = ESTADO_REPOSO;

            // 3. Notificar en pantalla para depuración
            DACAI_Set_Text(pagina_actual, 35, "ERR: ESTADO DESCONOCIDO");
            break;
    }
}

void Procesar_Boton_Manual(uint8_t control_id) 
{
    // Solo actuamos si el sistema está efectivamente en ESTADO_MANUAL
    if (autoclave.estado_actual != ESTADO_MANUAL) return;

    DACAI_Buzzer(); // Beep de confirmación táctil

    switch (control_id) 
    {
        case BTN_MAN_INICIO: // 0x10
            Ejecutar_Fase_Manual(MANUAL_INICIO);
            DACAI_Set_Text(pagina_actual, 35, "MAN: INICIO");
            break;

        case BTN_MAN_ENTRADA_VAPOR: // 0x11
            // Regla de seguridad de hardware: No permite meter vapor si la puerta está abierta
            if (PUERTA == 0) {
                DACAI_Set_Text(pagina_actual, 35, "ERR: PUERTA ABIERTA");
            } else {
                Ejecutar_Fase_Manual(MANUAL_ENTRADA_VAPOR);
                DACAI_Set_Text(pagina_actual, 35, "MAN: ENTRADA VAPOR");
            }
            break;

        case BTN_MAN_ESCAPE_RAPIDO: // 0x12
            Ejecutar_Fase_Manual(MANUAL_ESCAPE_RAPIDO);
            DACAI_Set_Text(pagina_actual, 35, "MAN: ESC. RAPIDO");
            break;

        case BTN_MAN_ESCAPE_LENTO: // 0x13
            Ejecutar_Fase_Manual(MANUAL_ESCAPE_LENTO);
            DACAI_Set_Text(pagina_actual, 35, "MAN: ESC. LENTO");
            break;

        case BTN_MAN_SECADO: // 0x14
            Ejecutar_Fase_Manual(MANUAL_SECADO);
            DACAI_Set_Text(pagina_actual, 35, "MAN: SECADO/VACIO");
            break;

        case BTN_MAN_ENTRADA_AIRE: // 0x15
            Ejecutar_Fase_Manual(MANUAL_ENTRADA_AIRE);
            DACAI_Set_Text(pagina_actual, 35, "MAN: ENTRADA AIRE");
            break;

        case BTN_MAN_FIN: // 0x16
            Ejecutar_Fase_Manual(MANUAL_FIN); // Apaga todas las válvulas/actuadores
            DACAI_Set_Text(pagina_actual, 35, "MODO MANUAL: IDLE");
            break;

        default:
            break;
    }
}        
        

void Procesar_Boton_OK(void) 
{
    // Solo si el ciclo ya finalizó, el botón OK permite volver a REPOSO
    if (autoclave.estado_actual == ESTADO_FIN_CICLO) 
    {
        autoclave.flag_fin_notificado = 0;   // Limpiar bandera para el próximo ciclo
        autoclave.estado_actual = ESTADO_REPOSO;
        
        // Opcional: Actualizar pantalla Dacai al menú principal / reposo
        DACAI_Set_Text(pagina_actual, 35, "Listo / Reposo");
    }
}

void Ejecutar_Fase_Manual(EstadoManual_t nueva_fase)
{
    // ===================================
    // 1. REGLA DE SEGURIDAD (INTERLOCK): APAGADO GENERAL DE ACTUADORES
    // =========================================================================
    VAL_ENTRADA_LED      = 0;  // Válvula de inyección de vapor a cámara
    VAL_ENTRADA = 0;
    VAL_ESC_RAPIDO_LED   = 0;  // Válvula de despresurización rápida / alivio
    VAL_ESC_RAPIDO = 0;
    VAL_ESC_LENTO_LED    = 0;
    VAL_SECADO_LED       = 0;  // Bomba de vacío / válvula de secado
    VAL_SECADO  = 0;
    VAL_ENTRADA_AIRE_LED = 0;  // Válvula de admisión de aire ambiental
    VAL_ENTRADA_AIRE = 0;

    // =========================================================================
    // 2. ACTIVACIÓN DE SALIDA SEGÚN LA FASE SELECCIONADA
    // =========================================================================
    switch (nueva_fase)
    {
        case MANUAL_INICIO:
            // Posición de reposo/listo: todos los actuadores permanecen apagados.
            break;

        case MANUAL_ENTRADA_VAPOR:
            // Validación de seguridad física: no inyectar vapor si la puerta está abierta
            if (PUERTA == 1) { 
                VAL_ENTRADA_LED = 1; // Inyecta vapor a la cámara desde la camisa
                VAL_ENTRADA = 1;
            }
            break;

        case MANUAL_ESCAPE_RAPIDO:
            VAL_ESC_RAPIDO_LED = 1; // Abre despresurización rápida
            VAL_ESC_RAPIDO = 1;
            break;

        case MANUAL_ESCAPE_LENTO:
            // Si usas una válvula o relé independiente para escape lento:
            // VAL_ESC_LENTO_LED = 1; 
            // Si usas pulsos sobre la misma válvula de escape:
            VAL_ESC_RAPIDO_LED = 1; 
            VAL_ESC_RAPIDO = 1;
            break;

        case MANUAL_SECADO:
            VAL_SECADO_LED = 1; // Enciende bomba/sistema de vacío
            VAL_SECADO = 1;
            break;

        case MANUAL_ENTRADA_AIRE:
            VAL_ENTRADA_AIRE_LED = 1; // Iguala la presión con aire ambiental
            VAL_ENTRADA_AIRE = 1;
            break;

        case MANUAL_FIN:
        default:
            // Finaliza la secuencia manual dejando todo en estado seguro
            break;
    }
}