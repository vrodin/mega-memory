import sys 
from pymodbus.client.sync import ModbusSerialClient as ModbusClient
from PyQt5 import uic
from PyQt5.QtCore import QFile, QIODevice
from PyQt5.QtSerialPort import QSerialPort, QSerialPortInfo 
from PyQt5.QtWidgets import QApplication, QFileDialog

class Programmer():

    def __init__(self, portName, memoryType):
        self.port = portName
        self.type = memoryType

    def write(self, wRom ):
        UNIT = 0x01
        client = ModbusClient(method='ascii', port=self.port, timeout=1, baudrate=250000)
        client.connect()
        rq = await client.write_coil(0, True, unit=UNIT)
        rr = await client.read_coils(0, 1, unit=UNIT)
        assert(rq.function_code < 0x80)     # test that we are not an error
        assert(rr.bits[0] == True)          # test the expected value
        romsize = len( wRom )
        numsectors=int( romsize / 256 )
        for i in range(numsectors):
            rq = await client.write_coil(0, wRom[i*256:(i+1)*256-1], unit=UNIT)
            assert(rq.function_code < 0x80)     
            rr = await client.read_coils(1, 256, unit=UNIT)
            assert(rr.bits == wRom[i*256:(i+1)*256-1])
        client.close()

class RomConverter():
    def __init__(self,byteArray):
        self.iRom = byteArray
        self.a160bits = [4,5,6,7,12,15,18,17,1,10,16,19,14,13,8,9,11,3,20,2]
        self.q160bits = [0,1,2,6,4,3,5,7]
        self.a801bits = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,18,19,16,17]

    def convertByteM27C160(self,bytes):
        for byte in bytes:
            nByte = 0;
            for i in range(8):
                for j in range(8):
                    if(i & (1<<j)):
                       nByte = nByte | (1 << (self.q160bits[j] - 1))
            byte = nByte
        return bytes
        
    def convertRom(self,eepromType,invertByte = False):
        wordSize = 1
        if(eepromType == 'M27C160'):
            adressMass = self.a160bits
            wordSize = 2
        elif(eepromType == 'M27C801'):
            adressMass = self.a801bits
        else:
            print 'Memory type is not exist'
            return None
        adressCount = 2**(len(adressMass))
        nRom = [0xFF for i in range(adressCount * wordSize)]
        for i in range(adressCount):
            fRomAdress = 0 
            for j in range(len(adressMass)):
                if(i & (1 << j)):
                    fRomAdress = fRomAdress | (1<< adressMass[j])
            if(eepromType == 'M27C160'):
                bytes = self.convertByteM27C160([ self.iRom[fRomAdress*2], self.iRom[fRomAdress*2 + 1] ])
                nRom[i*2] = bytes[0]
                nRom[i*2+1] = bytes[1]
            elif(eepromType == 'M27C801'):
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
        fBrowser = QFileDialog()
        filePath = fBrowser.getOpenFileName(self.mainWidget, \
            "Open Bin", \
            "", \
            "Bin file *.smc *.sfc *.bin");
        binFile = QFile(filePath[0])
        if binFile.open(QIODevice.ReadOnly):
            return binFile.readData(128)              
            

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
        self.initConsole()
        mWid.show()
        mWid.scrollArea.hide()   

if __name__ == '__main__':
    Aplication().exec_()
    
