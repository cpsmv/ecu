from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
from PyQt5 import QtCore


from ui_mainwindow import Ui_MainWindow
import sys, os, glob
import pickle
from TablePrototype import TableWindow, TableModel
from TablePrototype import ModelVE, ModelSA
from Speedometer import Speedometer
import serial

class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()

        # Set up the user interface from Designer.
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.ui.actionVolumetric_Efficiency.triggered.connect(self.openVE)
        self.ui.actionSpark_Advance.triggered.connect(self.openSA)

        self.currentport = ""

        layout = QGridLayout(self.ui.centralwidget)
        self.ui.centralwidget.setLayout(layout)

        try:
            self.vetable = pickle.load(open("tuningve.smv", "rb"))
        except FileNotFoundError:
            print("No existing tuning found!")
            self.vetable = ModelVE()

        try:
            self.satable = pickle.load(open("tuningsa.smv", "rb"))
        except FileNotFoundError:
            print("No existing tuning found!")
            self.satable = ModelSA()

        self.vemodel = TableModel(self.vetable)
        self.vewindow = TableWindow("Volumetric Efficiency Table")
        self.vewindow.setModel(self.vemodel)

        self.samodel = TableModel(self.satable)
        self.sawindow = TableWindow("Spark Advance Table")
        self.sawindow.setModel(self.samodel)

        self.value = 0
        self.meters = []
        for i in range(0,3):
            for k in range(0,2):
                spd = Speedometer("testlol", "units", 0, 100)
                self.meters.append(spd)
                layout.addWidget(spd, k, i)

        ports = self.serial_ports()
        if len(ports):
            self.ui.menuSerial_Port.clear()
            self.actiongroup = QActionGroup(self.ui.menuSerial_Port)
            for port in ports:
                action = QAction(port, self.ui.menuSerial_Port)
                action.setCheckable(True)
                self.actiongroup.addAction(action)
            self.actiongroup.actions()[0].setChecked(True)
            self.setSerialPort()
            self.ui.menuSerial_Port.addActions(self.actiongroup.actions())
            self.actiongroup.triggered.connect(self.setSerialPort)

        QTimer.singleShot(20, self.increment)
                

    def setSerialPort(self):
        portaction = self.actiongroup.checkedAction()
        self.currentport = portaction.text()
        print("set current port to %s" % self.currentport)
        

    def increment(self):
        self.value = (self.value + 1) % 101
        for spd in self.meters:
            spd.setSpeed(self.value)
        QTimer.singleShot(50, self.increment)

    def openVE(self):
        self.vewindow.show()

    def openSA(self):
        self.sawindow.show()

    def closeEvent(self, evt):
        reply = QMessageBox.question(self, "Preparing to exit", "Save changes before exit?", QMessageBox.Yes | QMessageBox.No)
        if reply == QMessageBox.Yes:
            pickle.dump(self.vetable, open("tuningve.smv", "wb"))
            pickle.dump(self.satable, open("tuningsa.smv", "wb"))
        evt.accept()
 
    def serial_ports(self):
        if sys.platform.startswith('win'):
            ports = ['COM' + str(i + 1) for i in range(256)]

        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
            # this is to exclude your current terminal "/dev/tty"
            ports = glob.glob('/dev/tty[A-Za-z]*')

        elif sys.platform.startswith('darwin'):
            ports = glob.glob('/dev/tty.*')

        else:
            raise EnvironmentError('Unsupported platform')

        result = []
        for port in ports:
            try:
                s = serial.Serial(port)
                s.close()
                result.append(port)
                print("found %s" % port)
            except (OSError, serial.SerialException):
                pass
        return result