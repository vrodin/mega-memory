
const int programPin=6;
const int readPin=7;
const int eP_801=A6, eP_322=3 ;
const int BYTEPin = A5 ;//only for 27c166
int enablePin = 0;

byte memoryType=0;

unsigned long addrSize = 0, wordSize = 0;
 
char adrPins_801[20]={ 5,21,22,14,15,16,17,18,A10,A9,A7,A8,19,A11,A12,20,21,A13,A14,24 };
char adrPins_322[21]={ 2,14,15,16,17,18,19,20,A13,A12,A11,A10,A9,A8,A15,A7,A6,21,24,A14,A5 }; //27c166 use this array without last element A5(BYTE pin)
char *adrPins;

char dataPins_801[8]={ 2,3,4,A0,A1,A2,A3,A5 };
char dataPins_322[16]={ 4,39,41,44,52,48,A0,A2,38,40,43,45,50,A4,A1,A3 };
char *dataPins;

byte inByte=0;
unsigned int secH=0,secL=0;

void setup() 
{
  Serial.begin(115200);
  pinMode(programPin,OUTPUT);
  pinMode(readPin,OUTPUT);
  
}

void setupMode(byte mode)
{  
  switch(mode)
  {
    case 0x30:
       adrPins = adrPins_801;
       dataPins = dataPins_801;
       wordSize = 1;
       addrSize = 20;
       enablePin = eP_801;
       break; 
    case 0x31:
       adrPins = adrPins_322;
       dataPins = dataPins_322;
       wordSize = 2;
       addrSize = 20;
       enablePin = eP_322;
       pinMode(BYTEPin,OUTPUT);
       digitalWrite(BYTEPin,HIGH);
       break;
    case 0x32:
       adrPins = adrPins_322;
       dataPins = dataPins_322;
       wordSize = 2;
       addrSize = 21;
       enablePin = eP_322;
       break;
  }
  for(int i = 0;i < addrSize; ++i)
  {
    pinMode(*(adrPins + i),OUTPUT);
  }
  pinMode(enablePin,OUTPUT);
}
int index=0;
void loop() 
{
  if(Serial.available()>36)
  {
    setupMode(Serial.read());
    inByte=Serial.read();
    switch(inByte)
    {
      case 'w':
        programMode();
        secH=Serial.read();
        secL=Serial.read();
        writeSector(secH,secL);
        break;
      case 'r':
        readMode();
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
  digitalWrite(readPin,LOW);
  digitalWrite(programPin,HIGH);
}
void readMode()
{
  //data as input
  for(int i=0;i<8*wordSize;++i)
  {
    pinMode(*(dataPins + i),INPUT);
  }
  digitalWrite(programPin,LOW);
  digitalWrite(readPin,LOW);

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
  delayMicroseconds(60);
  digitalWrite(enablePin,HIGH);
}

void writeSector(unsigned char sectorH,unsigned char sectorL)
{
  byte dataBuffer[32];
  unsigned long address=0;
  byte CHK=sectorH,CHKreceived;
  CHK^=sectorL;

  address=sectorH;
  address=(address<<8)|sectorL;
  address*=32;
  for(int i=0;i<32;i++)
  {
        dataBuffer[i]=Serial.read();
        CHK ^= dataBuffer[i];
  }
  CHKreceived = Serial.read();
  programMode();
  //only program the bytes if the checksum is equal to the one received
  if(CHKreceived==CHK){
    for (int i = 0; i < 32; ++i)
    {
      setAddress(address++);
      if(wordSize == 1) programByte(dataBuffer[i]);
      else if(wordSize == 2) { programByte(dataBuffer[i+1]<<8|dataBuffer[i]); ++i;}
    }
  Serial.write(CHK);
  }
  readMode();

}
int readROM()
{
  unsigned long num=pow(2,addrSize);
  unsigned long address;
  readMode();
  digitalWrite(readPin,LOW);
  digitalWrite(programPin,LOW);
  for(long address = 0; address < num; ++address)
  {
    Serial.write(readData(address++));
  }
  digitalWrite(readPin,HIGH);
}
