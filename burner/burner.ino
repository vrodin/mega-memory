
uint8_t programPin=6,
        Gpin = 6,
        Gpin_state = LOW,
        readPin=7,
        eP_801=A6, eP_322=3,
        enablePin = 0,
        wordSize = 0,
        memoryType=0,
        inByte=0,
        imp = 0,
        sec_0,sec_1,sec_2;

uint32_t addrSize = 0;
 
uint8_t adrPins_801[20]={ 5,21,22,14,15,16,17,18,A10,A9,A7,A8,19,A11,A12,20,21,A13,A14,24 };
uint8_t adrPins_322[21]={ 2,14,15,16,17,18,19,20,A13,A12,A11,A10,A9,A8,A15,A7,A6,21,24,A14,A5 }; //27c166 use this array without last element A5(BYTE pin)
uint8_t *adrPins;

uint8_t dataPins_801[8]={ 2,3,4,A0,A1,A2,A3,A5 };
uint8_t dataPins_322[16]={ 4,39,41,44,52,48,A0,A2,38,40,43,45,50,A4,A1,A3 };
uint8_t *dataPins;

void setup() 
{
  pinMode(programPin,OUTPUT);
  pinMode(readPin,OUTPUT);
  digitalWrite(programPin,LOW);
  digitalWrite(readPin,LOW);
  Serial.begin(115200);
  
}

void setupMode(byte mode)
{  
  switch(mode)
  {
    case '0':
       adrPins = adrPins_801;
       dataPins = dataPins_801;
       wordSize = 1;
       addrSize = 20;
       enablePin = eP_801;
       imp = 60;
       programPin = 6;
       break; 
    case '1':
       adrPins = adrPins_322;
       dataPins = dataPins_322;
       wordSize = 2;
       addrSize = 20;
       enablePin = eP_322;
       imp = 60;
       programPin = 47;
       pinMode(Gpin,OUTPUT);
       break;
    case '2':
       adrPins = adrPins_322;
       dataPins = dataPins_322;
       wordSize = 2;
       addrSize = 21;
       enablePin = eP_322;
       imp = 60;
       programPin = 6;
       break;
  }
  for(int i = 0;i < addrSize; ++i)
  {
    pinMode(*(adrPins + i),OUTPUT);
  }
  pinMode(enablePin,OUTPUT);
  digitalWrite(enablePin,HIGH);
}

void loop() 
{
  if(Serial.available()>1)
  {
    memoryType = Serial.read();
    setupMode(memoryType);
    inByte=Serial.read();
    switch(inByte)
    {
      case 'w':
        while(Serial.available()<3);
        if(memoryType == 0x31 && Gpin_state == LOW)
        {
          digitalWrite(readPin,LOW);
          delayMicroseconds(2);
          digitalWrite(Gpin,HIGH);
          delayMicroseconds(4);
        }
        sec_0=Serial.read();
        sec_1=Serial.read();
        sec_2=Serial.read();
        writeSector((uint32_t)sec_0<<16|sec_1<<8|sec_2);
        break;
      case 'r':
        if(memoryType == 0x31&& Gpin_state == HIGH)
        {
          digitalWrite(Gpin,LOW);
          delayMicroseconds(2);
          digitalWrite(readPin,HIGH);
          delayMicroseconds(4);
        }
        readROM();
        break;
    }
  }
}


void programMode()
{
  //data as output
  for(int i=0;i<8*wordSize;++i)
  {
    pinMode(*(dataPins + i),OUTPUT);
  }
  if(memoryType == 0x31)
  {
    digitalWrite(programPin,HIGH);
  }
  else
  {
    digitalWrite(readPin,LOW);
    digitalWrite(programPin,HIGH);
  }
}
void readMode()
{
  //data as input
  for(int i=0;i<8*wordSize;++i)
  {
    pinMode(*(dataPins + i),INPUT);
  }
  if(memoryType == 0x31)
  {
    digitalWrite(programPin,LOW);
  }
  else
  {
    digitalWrite(programPin,LOW);
    digitalWrite(readPin,LOW);
  }

}
void setAddress(uint32_t Addr)
{
    for(int i=0;i<addrSize;++i)
    {
      digitalWrite(*(adrPins + i),Addr&(1<<i));
    }
}
short readData(unsigned long adr)
{
    short data;
    setAddress(adr);
    digitalWrite(enablePin,LOW);
    delayMicroseconds(1);
    for(int i=8 * wordSize;i>0;--i)
    {
        data=data<<1;
        data|=digitalRead(*(dataPins + i - 1))&1;
    }
    digitalWrite(enablePin,HIGH);
    return data;
}
void setData(short Data)
{
  for(int i=0; i < 8 * wordSize; i++)
  {
      digitalWrite(*(dataPins + i),Data&(1<<i));
  }
}
void programByte(short Data)
{
  setData(Data);
  delayMicroseconds(4);
  digitalWrite(enablePin,LOW);
  delayMicroseconds(imp);
  digitalWrite(enablePin,HIGH);
}

void writeSector(uint32_t address)
{
  uint16_t byteCount = 0;
  address = (uint32_t)sec_0<<16|sec_1<<8|sec_2;
  uint8_t dataBuffer[1024], smallBuf[32];
  uint8_t CHK = 0;
  while(byteCount!=1024)
  {
    if(byteCount == 0)
    {
      CHK = sec_0^sec_1^sec_2;
    }
    while(Serial.available()<33);
    uint8_t CHKreceived;
    for(uint8_t i=0;i<32;++i)
    {
        smallBuf[i]=Serial.read();
        CHK ^= smallBuf[i];
    }
    CHKreceived = Serial.read();
    Serial.write(CHK);
    if(CHKreceived==CHK) 
    {
      for(int i=0;i<32;i++)
      {
        dataBuffer[byteCount + i]=smallBuf[i];
      }
      byteCount+=32;
      CHK = 0;
    }
    if(byteCount==1024)
    {
      programMode();
      for (uint16_t i = 0; i < 1024; ++i)
      {
        setAddress(address++);
        if(wordSize == 1) programByte(dataBuffer[i]);
        else if(wordSize == 2) { programByte(dataBuffer[i+1]<<8|dataBuffer[i]); ++i;}
      }
      break;
    }  
  }
  readMode();
  Serial.println("DONE!");
}
int readROM()
{
  uint32_t romSize=pow(2,addrSize);
  uint32_t address;
  readMode();
  digitalWrite(programPin,LOW);
  if(memoryType != 0x31)
  {
    digitalWrite(readPin,LOW);
  }
  else
  {
    digitalWrite(readPin,HIGH);
  }
  
  for(long address = 0; address < 512; ++address)
  {
    short data = readData(address++);
    Serial.write( data & 0xFF);
    if(wordSize==2) Serial.write((data>>8) & 0xFF);
  }
  digitalWrite(readPin,HIGH);
}
