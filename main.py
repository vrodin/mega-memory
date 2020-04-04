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
        ser = serial.Serial(self.port, 115200, timeout=None)
        
        romsize = len( wRom )
        numsectors=int( romsize / 32 )
        progress.setMaximum(numsectors)
        for i in range(numsectors):
            ser.write(bytes(self.type,"ASCII"))
            ser.write(bytes("w","ASCII"))
            ser.write(struct.pack(">B",i>>8))
            CHK=i>>8
            ser.write(struct.pack(">B",i&0xFF))
            CHK^=i&0xFF
            data = wRom[i*32:(i+1)*32]
            for j in range(32):
                CHK=CHK^data[j]
            response=~CHK
            while response!=CHK:
                ser.write(data)
                ser.write(struct.pack(">B",CHK&0xFF))     
                response=ord(ser.read(1))
                if response!=CHK:
                    print("wrong checksum, sending chunk again\n")
            progress.setValue(i + 1)
        ser.close()

class RomConverter():
    def __init__(self):
        global Rom
        self.iRom = Rom
        self.a160bits = [4,5,6,7,12,15,18,17,1,10,16,19,14,13,8,9,11,3,20,2]
        self.q160bits = [0,1,2,6,4,3,5,7]
        self.a801bits = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,18,19,16,17]

    def convertByteM27C160(self,bytes):
        for byte in bytes:
            nByte = 0;
            for i in range(8):
                for j in range(8):
                    if(i & (1<<j)):
                       nByte = nByte | (1 << (self.q160bits[j]))
            byte = nByte
        return bytes
        
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
            #Rom = RomConverter().convertRom(mWid.cBMemory.currentText())
            programmer.write(mWid.progressBar)
        
    def dump(self):
        print("TODO LOL, released on MC :D")

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
        mWid.dumpBtn.setDisabled(True)
        self.initConsole()
        mWid.show()
        
if __name__ == '__main__':
    Aplication().exec_()
    
