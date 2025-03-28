/*!
 * \file    profibus.c
 * \brief   Ablaufsteuerung Profibus DP-Slave Kommunikation
 * \author  � Joerg S.
 * \date    9.2007 (Erstellung) 10.2008 (Aktueller Stand)
 * \note    Verwendung nur fuer private Zwecke / Only for non-commercial use
 */
 /*
���������: 
- �������������� ���������� RX. 
- ����� ������������ ���������� ����������� ����� ����������� ����������� ����� 
- ���������� TX ������ �� ����, ���� ����� ��������� ��������� ���� 
- ������������ � TX �� RX (���������������� RS485) ������ ����� ������ 
(PROFIBUS_SEND_DATA) 
- ������ ��������� ���������� �� ������� �� ����� ��������� 
- ����������� ���������� �������� ���������� �� �������. 

��� ��������� ��������� ����� ��������� ��� 
���������� Profibus � ������������� ������ ����������� ����� ������� � ������� 
������ � ��.*/


#include "profibus.h"


char uart_buffer[MAX_BUFFER_SIZE];
unsigned int uart_byte_cnt = 0;
unsigned int uart_transmit_cnt = 0;


// Profibus Flags und Variablen
unsigned char profibus_status;
unsigned char slave_addr;
unsigned char master_addr;
unsigned char group;

unsigned char data_out_register[OUTPUT_DATA_SIZE];
unsigned char data_in_register [INPUT_DATA_SIZE];
unsigned char Input_Data_size;
unsigned char Output_Data_size;
//unsigned char Input_Data_size_Module[INPUT_MODULE_CNT];
//unsigned char Output_Data_size_Module[OUTPUT_MODULE_CNT];
unsigned char Vendor_Data_size;

#define TX_IRQ

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Profibus Timer und Variablen initialisieren
 */
void init_Profibus (void)
{
  unsigned char cnt;

  // Variablen initialisieren
  Input_Data_size = 0;
  Output_Data_size = 0;
  Vendor_Data_size = 0;
  group = 0;

  // Slave Adresse holen
  slave_addr = read_slave_addr();

  // Keine ungueltigen Adresse zulassen
  if((slave_addr == 0) || (slave_addr > 126))
    slave_addr = DEFAULT_ADD;

  // Register loeschen
  for (cnt = 0; cnt < OUTPUT_DATA_SIZE; cnt++)
  {
    data_out_register[cnt] = 0xFF;
  }
  for (cnt = 0; cnt < INPUT_DATA_SIZE; cnt++)
  {
    data_in_register[cnt] = 0x00;
  }

  profibus_status = PROFIBUS_WAIT_SYN;

  // Timer init
  TA_SMCLK_500KHZ;
  TAR = 0;
  TACCR0 = TIMEOUT_MAX_SYN_TIME;
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief ISR UART0 Receive
 */
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR (void)
{

  // Erst mal Byte in Buffer sichern
  uart_buffer[uart_byte_cnt] = UCA0RXBUF;


  // Nur einlesen wenn TSYN abgelaufen
  if (profibus_status == PROFIBUS_WAIT_DATA)
  {
    // TSYN abgelaufen, Daten einlesen
    profibus_status = PROFIBUS_GET_DATA;
  }

  // Einlesen erlaubt?
  if (profibus_status == PROFIBUS_GET_DATA)
  {
    uart_byte_cnt++;

    // Nicht mehr einlesen als in Buffer reinpasst
    if(uart_byte_cnt == MAX_BUFFER_SIZE) uart_byte_cnt--;
  }


  // Profibus Timer ruecksetzen
  TAR = 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Profibus auswertung
 */
void profibus_RX (void)
{
  unsigned char cnt;
  unsigned char telegramm_type;
  unsigned char process_data;

  // Profibus Datentypen
  unsigned char destination_add;
  unsigned char source_add;
  unsigned char function_code;
  unsigned char FCS_data;   // Frame Check Sequence
  unsigned char PDU_size;   // PDU Groesse
  unsigned char DSAP_data;  // SAP Destination
  unsigned char SSAP_data;  // SAP Source


  process_data = FALSE;


  telegramm_type = uart_buffer[0];

  switch (telegramm_type)
  {
    case SD1: // Telegramm ohne Daten, max. 6 Byte

        if (uart_byte_cnt != 6) break;

        destination_add = uart_buffer[1];
        source_add      = uart_buffer[2];
        function_code   = uart_buffer[3];
        FCS_data        = uart_buffer[4];

        if (addmatch(destination_add)       == FALSE) break;
        if (checksum(&uart_buffer[1], 3) != FCS_data) break;

        process_data = TRUE;

        break;

    case SD2: // Telegramm mit variabler Datenlaenge

        if (uart_byte_cnt != uart_buffer[1] + 6) break;

        PDU_size        = uart_buffer[1];
        destination_add = uart_buffer[4];
        source_add      = uart_buffer[5];
        function_code   = uart_buffer[6];
        FCS_data        = uart_buffer[PDU_size + 4];

        if (addmatch(destination_add)              == FALSE) break;
        if (checksum(&uart_buffer[4], PDU_size) != FCS_data) break;

        process_data = TRUE;

        break;

    case SD3: // Telegramm mit 5 Byte Daten, max. 11 Byte

        if (uart_byte_cnt != 11) break;

        destination_add = uart_buffer[1];
        source_add      = uart_buffer[2];
        function_code   = uart_buffer[3];
        FCS_data        = uart_buffer[9];

        if (addmatch(destination_add)       == FALSE) break;
        if (checksum(&uart_buffer[1], 8) != FCS_data) break;

        process_data = TRUE;

        break;

    case SD4: // Token mit 3 Byte Daten

        if (uart_byte_cnt != 3) break;

        destination_add = uart_buffer[1];
        source_add      = uart_buffer[2];

        if (addmatch(destination_add)       == FALSE) break;

        break;

    default:

        break;

  } // Switch Ende



  // Nur auswerten wenn Daten OK sind
  if (process_data == TRUE)
  {
    master_addr = source_add; // Master Adresse ist Source Adresse


    // Service Access Point erkannt?
    if ((destination_add & 0x80) && (source_add & 0x80))
    {
      DSAP_data = uart_buffer[7];
      SSAP_data = uart_buffer[8];


      // Ablauf Reboot:
      // 1) SSAP 62 -> DSAP 60 (Get Diagnostics Request)
      // 2) SSAP 62 -> DSAP 61 (Set Parameters Request)
      // 3) SSAP 62 -> DSAP 62 (Check Config Request)
      // 4) SSAP 62 -> DSAP 60 (Get Diagnostics Request)
      // 5) Data Exchange Request (normaler Zyklus)

      switch (DSAP_data)
      {
        case SAP_SET_SLAVE_ADR: // Set Slave Address (SSAP 62 -> DSAP 55)

            // Adressaenderung siehe Seite 93

            // Nur im Zustand "Wait Parameter" (WPRM) moeglich

            //new_addr = uart_buffer[9];
            //IDENT_HIGH_BYTE = uart_buffer[10];
            //IDENT_LOW_BYTE = uart_buffer[11];
            //if (uart_buffer[12] & 0x01) adress_aenderung_sperren = TRUE;

            profibus_send_CMD(SC, 0, SAP_OFFSET, &uart_buffer[0], 0);

            break;

        case SAP_GLOBAL_CONTROL: // Global Control Request (SSAP 62 -> DSAP 58)

            // Wenn "Clear Data" high, dann SPS CPU auf "Stop"
            if (uart_buffer[9] & CLEAR_DATA_)
            {
              LED_ERROR_AN;  // Status "SPS nicht bereit"
            }
            else
            {
              LED_ERROR_AUS; // Status "SPS OK"
            }

            // Gruppe berechnen
            for (cnt = 0;  uart_buffer[10] != 0; cnt++) uart_buffer[10]>>=1;

            // Wenn Befehl fuer uns ist
            if (cnt == group)
            {
              if (uart_buffer[9] & UNFREEZE_)
              {
                // FREEZE Zustand loeschen
              }
              else if (uart_buffer[9] & UNSYNC_)
              {
                // SYNC Zustand loeschen
              }
              else if (uart_buffer[9] & FREEZE_)
              {
                // Eingaenge nicht mehr neu einlesen
              }
              else if (uart_buffer[9] & SYNC_)
              {
                // Ausgaenge nur bei SYNC Befehl setzen
              }
            }

            break;

        case SAP_SLAVE_DIAGNOSIS: // Get Diagnostics Request (SSAP 62 -> DSAP 60)

            // Nach dem Erhalt der Diagnose wechselt der DP-Slave vom Zustand
            // "Power on Reset" (POR) in den Zustand "Wait Parameter" (WPRM)

            // Am Ende der Initialisierung (Zustand "Data Exchange" (DXCHG))
            // sendet der Master ein zweites mal ein Diagnostics Request um die
            // korrekte Konfiguration zu pruefen

            if (function_code == (REQUEST_ + FCB_ + SRD_HIGH))
            {
              // Erste Diagnose Anfrage

              //uart_buffer[4]  = master_addr;                  // Ziel Master (mit SAP Offset)
              //uart_buffer[5]  = slave_addr + SAP_OFFSET;      // Quelle Slave (mit SAP Offset)
              //uart_buffer[6]  = SLAVE_DATA;
              uart_buffer[7]  = SSAP_data;                    // Ziel SAP Master
              uart_buffer[8]  = DSAP_data;                    // Quelle SAP Slave
              uart_buffer[9]  = STATION_NOT_READY_;           // Status 1
              uart_buffer[10] = STATUS_2_DEFAULT + PRM_REQ_;  // Status 2
              uart_buffer[11] = DIAG_SIZE_OK;                 // Status 3
              uart_buffer[12] = MASTER_ADD_DEFAULT;           // Adresse Master
              uart_buffer[13] = IDENT_HIGH_BYTE;              // Ident high
              uart_buffer[14] = IDENT_LOW_BYTE;               // Ident low
              //uart_buffer[15] = 0x05;     // Geraetebezogene Diagnose (Anzahl Bytes)
              //uart_buffer[16] = 0x00;     //
              //uart_buffer[17] = 0x20;
              //uart_buffer[18] = 0x20;
              //uart_buffer[19] = 0x00;

              profibus_send_CMD(SD2, DATA_LOW, SAP_OFFSET, &uart_buffer[7], 8);//13);
            }
            else if (function_code == (REQUEST_ + FCV_ + SRD_HIGH) ||
                     function_code == (REQUEST_ + FCV_ + FCB_ + SRD_HIGH))
            {
              // Diagnose Anfrage um Daten von Check Config Request zu pruefen

              //uart_buffer[4]  = master_addr;                  // Ziel Master (mit SAP Offset)
              //uart_buffer[5]  = slave_addr + SAP_OFFSET;      // Quelle Slave (mit SAP Offset)
              //uart_buffer[6]  = SLAVE_DATA;
              uart_buffer[7]  = SSAP_data;                    // Ziel SAP Master
              uart_buffer[8]  = DSAP_data;                    // Quelle SAP Slave
              uart_buffer[9]  = 0x00;                         // Status 1
              uart_buffer[10] = STATUS_2_DEFAULT;             // Status 2
              uart_buffer[11] = DIAG_SIZE_OK;                 // Status 3
              uart_buffer[12] = master_addr - SAP_OFFSET;     // Adresse Master
              uart_buffer[13] = IDENT_HIGH_BYTE;              // Ident high
              uart_buffer[14] = IDENT_LOW_BYTE;               // Ident low
              //uart_buffer[15] = 0x05;     // Geraetebezogene Diagnose (Anzahl Bytes)
              //uart_buffer[16] = 0x00;     //
              //uart_buffer[17] = 0x20;
              //uart_buffer[18] = 0x20;
              //uart_buffer[19] = 0x00;

              profibus_send_CMD(SD2, DATA_LOW, SAP_OFFSET, &uart_buffer[7], 8);//13);
            }

            break;

        case SAP_SET_PRM: // Set Parameters Request (SSAP 62 -> DSAP 61)

            // Nach dem Erhalt der Parameter wechselt der DP-Slave vom Zustand
            // "Wait Parameter" (WPRM) in den Zustand "Wait Configuration" (WCFG)

            // Master Daten siehe Seite 99

            // Nur Daten fuer unser Geraet akzeptieren
            if ((uart_buffer[13] == IDENT_HIGH_BYTE) && (uart_buffer[14] == IDENT_LOW_BYTE))
            {
              //uart_buffer[9]  = Befehl
              //uart_buffer[10] = Watchdog 1
              //uart_buffer[11] = Watchdog 2
              //uart_buffer[12] = Min TSDR
              //uart_buffer[13] = Ident H
              //uart_buffer[14] = Ident L
              //uart_buffer[15] = Gruppe
              //uart_buffer[16] = User Parameter
              //uart_buffer[16] = DPV1 Status 1 (Bei DPV1 Unterstuetzung)
              //uart_buffer[17] = DPV1 Status 2
              //uart_buffer[18] = DPV1 Status 3


              // Gruppe einlesen
              for (group = 0;  uart_buffer[15] != 0; group++) uart_buffer[15]>>=1;

              profibus_send_CMD(SC, 0, SAP_OFFSET, &uart_buffer[0], 0);
            }

            break;

        case SAP_CHK_CFG: // Check Config Request (SSAP 62 -> DSAP 62)

            // Nach dem Erhalt der Konfiguration wechselt der DP-Slave vom Zustand
            // "Wait Configuration" (WCFG) in den Zustand "Data Exchange" (DXCHG)

            // Master Daten siehe Seite 94

            // IO Konfiguration:
            // Kompaktes Format fuer max. 16/32 Byte IO
            // Spezielles Format fuer max. 64/132 Byte IO

            // Je nach PDU Datengroesse mehrere Bytes auswerten
            // LE/LEr - (DA+SA+FC+DSAP+SSAP) = Anzahl Config Bytes
            for (cnt = 0; cnt < uart_buffer[1] - 5; cnt++)
            {
              switch (uart_buffer[9+cnt] & CFG_DIRECTION_)
              {
                case CFG_INPUT:

                    Input_Data_size = (uart_buffer[9+cnt] & CFG_BYTE_CNT_) + 1;
                    if (uart_buffer[9+cnt] & CFG_WIDTH_ & CFG_WORD)
                      Input_Data_size = Input_Data_size*2;

                    break;

                case CFG_OUTPUT:

                    Output_Data_size = (uart_buffer[9+cnt] & CFG_BYTE_CNT_) + 1;
                    if (uart_buffer[9+cnt] & CFG_WIDTH_ & CFG_WORD)
                      Output_Data_size = Output_Data_size*2;

                    break;

                case CFG_INPUT_OUTPUT:

                    Input_Data_size = (uart_buffer[9+cnt] & CFG_BYTE_CNT_) + 1;
                    Output_Data_size = (uart_buffer[9+cnt] & CFG_BYTE_CNT_) + 1;
                    if (uart_buffer[9+cnt] & CFG_WIDTH_ & CFG_WORD)
                    {
                      Input_Data_size = Input_Data_size*2;
                      Output_Data_size = Output_Data_size*2;
                    }

                    break;

                case CFG_SPECIAL:

                    // Spezielles Format

                    // Herstellerspezifische Bytes vorhanden?
                    if (uart_buffer[9+cnt] & CFG_SP_VENDOR_CNT_)
                    {
                      // Anzahl Herstellerdaten sichern
                      Vendor_Data_size = uart_buffer[9+cnt] & CFG_SP_VENDOR_CNT_;

                      // Anzahl von Gesamtanzahl abziehen
                      uart_buffer[1] -= Vendor_Data_size;
                    }

                    // I/O Daten
                    switch (uart_buffer[9+cnt] & CFG_SP_DIRECTION_)
                    {
                      case CFG_SP_VOID:
                          // Leeres Datenfeld
                          break;

                      case CFG_SP_INPUT:

                          Input_Data_size = (uart_buffer[10+cnt] & CFG_SP_BYTE_CNT_) + 1;
                          if (uart_buffer[10+cnt] & CFG_WIDTH_ & CFG_WORD)
                            Input_Data_size = Input_Data_size*2;

                          cnt++;  // Dieses Byte haben wir jetzt schon

                          break;

                      case CFG_SP_OUTPUT:

                          Output_Data_size = (uart_buffer[10+cnt] & CFG_SP_BYTE_CNT_) + 1;
                          if (uart_buffer[10+cnt] & CFG_WIDTH_ & CFG_WORD)
                            Output_Data_size = Output_Data_size*2;

                          cnt++;  // Dieses Byte haben wir jetzt schon

                          break;

                      case CFG_SP_INPUT_OPTPUT:

                          // Erst Ausgang...
                          Output_Data_size = (uart_buffer[10+cnt] & CFG_SP_BYTE_CNT_) + 1;
                          if (uart_buffer[10+cnt] & CFG_WIDTH_ & CFG_WORD)
                            Output_Data_size = Output_Data_size*2;

                          // Dann Eingang
                          Input_Data_size = (uart_buffer[11+cnt] & CFG_SP_BYTE_CNT_) + 1;
                          if (uart_buffer[11+cnt] & CFG_WIDTH_ & CFG_WORD)
                            Input_Data_size = Input_Data_size*2;

                          cnt += 2;  // Diese Bytes haben wir jetzt schon

                          break;

                    } // Switch Ende

                    break;

                default:

                    Input_Data_size = 0;
                    Output_Data_size = 0;

                    break;

              } // Switch Ende
            } // For Ende

            if (Vendor_Data_size != 0)
            {
              // Auswerten
            }


            // Bei Fehler -> CFG_FAULT_ in Diagnose senden

            // Kurzquittung
            profibus_send_CMD(SC, 0, SAP_OFFSET, &uart_buffer[0], 0);

            break;

        default:

            // Unbekannter SAP

            break;

      } // Switch DSAP_data Ende

    }
    // Ziel: Slave Adresse
    else if (destination_add == slave_addr)
    {

      // Status Abfrage
      if (function_code == (REQUEST_ + FDL_STATUS))
      {
        profibus_send_CMD(SD1, FDL_STATUS_OK, 0, &uart_buffer[0], 0);
      }
      // Master sendet Ausgangsdaten und verlangt Eingangsdaten (Send and Request Data)
      else if (function_code == (REQUEST_ + FCV_ + SRD_HIGH) ||
               function_code == (REQUEST_ + FCV_ + FCB_ + SRD_HIGH))
      {

        // Daten von Master einlesen
        for (cnt = 0; cnt < INPUT_DATA_SIZE; cnt++)
        {
          data_in_register[cnt] = uart_buffer[cnt + 7];
        }



        // Daten fuer Master in Buffer schreiben
        for (cnt = 0; cnt < OUTPUT_DATA_SIZE; cnt++)
        {
          uart_buffer[cnt + 7] = data_out_register[cnt];
        }


        profibus_send_CMD(SD2, DATA_LOW, 0, &uart_buffer[7], OUTPUT_DATA_SIZE);
      }
    }

  } // Daten gueltig Ende

}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Profibus Telegramm zusammenstellen und senden
 * \param type          Telegrammtyp (SD1, SD2 usw.)
 * \param function_code Function Code der uebermittelt werden soll
 * \param sap_offset    Wert des SAP-Offset
 * \param pdu           Pointer auf Datenfeld (PDU)
 * \param length_pdu    Laenge der reinen PDU ohne DA, SA oder FC
 */
void profibus_send_CMD (unsigned char type,
                        unsigned char function_code,
                        unsigned char sap_offset,
                        char *pdu,
                        unsigned char length_pdu)
{
  unsigned char length_data;


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
      uart_buffer[1] = length_pdu + 3;  // Laenge der PDU inkl. DA, SA und FC
      uart_buffer[2] = length_pdu + 3;
      uart_buffer[3] = SD2;
      uart_buffer[4] = master_addr;
      uart_buffer[5] = slave_addr + sap_offset;
      uart_buffer[6] = function_code;

      // Daten werden vor Aufruf der Funktion schon aufgefuellt

      uart_buffer[7+length_pdu] = checksum(&uart_buffer[4], length_pdu + 3);
      uart_buffer[8+length_pdu] = ED;

      length_data = length_pdu + 9;

      break;

    case SD3:

      uart_buffer[0] = SD3;
      uart_buffer[1] = master_addr;
      uart_buffer[2] = slave_addr + sap_offset;
      uart_buffer[3] = function_code;

      // Daten werden vor Aufruf der Funktion schon aufgefuellt

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
 * \param data Pointer auf Datenfeld
 * \param length Laenge der Daten
 */
void profibus_TX (char *data, unsigned char length)
{
#ifndef TX_IRQ
// Ohne Interrupt senden
// DEBUG
  /*
TESTPIN_HIGH;
  TACCTL0 &=~CCIE;  // Interrupt aus

  RS485_TX_EN;
  uart0_send_data(&data[0], length);

  // Warten bis letztes Byte gesendet worden ist
  while (UCA0STAT & UCBUSY);
  RS485_RX_EN;

  TACCTL0 = CCIE;  // Interrupt ein
  TAR = 0;
  TACCR0 = TIMEOUT_MAX_SYN_TIME;
  profibus_status = PROFIBUS_WAIT_SYN;
//  g_irq |= IRQ_TIMER;

// DEBUG
TESTPIN_LOW;
*/
#endif


#ifdef TX_IRQ
// Mit Interrupt

  RS485_TX_EN;

  TACCR0 = TIMEOUT_MAX_TX_TIME;   // Timeout setzen
  profibus_status = PROFIBUS_SEND_DATA;

  uart_byte_cnt = length;         // Anzahl zu sendender Bytes
  uart_transmit_cnt = 0;          // Zahler fuer gesendete Bytes

  IFG2 |= UCA0TXIFG;              // TX Flag setzen
  IE2  |= UCA0TXIE;               // Enable USCI_A0 TX interrupt

#endif
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Checksumme berechnen. Einfaches addieren aller Datenbytes im Telegramm.
 * \param data Pointer auf Datenfeld
 * \param length Laenge der Daten
 * \return Checksumme
 */
unsigned char checksum(char *data, unsigned char length)
{
  unsigned char csum = 0;

  while(length--)
  {
    csum += data[length];
  }

  return csum;
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Zieladresse ueberpruefen. Adresse muss mit Slave Adresse oder Broadcast (inkl. SAP Offset)
          uebereinstimmen
 * \param destination Zieladresse
 * \return TRUE wenn Zieladresse unsere ist, FALSE wenn nicht
 */
unsigned char addmatch (unsigned char destination)
{
  if ((destination != slave_addr) &&                // Slave
      (destination != slave_addr + SAP_OFFSET) &&   // SAP Slave
      (destination != BROADCAST_ADD) &&             // Broadcast
      (destination != BROADCAST_ADD + SAP_OFFSET))  // SAP Broadcast
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief ISR UART0 Transmit
 */
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR (void)
{

  // Alles gesendet?
  if (uart_transmit_cnt < uart_byte_cnt)
  {
    // TX Buffer fuellen
    UCA0TXBUF = uart_buffer[uart_transmit_cnt++];
  }
  else
  {
    // Alles gesendet, Interrupt wieder aus
    IE2 &= ~UCA0TXIE;
  }

  TAR = 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * \brief ISR TIMER A
 */
#pragma vector=TIMERA0_VECTOR
__interrupt void TIMERA0_ISR (void)
{

  interrupts_an();

  // Timer A Stop
  TA_STOP;


  switch (profibus_status)
  {
    case PROFIBUS_WAIT_SYN: // TSYN abgelaufen

        profibus_status = PROFIBUS_WAIT_DATA;
        TACCR0 = TIMEOUT_MAX_SDR_TIME;

        uart_byte_cnt = 0;

        break;

    case PROFIBUS_WAIT_DATA:  // TSDR abgelaufen aber keine Daten da

        break;

    case PROFIBUS_GET_DATA:   // TSDR abgelaufen und Daten da

        profibus_status = PROFIBUS_WAIT_SYN;
        TACCR0 = TIMEOUT_MAX_SYN_TIME;

        profibus_RX();

        break;

    case PROFIBUS_SEND_DATA:  // Sende-Timeout abgelaufen, wieder auf Empfang gehen

        profibus_status = PROFIBUS_WAIT_SYN;
        TACCR0 = TIMEOUT_MAX_SYN_TIME;

        RS485_RX_EN;    // Auf Receive umschalten

        break;

    default:

        _NOP();

        break;

  }

  TAR = 0;

  // Timer A Start
  TA_RUN;
}
///////////////////////////////////////////////////////////////////////////////////////////////////


