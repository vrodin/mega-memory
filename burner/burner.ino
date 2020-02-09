//FIXME


const int programPin=3;
const int readPin=4;
const int enablePin=2;

const unsigned long romSize=1024*1024;


/*
 * snes pinout(some of the pins don't follow the standard pinout)
 * use this pinout for burning eproms for use with snes.
 * You can also use the other pinout and mess up the rom using utilities
 * available online
 */
 
int adrPins[20]={ 22,//eprom A0  snes A0
                  23,//eprom A1  snes A1
                  24,//eprom A2  snes A2
                  25,//eprom A3  snes A3
                  26,//eprom A4  snes A4
                  27,//eprom A5  snes A5
                  28,//eprom A6  snes A6
                  29,//eprom A7  snes A7
                  30,//eprom A8  snes A8
                  31,//eprom A9  snes A9
                  32,//eprom A10 snes A10
                  33,//eprom A11 snes A11
                  34,//eprom A12 snes A12
                  35,//eprom A13 snes A13
                  36,//eprom A14 snes A14
                  37,//eprom A15 snes A15
                  40,//eprom A18 snes A16 *
                  41,//eprom A19 snes A17 *
                  38,//eprom A16 snes A18 *
                  39,//eprom A17 snes A19 * 
                  };

/*                  
 *Regular eprom pinout, uncommnet this one and comment the above                   
 *to burn regular eproms for use with other systems.
 *You may have to actually edit this for use with your console/8bit computer
 *The pinout from the eprom is different from the snes pinout
int adrPins[20]={ 22,//eprom A0  snes A0
                  23,//eprom A1  snes A1
                  24,//eprom A2  snes A2
                  25,//eprom A3  snes A3
                  26,//eprom A4  snes A4
                  27,//eprom A5  snes A5
                  28,//eprom A6  snes A6
                  29,//eprom A7  snes A7
                  30,//eprom A8  snes A8
                  31,//eprom A9  snes A9
                  32,//eprom A10 snes A10
                  33,//eprom A11 snes A11
                  34,//eprom A12 snes A12
                  35,//eprom A13 snes A13
                  36,//eprom A14 snes A14
                  37,//eprom A15 snes A15
                  38,//eprom A16 snes A18 *
                  39,//eprom A17 snes A19 *
                  40,//eprom A18 snes A16 *
                  41 //eprom A19 snes A17 *
                  };
*/
char dataPins[8]={5,6,7,8,9,10,11,12};

byte inByte=0;
unsigned int secH=0,secL=0;

void setup() {
  pinMode(programPin,OUTPUT);
  pinMode(readPin,OUTPUT);
  pinMode(enablePin,OUTPUT);
  for(int i=0;i<20;i++){
    pinMode(adrPins[i],OUTPUT);
  }
  digitalWrite(programPin,LOW);
  digitalWrite(readPin,LOW);
  digitalWrite(enablePin,HIGH);
  Serial.begin(250000);
  delay(1000);
  programMode();
}
int index=0;
void loop() {
  if(Serial.available()){
    inByte=Serial.read();
    if(inByte==0x55){
      while(Serial.available()==0);
      inByte=Serial.read();
      switch(inByte){
        case 'w':
          programMode();
          while(Serial.available()<2);
          secH=Serial.read();
          secL=Serial.read();
          writeSector(secH,secL);
          break;
        case 'r':
          readMode();
          readROM(22);
          break;
      }
    }
  }
}


//low level functions, direct ccontact with hardware pins
void programMode(int wordSize){
  //data as output
  for(int i=0;i<8*wordSize;++i){
    pinMode(dataPins[i],OUTPUT);
  }
  digitalWrite(readPin,LOW);
  digitalWrite(programPin,HIGH);
}
void readMode(int wordSize)
{
  //data as input
  for(int i=0;i<8*wordSize;++i)
  {
    pinMode(dataPins[i],INPUT);
  }
  digitalWrite(programPin,LOW);
  digitalWrite(readPin,LOW);

}
void setAddress(uint32_t Addr, short addrSize)
{
    for(int i=0;i<addrSize;++i)
    {
      digitalWrite(adrPins[i],Addr&(1<<i));
    }
}
byte readByte(unsigned long adr)
{
    byte data;
    setAddress(adr);
    digitalWrite(enablePin,LOW);
    delayMicroseconds(1);
    for(int i=7;i>=0;i--){
        data=data<<1;
        data|=digitalRead(dataPins[i])&1;
    }
    digitalWrite(enablePin,HIGH);
    return data;
}
void setData(short Data,int wordSize)
{
  for(int i=0; i < 8 * wordSize; i++)
  {
      digitalWrite(dataPins[i],Data&(1<<i));
  }
}
void programByte(byte Data){
  setData(Data);
  //Vpp pulse
  delayMicroseconds(4);
  digitalWrite(enablePin,LOW);
  delayMicroseconds(60);
  digitalWrite(enablePin,HIGH);
}

void writeSector(unsigned char sectorH,unsigned char sectorL){
  byte dataBuffer[128];
  unsigned long address=0;
  short CHK=0xFFFF,CHKreceived;
  

  address=sectorH;
  address=(address<<8)|sectorL;
  address*=128;

  for(int i=0;i<128;i++){
        while(Serial.available()==0);
        dataBuffer[i]=Serial.read();
        CHK ^= dataBuffer[i] << 8;
        for(int j=0; j<8; j++)
        {
          CHK = CHK & 0x8000 ? (CHK << 1) ^ 0x1021 : CHK << 1;
        }
  }
  while(Serial.available()==0);
  CHKreceived = Serial.read() | (Serial.read() << 8);
  programMode();
  //only program the bytes if the checksum is equal to the one received
  if(CHKreceived==CHK){
    for (int i = 0; i < 128; i++){
      setAddress(address++);
      programByte(dataBuffer[i]);
    }
  Serial.write(CHK);
  Serial.write(CHKreceived);
  }
  readMode();

}
int readROM(int addrSize){
  unsigned long num=pow(2,addrSize);
  unsigned long address;
  readMode();
  digitalWrite(readPin,LOW);
  digitalWrite(programPin,LOW);
  for(long address = 0; address < num; ++address){//1048576
    Serial.write(readByte(address++));
  }
  digitalWrite(readPin,HIGH);
}
