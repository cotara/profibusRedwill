/*!
 * \file    profibus.h
 * \brief   Ablaufsteuerung Profibus DP-Slave Kommunikation, h-Datei
 * \author  � Joerg S.
 * \date    9.2007 (Erstellung) 9.2010 (Aktueller Stand)
 * \note    Verwendung nur fuer private Zwecke / Only for non-commercial use
 */
#include "usart_user.h"
#include "time_user.h"

#ifndef PROFIBUS_H
#define PROFIBUS_H 1

///////////////////////////////////////////////////////////////////////////////////////////////////
// Ident Nummer DP Slave
///////////////////////////////////////////////////////////////////////////////////////////////////
#define IDENT_HIGH_BYTE       0x80
#define IDENT_LOW_BYTE        0x70
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Adressen
///////////////////////////////////////////////////////////////////////////////////////////////////
//#define MASTER_ADD            2     // SPS Adresse
#define SAP_OFFSET            128   // Service Access Point Adress Offset
#define BROADCAST_ADD         127
#define DEFAULT_ADD           25   // Auslieferungsadresse
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Kommandotypen
///////////////////////////////////////////////////////////////////////////////////////////////////
#define SD1                   0x10  // Telegramm ohne Datenfeld
#define SD2                   0x68  // Daten Telegramm variabel
#define SD3                   0xA2  // Daten Telegramm fest
#define SD4                   0xDC  // Token
#define SC                    0xE5  // Kurzquittung
#define ED                    0x16  // Ende
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Codes
///////////////////////////////////////////////////////////////////////////////////////////////////
/* FC Request */
#define FDL_STATUS            0x09  // SPS: Status Abfrage
#define SRD_HIGH              0x0D  // SPS: Ausgaenge setzen, Eingaenge lesen
#define FCV_                  0x10
#define FCB_                  0x20
#define REQUEST_              0x40

/* FC Response */
#define FDL_STATUS_OK         0x00  // SLA: OK
#define DATA_LOW              0x08  // SLA: (Data low) Отправить ввод данных
#define DATA_HIGH             0x0A  // SLA: (Data high) Ожидание диагностики
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Service Access Points (DP Slave) MS0
///////////////////////////////////////////////////////////////////////////////////////////////////
#define SAP_SET_SLAVE_ADR     55  // Master setzt Slave Adresse, Slave Anwortet mit SC
#define SAP_RD_INP            56  // Master fordert Input Daten, Slave sendet Input Daten
#define SAP_RD_OUTP           57  // Master fordert Output Daten, Slave sendet Output Daten
#define SAP_GLOBAL_CONTROL    58  // Master Control, Slave Antwortet nicht
#define SAP_GET_CFG           59  // Master fordert Konfig., Slave sendet Konfiguration
#define SAP_SLAVE_DIAGNOSIS   60  // Master fordert Diagnose, Slave sendet Diagnose Daten
#define SAP_SET_PRM           61  // Master sendet Parameter, Slave sendet SC
#define SAP_CHK_CFG           62  // Master sendet Konfuguration, Slave sendet SC
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// SAP: Global Control (Daten Master)
///////////////////////////////////////////////////////////////////////////////////////////////////
#define CLEAR_DATA_           0x02
#define UNFREEZE_             0x04
#define FREEZE_               0x08
#define UNSYNC_               0x10
#define SYNC_                 0x20
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// SAP: Diagnose (Antwort Slave)
///////////////////////////////////////////////////////////////////////////////////////////////////
/* Status Byte 1 */

#define STATION_NOT_EXISTENT_ 0x01
#define STATION_NOT_READY_    0x02
#define CFG_FAULT_            0x04
#define EXT_DIAG_             0x08  // Доступна расширенная диагностика
#define NOT_SUPPORTED_        0x10
#define INV_SLAVE_RESPONSE_   0x20
#define PRM_FAULT_            0x40
#define MASTER_LOCK           0x80

/* Status Byte 2 */

#define PRM_REQ_              0x01
#define STAT_DIAG_            0x02
#define STATUS_2_DEFAULT      0x04
#define WD_ON_                0x08
#define FREEZE_MODE_          0x10
#define SYNC_MODE_            0x20
//#define free                  0x40
#define DEACTIVATED_          0x80

/* Status Byte 3 */
#define DIAG_SIZE_OK          0x00
#define DIAG_SIZE_ERROR       0x80

/* Adresse */
#define MASTER_ADD_DEFAULT    0xFF


/* Диагностика ошибок (EXT_DIAG_ = 1) */
#define EXT_DIAG_TYPE_        0xC0  // Бит 6-7 Тип диагностики
#define EXT_DIAG_BYTE_CNT_    0x3F  // Биты 0-5 — это количество диагностических байтов.

#define EXT_DIAG_GERAET       0x00  // Если бит 7 и 6 = 00, то это относится к устройству.
#define EXT_DIAG_KENNUNG      0x40  // Если бит 7 и 6 = 01, то связанные с идентификатором
#define EXT_DIAG_KANAL        0x80  // Если бит 7 и 6 = 01, то связанные с каналом
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// SAP: Set Parameters Request (Daten Master)
///////////////////////////////////////////////////////////////////////////////////////////////////
///* Befehl */
//#define LOCK_SLAVE_           0x80  // Slave fuer andere Master gesperrt
//#define UNLOCK_SLAVE_         0x40  // Slave fuer andere Master freigegeben
//#define ACTIVATE_SYNC_        0x20
//#define ACTIVATE_FREEZE_      0x10
//#define ACTIVATE_WATCHDOG_    0x08
//
///* DPV1 Status Byte 1 */
//#define DPV1_MODE_            0x80
//#define FAIL_SAVE_MODE_       0x40
//#define PUBLISHER_MODE_       0x20
//#define WATCHDOG_TB_1MS       0x04
//
///* DPV1 Status Byte 2 */
//#define PULL_PLUG_ALARM_      0x80
//#define PROZESS_ALARM_        0x40
//#define DIAGNOSE_ALARM_       0x20
//#define VENDOR_ALARM_         0x10
//#define STATUS_ALARM_         0x08
//#define UPDATE_ALARM_         0x04
//#define CHECK_CONFIG_MODE_    0x01
//
///* DPV1 Status Byte 3 */
//#define PARAMETER_CMD_ON_     0x80
//#define ISOCHRON_MODE_ON_     0x10
//#define PARAMETER_BLOCK_      0x08
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// SAP: Check Config Request (Daten Master)
///////////////////////////////////////////////////////////////////////////////////////////////////
#define CFG_DIRECTION_        0x30  // Биты 4-5 - это направление. 01 = вход, 10 = выход, 11 = вход/выход
#define CFG_INPUT             0x10  // Ввод
#define CFG_OUTPUT            0x20  // вывод
#define CFG_INPUT_OUTPUT      0x30  // Ввод/ вывод
#define CFG_SPECIAL           0x00  // Специальный формат, если необходимо передать более 16/32 байт.

#define CFG_WIDTH_            0x40  // Бит 6 — ширина ввода-вывода. 0 = байт (8 бит), 1 = слово (16 бит)
#define CFG_BYTE              0x00  // байт
#define CFG_WORD              0x40  // Слово

/* Специальный формат */
#define CFG_BYTE_CNT_         0x0F  // Биты 0-3 - это количество байтов или слов. 0 = 1 байт, 1 = 2 байта и т. д.

/* Spezielles Format */
#define CFG_SP_DIRECTION_     0xC0  // Биты 6-7 — это направление. 01 = вход, 10 = выход, 11 = вход/выход
#define CFG_SP_VOID           0x00  // пустое место
#define CFG_SP_INPUT          0x40  // Вход
#define CFG_SP_OUTPUT         0x80  // Выход
#define CFG_SP_INPUT_OPTPUT   0xC0  // Вход/Выход

#define CFG_SP_VENDOR_CNT_    0x0F  // Биты 0-3 - это количество байтов, определенных производителем. 0 = нет

/* Специальный формат / длина байта*/
#define CFG_SP_BYTE_CNT_      0x3F  // Биты 0-5 - это количество байтов или слов. 0 = 1 байт, 1 = 2 байта и т. д.
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
#define TIMEOUT_MAX_SYN_TIME  15 // 33 TBit = TSYN                 //33 длины бита надо ждать
#define TIMEOUT_MAX_TX_TIME   15                                   //
#define TIMEOUT_MAX_SDR_TIME  14  // 15 Tbit = TSDR                 //Принимаем данные
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
#define MAX_BUFFER_SIZE       256

#define INPUT_DATA_SIZE       128    // Anzahl Bytes die vom Master kommen
#define OUTPUT_DATA_SIZE      128    // Anzahl Bytes die an Master gehen
#define MODULE_CNT            8     // Anzahl der Module (Ein- Ausgangsmodule) bei modularer Station

#define USER_PARA_SIZE        5     // Anzahl Bytes fuer Herstellerspezifische Parameterdaten
#define EXT_DIAG_DATA_SIZE    0     // Anzahl Bytes fuer erweiterte Diagnose
#define VENDOR_DATA_SIZE      0     // Anzahl Herstellerspezifische Moduldaten
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Profibus Ablaufsteuerung
///////////////////////////////////////////////////////////////////////////////////////////////////
#define PROFIBUS_WAIT_SYN     1
#define PROFIBUS_WAIT_DATA    2
#define PROFIBUS_GET_DATA     3
#define PROFIBUS_SEND_DATA    4
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// ERRORS
///////////////////////////////////////////////////////////////////////////////////////////////////
#define DEST_ERROR             0x8F
#define CRC_ERROR              0x8E
#define SIZE_ERROR             0x8D
#define UNKNOWN_SAP            0x8C
#define UNKNOWN_MODULE         0x8B
   
//#define DELAY_TIMER_1_7MS   0x035C  // 33 TB (UART @ 19200)
//#define DELAY_TIMER_782US   0x0187  // 15 TB (UART @ 19200)
//#define DELAY_TBIT          26.04   // UART @ 19200

//#define DELAY_TIMER_1_7MS   0x00B1  // 33 TB (UART @ 93750)
//#define DELAY_TIMER_782US   0x0051  // 15 TB (UART @ 93750)
//Если таймер 500 кГц, то цикл 2мкс
//93750 бод/с - это 10.66 мкс/бит
//Тогда TBIT = 10.66/2 = 5.33 циклов(тиков) на бит
   
//Если таймер 12 Мгц, то цикл 0,0833 мкс

//9600 бод/с - это 104.17 мкс/бит
//Тогда TBIT = 104.17/0,0833 = 1250 циклов(тиков) на бит

//19200 бод/с - это 52.08 мкс/бит
//Тогда TBIT = 52.08/0,0833 = 625 циклов(тиков) на бит

//45450 бод/с - это 22 мкс/бит
//Тогда TBIT = 22/0,0833 = 264 циклов(тиков) на бит

//93750 бод/с - это 10.66 мкс/бит
//Тогда TBIT = 10.66/0,0833 =  128 циклов(тиков) на бит
   
//500000 бод/с - это 2 мкс/бит
//Тогда TBIT = 2/0,0833 = 24 циклов(тиков) на бит
   
//1000000 бод/с - это 1 мкс/бит
//Тогда TBIT = 1/0,0833 = 12 циклов(тиков) на бит 

//1500000 бод/с - это 0,667 мкс/бит
//Тогда TBIT = 0,667/0,0833 = 8 циклов(тиков) на бит 

//1500000 бод/с - это 0,333 мкс/бит
//Тогда TBIT = 0,333/0,0833 = 8 циклов(тиков) на бит 



#define DELAY_TBIT3000            4    // UART @ 3000000 
#define DELAY_TBIT1500            8    // UART @ 1500000    
#define DELAY_TBIT500            24    // UART @ 500000      
#define DELAY_TBIT187_5            64    // UART @ 187500
#define DELAY_TBIT93_75            128    // UART @ 93750                           //Количество тиков таймера на бит
#define DELAY_TBIT45_45            264    // UART @ 187500
#define DELAY_TBIT19_2            625    // UART @ 187500
#define DELAY_TBIT9_6            1250    // UART @ 9600

#define DELAY_TBIT DELAY_TBIT9_6
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum {NULL_device = 0, ID_device = 1, ZASI_device = 2,LDM_device = 3} DeviceModel;
///////////////////////////////////////////////////////////////////////////////////////////////////
void init_Profibus (uint32_t baud);
void profibusSetAddress(uint8_t add);
void profibus_RX(void);
void profibus_send_CMD (uint8_t  type, uint8_t  function_code,  uint8_t  sap_offset, uint8_t *pdu, uint8_t  length_pdu);
void  profibus_TX (uint8_t *data, uint8_t  length);

uint8_t checksum(uint8_t *data, uint8_t length);
uint8_t  addmatch(uint8_t  destination);


///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
