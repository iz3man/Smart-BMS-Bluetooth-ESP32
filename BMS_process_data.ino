bool isPacketValid(byte *packet) //check if packet is valid
{
  TRACE;
  if (packet == nullptr)
  {
    return false;
  }

  bmsPacketHeaderStruct *pHeader = (bmsPacketHeaderStruct *)packet;
  int checksumLen = pHeader->dataLen + 2; // status + data len + data

  if (pHeader->start != 0xDD)
  {
    return false;
  }

  int offset = 2; // header 0xDD and command type are skipped

  byte checksum = 0;

  for (int i = 0; i < checksumLen; i++)
  {
    checksum += packet[offset + i];
  }

  //printf("checksum: %x\n", checksum);

  checksum = ((checksum ^ 0xFF) + 1) & 0xFF;
  //printf("checksum v2: %x\n", checksum);

  byte rxChecksum = packet[offset + checksumLen + 1];

  if (checksum == rxChecksum)
  {
    //printf("Packet is valid\n");
    return true;
  }
  else
  {
    //printf("Packet is not valid\n");
    //printf("Expected value: %x\n", rxChecksum);
    return false;
  }
}

bool processBasicInfo(packBasicInfoStruct *output, byte *data, unsigned int dataLen)
{
  TRACE;  // Expected data len

  if (dataLen != 0x1B)
  {
    return false;
  }

  output->Volts = ((uint32_t)two_ints_into16(data[0], data[1])) * 10; // Resolution 10 mV -> convert to milivolts
  output->Amps = ((int32_t)two_ints_into16(data[2], data[3])) * 10;   // Resolution 10 mA -> convert to miliamps

  output->Watts = output->Volts * output->Amps / 1000000; // W

  output->CapacityRemainAh = ((uint16_t)two_ints_into16(data[4], data[5])) * 10;
  output->CapacityRemainPercent = ((uint8_t)data[19]);

  output->CapacityRemainWh = (output->CapacityRemainAh * c_cellNominalVoltage) / 1000000 * packCellInfo.NumOfCells;

  output->Temp1 = (((uint16_t)two_ints_into16(data[23], data[24])) - 2731);
  output->Temp2 = (((uint16_t)two_ints_into16(data[25], data[26])) - 2731);
  output->BalanceCodeLow = (two_ints_into16(data[12], data[13]));
  output->BalanceCodeHigh = (two_ints_into16(data[14], data[15]));
  output->MosfetStatus = ((byte)data[20]);

  return true;
};

bool processCellInfo(packCellInfoStruct *output, byte *data, unsigned int dataLen)
{

  TRACE;
  uint16_t _cellSum;
  uint16_t _cellMin = 5000;
  uint16_t _cellMax = 0;
  uint16_t _cellAvg;
  uint16_t _cellDiff;

  output->NumOfCells = dataLen / 2; // Data length * 2 is number of cells !!!!!!

  // go trough individual cells

  for (byte i = 0; i < dataLen / 2; i++)
  {
    output->CellVolt[i] = ((uint16_t)two_ints_into16(data[i * 2], data[i * 2 + 1])); // Resolution 1 mV
    _cellSum += output->CellVolt[i];
    if (output->CellVolt[i] > _cellMax)
    {
      _cellMax = output->CellVolt[i];
    }
    if (output->CellVolt[i] < _cellMin)
    {
      _cellMin = output->CellVolt[i];
    }

  }
  output->CellMin = _cellMin;
  output->CellMax = _cellMax;
  output->CellDiff = _cellMax - _cellMin; // Resolution 10 mV -> convert to volts
  output->CellAvg = _cellSum / output->NumOfCells;

  // cell median calculation

  uint16_t n = output->NumOfCells;
  uint16_t i, j;
  uint16_t temp;
  uint16_t x[n];

  for (uint8_t u = 0; u < n; u++)
  {
    x[u] = output->CellVolt[u];
  }

  for (i = 1; i <= n; ++i) //sort data
  {
    for (j = i + 1; j <= n; ++j)
    {
      if (x[i] > x[j])
      {
        temp = x[i];
        x[i] = x[j];
        x[j] = temp;
      }
    }
  }

  if (n % 2 == 0) // compute median
  {
    output->CellMedian = (x[n / 2] + x[n / 2 + 1]) / 2;
  }
  else
  {
    output->CellMedian = x[n / 2 + 1];
  }

  for (uint8_t q = 0; q < output->NumOfCells; q++)
  {
    uint32_t disbal = abs(output->CellMedian - output->CellVolt[q]);
  }
  return true;
};

bool bmsProcessPacket(byte *packet)
{
  TRACE;
  bool isValid = isPacketValid(packet);

  if (isValid != true)
  {
    commSerial.println("Invalid packer received");
    return false;
  }

  bmsPacketHeaderStruct *pHeader = (bmsPacketHeaderStruct *)packet;
  byte *data = packet + sizeof(bmsPacketHeaderStruct); // TODO Fix this ugly hack
  unsigned int dataLen = pHeader->dataLen;

  bool result = false;

  // Decision based on pac ket type (info3 or info4)
  switch (pHeader->type)
  {
    case cBasicInfo3:
      {
        // Process basic info
        result = processBasicInfo(&packBasicInfo, data, dataLen);
        newPacketReceived = true;
        break;
      }

    case cCellInfo4:
      {
        result = processCellInfo(&packCellInfo, data, dataLen);
        newPacketReceived = true;
        break;
      }

    default:
      result = false;
      commSerial.printf("Unsupported packet type detected. Type: %d", pHeader->type);
  }

  return result;
}

bool bmsCollectPacket_uart(byte *packet) // unused function to get packet directly from uart
{
  TRACE;
#define packet1stByte 0xdd
#define packet2ndByte 0x03
#define packet2ndByte_alt 0x04
#define packetLastByte 0x77
  bool retVal;
  byte actualByte;
  static byte previousByte;
  static bool inProgress = false;
  static byte bmsPacketBuff[40];
  static byte bmsPacketBuffCount;

  if (bmsSerial.available() > 0) // data in serial buffer available
  {
    actualByte = bmsSerial.read();
    if (previousByte == packetLastByte && actualByte == packet1stByte) // got packet footer
    {
      memcpy(packet, bmsPacketBuff, bmsPacketBuffCount);
      inProgress = false;
      retVal = true;
    }
    if (inProgress) // filling bytes to output buffer
    {
      bmsPacketBuff[bmsPacketBuffCount] = actualByte;
      bmsPacketBuffCount++;
      retVal = false;
    }

    if (previousByte == packet1stByte && (actualByte == packet2ndByte || actualByte == packet2ndByte_alt)) // got packet header
    {
      bmsPacketBuff[0] = previousByte;
      bmsPacketBuff[1] = actualByte;
      bmsPacketBuffCount = 2; // for next pass. [0] and [1] are filled already
      inProgress = true;
      retVal = false;
    }

    previousByte = actualByte;
  }
  else
  {
    retVal = false;
  }
  return retVal;
}

bool bleCollectPacket(char *data, uint32_t dataSize) // reconstruct packet from BLE incomming data, called by notifyCallback function
{
  TRACE;
  static uint8_t packetstate = 0; // 0 - empty, 1 - first half of packet received, 2 - second half of packet received
  static uint8_t packetbuff[40] = {0x0};
  static uint32_t previousDataSize = 0;
  bool retVal = false;
  //hexDump(data,dataSize);

  if (data[0] == 0xdd && packetstate == 0) // probably got 1st half of packet
  {
    packetstate = 1;
    previousDataSize = dataSize;
    for (uint8_t i = 0; i < dataSize; i++)
    {
      packetbuff[i] = data[i];
    }
    retVal = false;
  }

  if (data[dataSize - 1] == 0x77 && packetstate == 1) // probably got 2nd half of the packet
  {
    packetstate = 2;
    for (uint8_t i = 0; i < dataSize; i++)
    {
      packetbuff[i + previousDataSize] = data[i];
    }
    retVal = false;
  }

  if (packetstate == 2) // got full packet
  {
    uint8_t packet[dataSize + previousDataSize];
    memcpy(packet, packetbuff, dataSize + previousDataSize);

    bmsProcessPacket(packet); // pass pointer to retrieved packet to processing function
    packetstate = 0;
    retVal = true;
  }
  return retVal;
}
void bmsGetInfo3()
{
  TRACE;
  // header status command length data checksum footer
  //   DD     A5      03     00    FF     FD      77
  uint8_t data[7] = {0xdd, 0xa5, 0x3, 0x0, 0xff, 0xfd, 0x77};
  sendCommand(data, sizeof(data));
}

void bmsGetInfo4()
{
  TRACE;
  //  DD  A5 04 00  FF  FC  77
  uint8_t data[7] = {0xdd, 0xa5, 0x4, 0x0, 0xff, 0xfc, 0x77};
  sendCommand(data, sizeof(data));
}

void printBasicInfo() // debug all data to uart
{
  TRACE;
  commSerial.printf("Total voltage: %f\n", (float)packBasicInfo.Volts / 1000);
  commSerial.printf("Amps: %f\n", (float)packBasicInfo.Amps / 1000);
  commSerial.printf("CapacityRemainAh: %f\n", (float)packBasicInfo.CapacityRemainAh / 1000);
  commSerial.printf("CapacityRemainPercent: %d\n", packBasicInfo.CapacityRemainPercent);
  commSerial.printf("Temp1: %f\n", (float)packBasicInfo.Temp1 / 10);
  commSerial.printf("Temp2: %f\n", (float)packBasicInfo.Temp2 / 10);
  commSerial.printf("Balance Code Low: 0x%x\n", packBasicInfo.BalanceCodeLow);
  commSerial.printf("Balance Code High: 0x%x\n", packBasicInfo.BalanceCodeHigh);
  commSerial.printf("Mosfet Status: 0x%x\n", packBasicInfo.MosfetStatus);
}

void printCellInfo() // debug all data to uart
{
  TRACE;
  commSerial.printf("Number of cells: %u\n", packCellInfo.NumOfCells);
  for (byte i = 1; i <= packCellInfo.NumOfCells; i++)
  {
    commSerial.printf("Cell no. %u", i);
    commSerial.printf("   %f\n", (float)packCellInfo.CellVolt[i - 1] / 1000);
  }
  commSerial.printf("Max cell volt: %f\n", (float)packCellInfo.CellMax / 1000);
  commSerial.printf("Min cell volt: %f\n", (float)packCellInfo.CellMin / 1000);
  commSerial.printf("Difference cell volt: %f\n", (float)packCellInfo.CellDiff / 1000);
  commSerial.printf("Average cell volt: %f\n", (float)packCellInfo.CellAvg / 1000);
  commSerial.printf("Median cell volt: %f\n", (float)packCellInfo.CellMedian / 1000);
  commSerial.println();
}

void hexDump(const char *data, uint32_t dataSize) // debug function
{
  TRACE;
  commSerial.println("HEX data:");

  for (int i = 0; i < dataSize; i++)
  {
    commSerial.printf("0x%x, ", data[i]);
  }
  commSerial.println("");
}

int16_t two_ints_into16(int highbyte, int lowbyte) // turns two bytes into a single long integer
{
  TRACE;
  int16_t result = (highbyte);
  result <<= 8;                // Left shift 8 bits,
  result = (result | lowbyte); // OR operation, merge the two
  return result;
}
