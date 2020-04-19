import sys 
import serial as serial,time
import struct      
from PyQt5 import uic
from PyQt5.QtCore import QFile, QIODevice
from PyQt5.QtSerialPort import QSerialPort, QSerialPortInfo 
from PyQt5.QtWidgets import QApplication, QFileDialog

Rom = None

class Programmer():

    def __init__(self, portName, memoryType):
        self.port = portName
        self.type = str(memoryType)

    def write(self, progress ):
        global Rom        
        wRom = Rom
        progress.setValue(0)
        ser = serial.Serial(self.port, 115200, timeout=4.0, rtscts=True, xonxoff=False)
        ser.setDTR(False)
        time.sleep(1)
        romsize = len( wRom )
        numsectors=int( romsize / 1024 )
        progress.setMaximum(numsectors)
        for i in range(numsectors):
            ser.write(bytes(self.type,"ASCII"))
            ser.write(bytes("w","ASCII"))
            address = i*1024
            data = bytearray(wRom[i*1024:(i+1)*1024])
            CHK = ((address>>16)&0xFF)^((address>>8)&0xFF)^(address&0xFF)
            ser.write(bytearray([(address>>16)&0xFF,(address>>8)&0xFF,address&0xFF]))
            for k in range(int(1024/32)):
                for j in range(32):
                    CHK=CHK^data[k*int(1024/32)+j]
                response=~CHK
                while response!=CHK:
                    time.sleep(0.01)
                    ser.write(data[k*int(1024/32):(k+1)*int(1024/32)])
                    ser.write(struct.pack(">B",CHK&0xFF))     
                    response=ord(ser.read(1))
                    print(response)
                    if response!=CHK:
                        print("wrong checksum, sending chunk again\n")
                CHK = 0
            progress.setValue(i + 1)
            ser.read(1)
            time.sleep(0.01)
        ser.close()
    def read(self, romSize, progress):
        rRom = bytearray()
        progress.setValue(0)
        ser = serial.Serial(self.port, 115200, timeout=None)
        time.sleep(1)
        progress.setMaximum(romSize)
        print (bytes(self.type,"ASCII"))
        ser.write(bytes(self.type,"ASCII"))
        ser.write(bytes("r","ASCII")) 
        data = ser.read(128) 
        ser.close()
        return data
        

class RomConverter():
    def __init__(self):
        global Rom
        self.iRom = Rom
        self.a160bits = [4,5,6,7,12,15,18,17,1,10,16,19,14,13,8,9,11,3,20,2]
        self.q160bits = [0,1,2,6,4,3,5,7]
        self.a801bits = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,18,19,16,17]

    def convertByteM27C160(self,bytes):
        nBytes=[]
        for byte in bytes:
            nByte = 0;
            for i in range(8):
                if(byte & (1<<i)):
                    nByte = nByte | (1 << (self.q160bits[i]))
            nBytes.append(nByte)
        return nBytes
        
    def convertRom(self,eepromType,invertByte = False):
        wordSize = 1
        if(eepromType == '27c160'):
            adressMass = self.a160bits
            wordSize = 2
        elif(eepromType == '27c801'):
            adressMass = self.a801bits
        else:
            print('Memory type is not exist')
            return None
        adressCount = 2**(len(adressMass))
        nRom = [0xFF for i in range(adressCount * wordSize)]
        for i in range(adressCount):
            fRomAdress = 0 
            for j in range(len(adressMass)):
                if(i & (1 << j)):
                    fRomAdress = fRomAdress | (1<< (adressMass[j] - 1))
            if(eepromType == '27c160'):
                bytes = self.convertByteM27C160([ self.iRom[fRomAdress*2], self.iRom[fRomAdress*2 + 1] ])
                nRom[i*2] = bytes[0]
                nRom[i*2+1] = bytes[1]
            elif(eepromType == '27c801'):
                if(invertByte):
                    nRom[i]=~self.iRom[fRomAdress]
        return nRom    

class Aplication():
    def exec_(self):
        self.app.exec_()
    
    def initConsole(self):
        self.mainWidget.cBConsole.addItems(['SEGA GEN/MD','SNES/SFC'])

    def usableMemory(self,index):
        mW = self.mainWidget
        mW.cBMemory.clear()
        if(index == 1):
            mW.cBMemory.addItems(['27c801','27c160','27c322'])

    def binOpen(self):
        global Rom
        fBrowser = QFileDialog()
        filePath = fBrowser.getOpenFileName(self.mainWidget, \
            "Open Bin", \
            "", \
            "Bin file *.smc *.sfc *.bin");
        Rom = []
        with open(filePath[0], "rb") as binFile:
            for byte in binFile:
                Rom+=byte
            self.mainWidget.burnBtn.setEnabled(True)              
    
    def burn(self):
        global Rom  
        mWid = self.mainWidget 
        memoryType = mWid.cBMemory.currentIndex() 
        if(memoryType != -1):          
            programmer = Programmer(mWid.aPorts.currentText(), memoryType)
            Rom = RomConverter().convertRom(mWid.cBMemory.currentText())
            with open('2.sfc','wb') as fl: 
                fl.write(bytearray(Rom))
                fl.close()
            programmer.write(mWid.progressBar)
        
    def dump(self):
        mWid = self.mainWidget 
        memoryType = mWid.cBMemory.currentIndex() 
        if(memoryType != -1):
            with open('1.sfc','wb') as fl:         
                programmer = Programmer(mWid.aPorts.currentText(), memoryType)
                fl.write(programmer.read(2**20,mWid.progressBar))
                fl.close()

    def close(self):
        exit()

    def __init__(self):
        self.app = QApplication(sys.argv)
        self.mainWidget = uic.loadUi("main.ui")
        mWid = self.mainWidget
        for port in QSerialPortInfo().availablePorts():
            mWid.aPorts.addItem(port.systemLocation())
        mWid.actionExit.triggered.connect(self.close)
        mWid.actionOpen.triggered.connect(self.binOpen)
        mWid.cBConsole.activated.connect(self.usableMemory)
        mWid.dumpBtn.released.connect(self.dump)
        mWid.burnBtn.released.connect(self.burn)
        mWid.burnBtn.setDisabled(True)
        #mWid.dumpBtn.setDisabled(True)
        self.initConsole()
        mWid.show()
        
if __name__ == '__main__':
    Aplication().exec_()
    
