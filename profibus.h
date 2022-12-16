/*!
 * \file    profibus.h
 * \brief   Ablaufsteuerung Profibus DP-Slave Kommunikation, h-Datei
 * \author  © Joerg S.
 * \date    9.2007 (Erstellung) 7.2008 (Aktueller Stand)
 * \note    Verwendung nur fuer private Zwecke / Only for non-commercial use
 */

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
//#define MASTER_ADD            0x02  // SPS Adresse
#define BROADCAST_ADD         0x7F
#define SAP_OFFSET            0x80  // Service Access Point Adress Offset
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
#define REQ_FDL_STATUS        0x49  // SPS: Status Abfrage
#define FDL_STATUS_OK         0x00  // SLA: OK
//#define SEND_DATA
#define SEND_REQ_DATA         0x5D  // SPS: Ausgaenge setzen, Eingaenge lesen
#define SEND_REQ_DATA_T       0x7D  // SPS: Ausgaenge setzen, Eingaenge lesen (Toggle)
#define SEND_REQ_DATA_FIRST   0x6D  // SPS: Erste Anfrage
#define SLAVE_DATA            0x08  // SLA: Daten Eingaenge senden
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
#define EXT_DIAG_             0x08  // Erweiterte Diagnose vorhanden
#define NOT_SUPPORTED_        0x10
#define INV_SLAVE_RESPONSE_   0x20
#define PRM_FAULT_            0x40
#define MASTER_LOCK           0x80

/* Status Byte 2 */
#define STATUS_2_DEFAULT      0x04
#define PRM_REQ_              0x01
#define STAT_DIAG_            0x02
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


/* Erweiterte Diagnose (EXT_DIAG_ = 1) */
#define EXT_DIAG_TYPE_        0xC0  // Bit 6-7 ist Diagnose Typ
#define EXT_DIAG_GERAET       0x00  // Wenn Bit 7 und 6 = 00, dann Geraetebezogen
#define EXT_DIAG_KENNUNG      0x40  // Wenn Bit 7 und 6 = 01, dann Kennungsbezogen
#define EXT_DIAG_KANAL        0x80  // Wenn Bit 7 und 6 = 10, dann Kanalbezogen

#define EXT_DIAG_BYTE_CNT_    0x3F  // Bit 0-5 sind Anzahl der Diagnose Bytes
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// SAP: Set Parameters Request (Daten Master)
///////////////////////////////////////////////////////////////////////////////////////////////////
//#define
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// SAP: Check Config Request (Daten Master)
///////////////////////////////////////////////////////////////////////////////////////////////////
#define CFG_DIRECTION_        0x30  // Bit 4-5 ist Richtung. 01 =  Eingang, 10 = Ausgang, 11 = Eingang/Ausgang
#define CFG_INPUT             0x10  // Eingang
#define CFG_OUTPUT            0x20  // Ausgang
#define CFG_INPUT_OUTPUT      0x30  // Eingang/Ausgang
#define CFG_SPECIAL           0x00  // Spezielles Format wenn mehr als 16/32Byte uebertragen werden sollen

#define CFG_KONSISTENZ_       0x80  // Bit 7 ist Konsistenz. 0 = Byte oder Wort, 1 = Ueber gesamtes Modul
#define CFG_KONS_BYTE_WORT    0x00  // Byte oder Wort
#define CFG_KONS_MODUL        0x80  // Modul

#define CFG_WIDTH_            0x40  // Bit 6 ist IO Breite. 0 = Byte (8bit), 1 = Wort (16bit)
#define CFG_BYTE              0x00  // Byte
#define CFG_WORD              0x40  // Wort

/* Kompaktes Format */
#define CFG_BYTE_CNT_         0x0F  // Bit 0-3 sind Anzahl der Bytes oder Worte. 0 = 1 Byte, 1 = 2 Byte usw.

/* Spezielles Format */
#define CFG_SP_DIRECTION_     0xC0  // Bit 6-7 ist Richtung. 01 =  Eingang, 10 = Ausgang, 11 = Eingang/Ausgang
#define CFG_SP_VOID           0x00  // Leerplatz
#define CFG_SP_INPUT          0x40  // Eingang
#define CFG_SP_OUTPUT         0x80  // Ausgang
#define CFG_SP_INPUT_OPTPUT   0xC0  // Eingang/Ausgang

#define CFG_SP_VENDOR_CNT_    0x0F  // Bit 0-3 sind Anzahl der herstellerspezifischen Bytes. 0 = keine

/* Spezielles Format / Laengenbyte */
#define CFG_SP_BYTE_CNT_      0x3F  // Bit 0-5 sind Anzahl der Bytes oder Worte. 0 = 1 Byte, 1 = 2 Byte usw.
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
#define TIMEOUT_MAX_SYN_TIME  33 * DELAY_TBIT // 33 TBit = TSYN
#define TIMEOUT_MAX_RX_TIME   15 * DELAY_TBIT
#define TIMEOUT_MAX_TX_TIME   15 * DELAY_TBIT
#define TIMEOUT_MAX_SDR_TIME  15 * DELAY_TBIT // 15 Tbit = TSDR

#define TA_SMCLK_500KHZ_INT   TACTL = MC1 + TASSEL1 + ID_3;
//#define DELAY_TIMER_1_7MS   0x035C  // 33 TB (UART @ 19200)
//#define DELAY_TIMER_782US   0x0187  // 15 TB (UART @ 19200)
//#define DELAY_TBIT          26.04   // UART @ 19200

//#define DELAY_TIMER_1_7MS   0x00B1  // 33 TB (UART @ 93750)
//#define DELAY_TIMER_782US   0x0051  // 15 TB (UART @ 93750)
#define DELAY_TBIT            5.33    // UART @ 93750
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
#define MAX_BUFFER_SIZE       45

#define INPUT_DATA_SIZE       35    // Anzahl Bytes die vom Master kommen
#define OUTPUT_DATA_SIZE      35    // Anzahl Bytes die an Master gehen
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
void          init_Profibus               (void);
void          profibus_RX                 (void);
void          profibus_send_CMD           (unsigned char type,
                                           unsigned char function_code,
                                           unsigned char sap_offset,
                                           char *pdu,
                                           unsigned char length_pdu);
void          profibus_TX                 (char *data, unsigned char length);

unsigned char checksum                    (char *data, unsigned char length);
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif