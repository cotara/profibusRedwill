/*!
 * \file    profibus.c
 * \brief   Ablaufsteuerung Profibus DP-Slave Kommunikation
 * \author  � Joerg S.
 * \date    9.2007 (Erstellung) 9.2010 (Aktueller Stand)
 * \note    Verwendung nur fuer private Zwecke / Only for non-commercial use
 */
/*
Изменения 2: 
- Оптимизировано прерывание RX. 
- Адрес подчиненного устройства проверяется перед вычислением контрольной суммы 
- Прерывание TX больше не ждет, пока будет отправлен последний байт 
- Переключение с TX на RX (приемопередатчик RS485) теперь через таймер 
(PROFIBUS_SEND_DATA) 
- Таймер остановки прерывания по таймеру на время обработки 
- Вложенность прерываний возможно прерывание по таймеру. 

Все изменения сокращают время обработки для 
интерфейса Profibus и предоставляют другим приложениям более быстрый и быстрый 
доступ к ЦП.

Изменения4: 
диагностический_статус больше не используется в качестве флага, но теперь используется непосредственно 
как байт состояния 1 в диагностике. Поэтому переименовав в dialog_status_1. 
Процесс диагностики также был пересмотрен, так как ошибка приводила к отправке неправильного количества байтов диагностики. 
В настоящее время программное обеспечение использует диагностику, связанную с устройством. Два других 
варианта диагностики еще не тестировались. 
Определение DIAGNOSE было переименовано в DATA_HIGH, чтобы соответствовать 
документации Felser. 
Примечание к процедуре: Если диагностические данные передаются, ПЛК удаляет 
диагностический статус только тогда, когда с установленным диагностическим битом (diagnose_status_1 &
EXT_DIAG_) были переданы пустые диагностические данные. Если вы удалите EXT_DIAG_ 
вместе с диагностическими данными, ПЛК не распознает удаленный диагностический статус. 


В случае «Запроса параметров» более корректным является термин Пользовательский параметр. 
Поэтому определение VENDOR_DATA_SIZE переименовывается в USER_PARA_SIZE. 
Внимание: имя VENDOR_DATA_SIZE будет по-прежнему использоваться (с запросом конфигурации 
). 


Как уже сообщалось, размер входных/выходных данных будет пересчитан. 
Для этого используется новый двумерный массив Module_Data_size[][]. 
ПЛК может задавать не только одну входную и выходную переменную, но и разные модули. Особенно со «спец.
Формат». 
Количество модулей определяется с помощью MODULE_CNT. Количество входных и выходных байтов 
хранится в двух байтах на модуль в массиве . В случае ошибки автоматически 
устанавливается соответствующий флаг (CFG_FAULT_). в диагностике Весь раздел «Проверить запрос конфигурации» был переработан в этом отношении.
*/
#include "profibus4.h"
#include "spi_user.h"

extern uint8_t uart_buffer[BUFFER_SIZE];
extern uint16_t rx_index, tx_index,tx_counter;


// Profibus флаги и переменные
uint8_t profibus_status;
uint8_t diagnose_status_1;
uint8_t slave_addr;
uint8_t master_addr;
uint8_t group;
uint8_t inputSize,outputSize;
uint8_t data_out_register[OUTPUT_DATA_SIZE]={0xa4,0x70,0x45,0x41,5,6,7,8,9,10,11,12,13,14,15,16};
uint8_t data_in_register [INPUT_DATA_SIZE]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
uint32_t errorCount = 0;

#if (USER_PARA_SIZE > 0)
uint8_t User_Para[USER_PARA_SIZE];
#endif
#if (EXT_DIAG_DATA_SIZE > 0)
uint8_t Diag_Data[EXT_DIAG_DATA_SIZE];
#endif
#if (VENDOR_DATA_SIZE > 0)
uint8_t Vendor_Data[VENDOR_DATA_SIZE];
#endif
uint8_t User_Para_size;
//unsigned char Input_Data_size;
//unsigned char Output_Data_size;
uint8_t Module_cnt;
uint8_t Module_Data_size[MODULE_CNT][2];                                        // [][0] = количество входов, [][1] = количество выходов
uint8_t Vendor_Data_size;                                                       // Количество прочитанных байтов производителя

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Profibus инициализация таймеров и переменных
 */
void init_Profibus (void)
{
  uint8_t cnt;
  
  profibus_status = PROFIBUS_WAIT_SYN;                                          // Инициализация переменных
  diagnose_status_1 = STATION_NOT_READY_;
  User_Para_size = 0;
  Vendor_Data_size = 0;
  group = 0;
  
  //slave_addr = get_Address();                                                 // Чтение slave-адреса из EEPROM
  if((slave_addr == 0) || (slave_addr > 126))                                   // Не разрешать недействительные адреса
    slave_addr = DEFAULT_ADD;
  
//  for (cnt = 0; cnt < OUTPUT_DATA_SIZE; cnt++)                                  // Очистка регистров данных{
//    data_out_register[cnt] = 0xFF;
//
//  for (cnt = 0; cnt < INPUT_DATA_SIZE; cnt++)
//    data_in_register[cnt] = 0x00;


  #if (USER_PARA_SIZE > 0)
    for (cnt = 0; cnt < USER_PARA_SIZE; cnt++){
      User_Para[cnt] = 0x00;
    }
  #endif
  #if (DIAG_DATA_SIZE > 0)
    for (cnt = 0; cnt < DIAG_DATA_SIZE; cnt++){
      Diag_Data[cnt] = 0x00;
    }
  #endif
  
  // Инициализация таймера
  timers_init((uint32_t)(TIMEOUT_MAX_SYN_TIME));
  TIM_Cmd(TIM3,ENABLE); 
}

///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Profibus обработка принятого пакета
 */
void profibus_RX (void)
{
  uint8_t cnt;
  uint8_t telegramm_type;
  uint8_t process_data;

  // Profibus Datentypen
  uint8_t destination_add;
  uint8_t source_add;
  uint8_t function_code;
  uint8_t FCS_data;                                                             // Frame Check Sequence (последовательность проверки кадра) 
  uint8_t PDU_size;                                                             // PDU размер
  uint8_t DSAP_data;                                                            // SAP получателя
  uint8_t SSAP_data;                                                            // SAP отправителя

  process_data = 0;                                                             //Статус обработки пакета
  telegramm_type = uart_buffer[0];

  switch (telegramm_type)
  {
    case SD1:                                                                   // Телеграмма без данных, не более 6 байт
        if (rx_index != 6) break;

        destination_add = uart_buffer[1];
        source_add      = uart_buffer[2];
        function_code   = uart_buffer[3];
        FCS_data        = uart_buffer[4];

        if (addmatch(destination_add)       == 0) {spiSendByte(DEST_ERROR);break;}
        if (checksum(&uart_buffer[1], 3) != FCS_data) {spiSendByte(CRC_ERROR);break;}

        process_data = 1;
        break;

    case SD2:                                                                   // Телеграмма с переменной длиной данных
        if (rx_index != uart_buffer[1] + 6) {spiSendByte(rx_index);break;}

        PDU_size        = uart_buffer[1];                                       // DestA+SourseA+FuCode+Полезные данные
        destination_add = uart_buffer[4];
        source_add      = uart_buffer[5];
        function_code   = uart_buffer[6];
        FCS_data        = uart_buffer[PDU_size + 4];
        
        if (addmatch(destination_add)              == 0) {spiSendByte(DEST_ERROR);break;}
        if (checksum(&uart_buffer[4], PDU_size) != FCS_data) {spiSendByte(CRC_ERROR);break;}

        process_data = 1;
        break;

    case SD3:                                                                   // Телеграмма с 5 байтами данных, максимум 11 байт
        if (rx_index != 11) break;

        PDU_size        = 8;                                                    // DestA+SourseA+FuCode+Полезные данные
        destination_add = uart_buffer[1];
        source_add      = uart_buffer[2];
        function_code   = uart_buffer[3];
        FCS_data        = uart_buffer[9];

        if (addmatch(destination_add)       == 0) {spiSendByte(DEST_ERROR);break;}
        if (checksum(&uart_buffer[1], 8) != FCS_data) {spiSendByte(CRC_ERROR);break;}

        process_data = 1;
        break;

    case SD4:                                                                   // Токен с 3 байтами данных
        if (rx_index != 3) break;

        destination_add = uart_buffer[1];
        source_add      = uart_buffer[2];
      
        if (addmatch(destination_add)       == 0) {spiSendByte(DEST_ERROR);break;}
        
        break;
        
    default:
        spiSendByte(telegramm_type);
        errorCount++;
        break;
  } 

  if (process_data == 1)                                                        // Продолджаем только если данные в порядке
  {     
    master_addr = source_add;                                                   // Master Adress это адрес отправителя
    if ((destination_add & 0x80) && (source_add & 0x80))                        // Service Access Point (SAP) обнаружен?
    {
      DSAP_data = uart_buffer[7];
      SSAP_data = uart_buffer[8];

      // Процесс перезагрузки:                                                  // См. Felser 8/2009 Kap. 4.1
      // 1) SSAP 62 -> DSAP 60 (Запрос диагностики)
      // 2) SSAP 62 -> DSAP 61 (Запрос параметризации)
      // 3) SSAP 62 -> DSAP 62 (Запрос проверки конфигурации)
      // 4) SSAP 62 -> DSAP 60 (Запрос диагностики)
      // 5) Запрос на обмен данными (нормальный цикл)
      
      switch (DSAP_data)
      {
        case SAP_SET_SLAVE_ADR:                                                 // Установка адреса ведомого (Slave Address) (SSAP 62 -> DSAP 55)// Siehe Felser 8/2009 Kap. 4.2
            profibus_send_CMD(SC, 0, SAP_OFFSET, &uart_buffer[0], 0);           
            break;

        case SAP_GLOBAL_CONTROL:                                                // Global Control Request (SSAP 62 -> DSAP 58) // Siehe Felser 8/2009 Kap. 4.6.2
          if (uart_buffer[9] & CLEAR_DATA_){ /*LED_ERROR_AN;*/ }                  // Если значение "Clear Data" высокое, тогда SPS CPU перевести в состояние "Stop"                                                                              
          else{}//LED_ERROR_AUS;                                                
                                                                  
            for (cnt = 0;  uart_buffer[10] != 0; cnt++) uart_buffer[10]>>=1;    // Рассчёт группы
      
            if (cnt == group){                                                  // Если команда предназначалась нашей группе
              if (uart_buffer[9] & UNFREEZE_){}                                 // выключить состояние FREEZE
              else if (uart_buffer[9] & UNSYNC_){}                              // выключить состояние SYNC 
              else if (uart_buffer[9] & FREEZE_){}                              // Включить состояние FREEZE. Не читать входные данные
              else if (uart_buffer[9] & SYNC_){}                                // Включить состояние SYNC. Не менять выходы до следующей команды SYNC
            }
            break;

        case SAP_SLAVE_DIAGNOSIS:                                               // Запрос диагностики (SSAP 62 -> DSAP 60)// Siehe Felser 8/2009 Kap. 4.5.2          
             // После получения запроса диагностики ведомый DP меняет состояние
             // "Power on Reset" (POR) на состояние "Wait Parameter" (WPRM)

             // В конце инициализации (состояние "Обмен данными" (DXCHG))
             // мастер повторно отправляет запрос на диагностику
             // для проверки правильности конфигурации

            if (function_code == (REQUEST_ + FCB_ + SRD_HIGH)){                 // Первый диагностический запрос
              uart_buffer[7]  = SSAP_data;                                      // Целевой SAP мастера
              uart_buffer[8]  = DSAP_data;                                      // Целевой SAP Ведомого
              uart_buffer[9]  = diagnose_status_1;                              // Status 1
              uart_buffer[10] = STATUS_2_DEFAULT + PRM_REQ_;                    // Status 2
              uart_buffer[11] = DIAG_SIZE_OK;                                   // Status 3
              uart_buffer[12] = MASTER_ADD_DEFAULT;                             // Адрес мастера
              uart_buffer[13] = IDENT_HIGH_BYTE;                                // Байт идентификации high
              uart_buffer[14] = IDENT_LOW_BYTE;                                 // Байт идентификации low
              #if (EXT_DIAG_DATA_SIZE > 0)
                uart_buffer[15] = EXT_DIAG_GERAET+EXT_DIAG_DATA_SIZE+1;           // Заголовок диагностики (тип и количество байт)
                for (cnt = 0; cnt < EXT_DIAG_DATA_SIZE; cnt++)                    //Данные диагностики
                  uart_buffer[16+cnt] = Diag_Data[cnt];

                profibus_send_CMD(SD2, DATA_LOW, SAP_OFFSET, &uart_buffer[7], 9 + EXT_DIAG_DATA_SIZE);
              #else
                profibus_send_CMD(SD2, DATA_LOW, SAP_OFFSET, &uart_buffer[7], 8);
              #endif
            }
            
            else if (function_code == (REQUEST_ + FCV_ + SRD_HIGH) ||           // Диагностический запрос для проверки данных из запроса Check Config
                     function_code == (REQUEST_ + FCV_ + FCB_ + SRD_HIGH))      // Диагностический запрос, когда ведомое устройство запрашивает диагностический запрос
            {
              uart_buffer[7]  = SSAP_data;                    // Целевой SAP мастера
              uart_buffer[8]  = DSAP_data;                    // Целевой SAP Ведомого
              uart_buffer[9]  = diagnose_status_1;            // Status 1
              uart_buffer[10] = STATUS_2_DEFAULT;             // Status 2
              uart_buffer[11] = DIAG_SIZE_OK;                 // Status 3
              uart_buffer[12] = master_addr - SAP_OFFSET;     // Адрес мастера
              uart_buffer[13] = IDENT_HIGH_BYTE;              // Байт идентификации high
              uart_buffer[14] = IDENT_LOW_BYTE;               // Байт идентификации low
              #if (EXT_DIAG_DATA_SIZE > 0)
                uart_buffer[15] = EXT_DIAG_GERAET+EXT_DIAG_DATA_SIZE+1; // Diagnose (Typ und Anzahl Bytes)
                for (cnt = 0; cnt < EXT_DIAG_DATA_SIZE; cnt++)
                  uart_buffer[16+cnt] = Diag_Data[cnt];
                
                profibus_send_CMD(SD2, DATA_LOW, SAP_OFFSET, &uart_buffer[7], 9 + EXT_DIAG_DATA_SIZE);
              #else
                profibus_send_CMD(SD2, DATA_LOW, SAP_OFFSET, &uart_buffer[7], 8);
              #endif
            }

            break;

        case SAP_SET_PRM:                                                       // Запрос установки параметров (SSAP 62 -> DSAP 61)// Siehe Felser 8/2009 Kap. 4.3.1
            // После получения параметров подчиненное устройство DP переходит из состояния «Ожидание параметра» (WPRM) в состояние «Ожидание конфигурации» (WCFG)         
            if ((uart_buffer[13] == IDENT_HIGH_BYTE) && (uart_buffer[14] == IDENT_LOW_BYTE)){// Принимаем данные только для нашего устройства
              User_Para_size = PDU_size - 12;                                   // Размер параметра пользователя  = длина - DA, SA, FC, DSAP, SSAP и 7 байтов параметра
              #if (USER_PARA_SIZE > 0)                                          // Чтение пользовательских параметров
                for (cnt = 0; cnt < User_Para_size; cnt++) User_Para[cnt] = uart_buffer[16+cnt];
              #endif
             
              for (group = 0; uart_buffer[15] != 0; group++) uart_buffer[15]>>=1;       // Чтение группы

              profibus_send_CMD(SC, 0, SAP_OFFSET, &uart_buffer[0], 0);
            }
            break;

        case SAP_CHK_CFG:                                                       // Проверка запроса конфигурации (SSAP 62 -> DSAP 62) // Siehe Felser 8/2009 Kap. 4.4.1
            // После получения конфигурации ведомое устройство DP переходит из состояния «Ожидание конфигурации» (WCFG) в состояние «Обмен данными» (DXCHG)
            // Конфигурация ввода-вывода:
           // Компактный формат для ввода/вывода не более 16/32 байт
           // Специальный формат для максимальных 64/132 байт ввода-вывода
            Module_cnt = 0;
            Vendor_Data_size = 0;
          
            // Оценка нескольких байтов в зависимости от размера данных PDU
            // LE/LEr - (DA+SA+FC+DSAP+SSAP) = количество байтов конфигурации
            for (cnt = 0; cnt < uart_buffer[1] - 5; cnt++)
            {
              switch (uart_buffer[9+cnt] & CFG_DIRECTION_)                      //Настройки направления ввода-вывода
              {
                case CFG_INPUT:
                    Module_Data_size[Module_cnt][0] = (uart_buffer[9+cnt] & CFG_BYTE_CNT_) + 1;
                    if (uart_buffer[9+cnt] & CFG_WIDTH_ & CFG_WORD)
                      Module_Data_size[Module_cnt][0] = Module_Data_size[Module_cnt][0]*2;
                      
                    Module_cnt++;
                    break;
  
                case CFG_OUTPUT:
                    Module_Data_size[Module_cnt][1] = (uart_buffer[9+cnt] & CFG_BYTE_CNT_) + 1;
                    if (uart_buffer[9+cnt] & CFG_WIDTH_ & CFG_WORD)
                      Module_Data_size[Module_cnt][1] = Module_Data_size[Module_cnt][1]*2;
                    
                    Module_cnt++;
                    break;
  
                case CFG_INPUT_OUTPUT:
                    Module_Data_size[Module_cnt][0] = (uart_buffer[9+cnt] & CFG_BYTE_CNT_) + 1;
                    Module_Data_size[Module_cnt][1] = (uart_buffer[9+cnt] & CFG_BYTE_CNT_) + 1;
                    if (uart_buffer[9+cnt] & CFG_WIDTH_ & CFG_WORD){
                      Module_Data_size[Module_cnt][0] = Module_Data_size[Module_cnt][0]*2;
                      Module_Data_size[Module_cnt][1] = Module_Data_size[Module_cnt][1]*2;
                    }
                    
                    Module_cnt++;
                    break;
  
                case CFG_SPECIAL:                                               // Доп настройки, которые не влезли в 16 бит . Специальный формат                   
                    if (uart_buffer[9+cnt] & CFG_SP_VENDOR_CNT_){               // Доступны байты, специфичные для производителя?
                      Vendor_Data_size += uart_buffer[9+cnt] & CFG_SP_VENDOR_CNT_;// Сохраняем количество данных производителя                    
                      uart_buffer[1] -= uart_buffer[9+cnt] & CFG_SP_VENDOR_CNT_;// Вычесть количество из общего количества
                    }
                    switch (uart_buffer[9+cnt] & CFG_SP_DIRECTION_)             // данные ввода-вывода
                    {
                      case CFG_SP_VOID:                                         // Пустой модуль (1 байт)
                          Module_Data_size[Module_cnt][0] = 0;
                          Module_Data_size[Module_cnt][1] = 0;
                          Module_cnt++;
                          break;
  
                      case CFG_SP_INPUT:                                        // ввод (2 байта)                          
                          Module_Data_size[Module_cnt][0] = (uart_buffer[10+cnt] & CFG_SP_BYTE_CNT_) + 1;
                          if (uart_buffer[10+cnt] & CFG_WIDTH_ & CFG_WORD)
                            Module_Data_size[Module_cnt][0] = Module_Data_size[Module_cnt][0]*2;
                          
                          Module_cnt++;
                          cnt++;                                                // У нас уже есть второй байт
                          break;
  
                      case CFG_SP_OUTPUT:                                       // вывод (2 байта)
                          Module_Data_size[Module_cnt][1] = (uart_buffer[10+cnt] & CFG_SP_BYTE_CNT_) + 1;
                          if (uart_buffer[10+cnt] & CFG_WIDTH_ & CFG_WORD)
                            Module_Data_size[Module_cnt][1] = Module_Data_size[Module_cnt][1]*2;
                          
                          Module_cnt++;
                          cnt++;                                                // У нас уже есть второй байт
                          break;
  
                      case CFG_SP_INPUT_OPTPUT:                                 // ввод и вывод (3 байта)
                          // первый выход...
                          Module_Data_size[Module_cnt][0] = (uart_buffer[10+cnt] & CFG_SP_BYTE_CNT_) + 1;
                          if (uart_buffer[10+cnt] & CFG_WIDTH_ & CFG_WORD)
                            Module_Data_size[Module_cnt][0] = Module_Data_size[Module_cnt][0]*2;
                          
                          // затем вход
                          Module_Data_size[Module_cnt][1] = (uart_buffer[11+cnt] & CFG_SP_BYTE_CNT_) + 1;
                          if (uart_buffer[11+cnt] & CFG_WIDTH_ & CFG_WORD)
                            Module_Data_size[Module_cnt][1] = Module_Data_size[Module_cnt][1]*2;
                          
                          Module_cnt++;
                          cnt += 2;                                             // У нас уже есть второй и третий байты
                          
                          break;
                    }
                    break;
  
                default:
                    spiSendByte(UNKNOWN_MODULE); 
                    break;
  
              } // Switch End
              
            } // For End
            //Расчёт размеров входных и выходных регистров
            inputSize = 0;
            outputSize = 0;
            for(int i=0;i<Module_cnt;i++){
              inputSize+=Module_Data_size[i][1];
              outputSize+=Module_Data_size[i][0];
            }
            if (Vendor_Data_size != 0) {/* обработка*/}
                
            #if (VENDOR_DATA_SIZE > 0)                                          // В случае ошибки -> отправить CFG_FAULT_ в диагностике
              if (Module_cnt > MODULE_CNT || Vendor_Data_size != VENDOR_DATA_SIZE)
                diagnose_status_1 |= CFG_FAULT_;
            #else
              if (Module_cnt > MODULE_CNT)
                diagnose_status_1 |= CFG_FAULT_;
            #endif
            
              else
              diagnose_status_1 &= ~(STATION_NOT_READY_ + CFG_FAULT_);
            
            
            // Короткие ответ (квитирование) 
            profibus_send_CMD(SC, 0, SAP_OFFSET, &uart_buffer[0], 0);

            break;

        default:                                                                // Неизвестный SAP
          spiSendByte(UNKNOWN_SAP);  
          break;

      } // Switch DSAP_data End

    }
    
    else if (destination_add == slave_addr) {                                   // Цель: Slave Adresse    
      if (function_code == (REQUEST_ + FDL_STATUS))                             // Если пришёл запрос состояние
        profibus_send_CMD(SD1, FDL_STATUS_OK, 0, &uart_buffer[0], 0);

      else if (function_code == (REQUEST_ + FCV_ + SRD_HIGH) ||                 // Если Master отправляет выходные данные и запрашивает входные данные (Send and Request Data)
               function_code == (REQUEST_ + FCV_ + FCB_ + SRD_HIGH))
      {

        for (cnt = 0; cnt < inputSize; cnt++)
          data_in_register[cnt] = uart_buffer[cnt + 7];
        for (cnt = 0; cnt < outputSize; cnt++)
          uart_buffer[cnt + 7] = data_out_register[cnt];
        
        if (diagnose_status_1 & EXT_DIAG_)                                      //Если требуется внешняя диагностика
            profibus_send_CMD(SD2, DATA_HIGH, 0, &uart_buffer[7], 0);           // Запросить диагностический запрос
        else
            profibus_send_CMD(SD2, DATA_LOW, 0, &uart_buffer[7], outputSize);            // Отправляем данные
          
          
//        for (cnt = 0; cnt < INPUT_DATA_SIZE; cnt++)                             // Чтение данных от мастера
//          data_in_register[cnt] = uart_buffer[cnt + 7];
//               
//        for (cnt = 0; cnt < OUTPUT_DATA_SIZE; cnt++)                            // Записываем данные для мастера в буфер
//          uart_buffer[cnt + 7] = data_out_register[cnt];
//
//          
//        #if (OUTPUT_DATA_SIZE > 0)
//          if (diagnose_status_1 & EXT_DIAG_)                                    //Если требуется внешняя диагностика
//            profibus_send_CMD(SD2, DATA_HIGH, 0, &uart_buffer[7], 0);           // Запросить диагностический запрос
//          else
//            profibus_send_CMD(SD2, DATA_LOW, 0, &uart_buffer[7], 0);            // Отправляем данные
//        #else
//          if (diagnose_status_1 & EXT_DIAG_ || (DEFAULT_ADD & 0x80))
//            profibus_send_CMD(SD1, DATA_HIGH, 0, &uart_buffer[7], 0);           // Запросить диагностический запрос
//          else        
//            profibus_send_CMD(SC, 0, 0, &uart_buffer[7], 0);                    // Короткий ответ (квитирование)
//        #endif
      }
    }
    else
      spiSendByte(destination_add);
      errorCount++;

  } 

}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief сборка и отправка пакета Profibus
 * \param type          Тип пакета (SD1, SD2 usw.)
 * \param function_code Передаваемый код функции
 * \param sap_offset    Значение смещения SAP
 * \param pdu           Указатель на поле данных (PDU)
 * \param length_pdu    Длина чистого PDU без DA, SA или FC
 */
void profibus_send_CMD (uint8_t type,uint8_t function_code,uint8_t sap_offset,uint8_t *pdu,uint8_t length_pdu){
  uint8_t length_data;   
  switch(type)
  {
    case SD1:
      uart_buffer[0] = SD1;
      uart_buffer[1] = master_addr;
      uart_buffer[2] = slave_addr + sap_offset;
      uart_buffer[3] = function_code;
      uart_buffer[4] = checksum(&uart_buffer[1], 3);
      uart_buffer[5] = ED;

      length_data = 6;
      break;

    case SD2:
      uart_buffer[0] = SD2;
      uart_buffer[1] = length_pdu + 3;                                          // Длина PDU, включая DA, SA и FC
      uart_buffer[2] = length_pdu + 3;
      uart_buffer[3] = SD2;
      uart_buffer[4] = master_addr;
      uart_buffer[5] = slave_addr + sap_offset;
      uart_buffer[6] = function_code;    
      // Данные уже заполнены до вызова функции
      uart_buffer[7+length_pdu] = checksum(&uart_buffer[4], length_pdu + 3);
      uart_buffer[8+length_pdu] = ED;

      length_data = length_pdu + 9;
      break;

    case SD3:

      uart_buffer[0] = SD3;
      uart_buffer[1] = master_addr;
      uart_buffer[2] = slave_addr + sap_offset;
      uart_buffer[3] = function_code;
      // Данные уже заполнены до вызова функции
      uart_buffer[9] = checksum(&uart_buffer[4], 8);
      uart_buffer[10] = ED;

      length_data = 11;
      break;

    case SD4:

      uart_buffer[0] = SD4;
      uart_buffer[1] = master_addr;
      uart_buffer[2] = slave_addr + sap_offset;

      length_data = 3;
      break;

    case SC:

      uart_buffer[0] = SC;

      length_data = 1;
      break;

    default:
      break;

  }
  
  profibus_TX(&uart_buffer[0], length_data);

}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Telegramm senden
 * \param data    Pointer auf Datenfeld
 * \param length  Laenge der Daten
 */
void profibus_TX (uint8_t *data, uint8_t length)
{
    profibus_status = PROFIBUS_SEND_DATA;
    
    timers_init((uint32_t)(TIMEOUT_MAX_TX_TIME));                           // Инициализация таймера

    TIM_Cmd(TIM3,ENABLE);
    GPIO_SetBits(GPIOD,GPIO_Pin_4);
    GPIO_ResetBits(GPIOD,GPIO_Pin_0);
    GPIO_ResetBits(GPIOD,GPIO_Pin_1);
    GPIO_ResetBits(GPIOD,GPIO_Pin_2); 
    USART2_put_string(data,length);                                           //Загружаем строку в буфер на отправку
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Вычислить контрольную сумму. Простое добавление всех байтов данных в телеграмму.
 * \param data   Указатель на данные
 * \param length  Длина данных
 * \return Checksumme
 */
uint8_t checksum(uint8_t *data, uint8_t length){
  uint8_t csum = 0;

  while(length--)    csum += data[length];

  return csum;
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Проверяет адрес назначения. Адрес должен совпадать с подчиненным адресом или широковещательным адресом (включая смещение SAP).
 * \param адрес назначения
 * \return TRUE, если адрес назначения наш, FALSE, если нет
 */
uint8_t addmatch (uint8_t destination)
{
  if ((destination != slave_addr) &&                // Slave
      (destination != slave_addr + SAP_OFFSET) &&   // SAP Slave
      (destination != BROADCAST_ADD) &&             // Broadcast
      (destination != BROADCAST_ADD + SAP_OFFSET))  // SAP Broadcast
    return 0;
  
  return 1;
}
