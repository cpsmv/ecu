# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui_mainwindow.ui'
#
# Created by: PyQt5 UI code generator 5.4.1
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName("MainWindow")
        MainWindow.resize(946, 595)
        self.centralwidget = QtWidgets.QWidget(MainWindow)
        self.centralwidget.setObjectName("centralwidget")
        MainWindow.setCentralWidget(self.centralwidget)
        self.menubar = QtWidgets.QMenuBar(MainWindow)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 946, 22))
        self.menubar.setObjectName("menubar")
        self.menuTables = QtWidgets.QMenu(self.menubar)
        self.menuTables.setObjectName("menuTables")
        self.menuFile = QtWidgets.QMenu(self.menubar)
        self.menuFile.setObjectName("menuFile")
        self.menuSettings = QtWidgets.QMenu(self.menubar)
        self.menuSettings.setObjectName("menuSettings")
        self.menuSerial_Port = QtWidgets.QMenu(self.menuSettings)
        self.menuSerial_Port.setObjectName("menuSerial_Port")
        MainWindow.setMenuBar(self.menubar)
        self.actionVolumetric_Efficiency = QtWidgets.QAction(MainWindow)
        self.actionVolumetric_Efficiency.setObjectName("actionVolumetric_Efficiency")
        self.actionSpark_Advance = QtWidgets.QAction(MainWindow)
        self.actionSpark_Advance.setObjectName("actionSpark_Advance")
        self.actionNo_Devices = QtWidgets.QAction(MainWindow)
        self.actionNo_Devices.setEnabled(False)
        self.actionNo_Devices.setObjectName("actionNo_Devices")
        self.actionOpen_Profile = QtWidgets.QAction(MainWindow)
        self.actionOpen_Profile.setObjectName("actionOpen_Profile")
        self.actionSave_Profile = QtWidgets.QAction(MainWindow)
        self.actionSave_Profile.setObjectName("actionSave_Profile")
        self.actionSave_Profile_As = QtWidgets.QAction(MainWindow)
        self.actionSave_Profile_As.setObjectName("actionSave_Profile_As")
        self.menuTables.addAction(self.actionVolumetric_Efficiency)
        self.menuTables.addAction(self.actionSpark_Advance)
        self.menuFile.addAction(self.actionOpen_Profile)
        self.menuFile.addAction(self.actionSave_Profile)
        self.menuFile.addAction(self.actionSave_Profile_As)
        self.menuSerial_Port.addAction(self.actionNo_Devices)
        self.menuSettings.addAction(self.menuSerial_Port.menuAction())
        self.menubar.addAction(self.menuFile.menuAction())
        self.menubar.addAction(self.menuTables.menuAction())
        self.menubar.addAction(self.menuSettings.menuAction())

        self.retranslateUi(MainWindow)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    def retranslateUi(self, MainWindow):
        _translate = QtCore.QCoreApplication.translate
        MainWindow.setWindowTitle(_translate("MainWindow", "Cal Poly SMV ECU Tuner"))
        self.menuTables.setTitle(_translate("MainWindow", "Tab&les"))
        self.menuFile.setTitle(_translate("MainWindow", "File"))
        self.menuSettings.setTitle(_translate("MainWindow", "Settings"))
        self.menuSerial_Port.setTitle(_translate("MainWindow", "Serial Port"))
        self.actionVolumetric_Efficiency.setText(_translate("MainWindow", "&Volumetric Efficiency"))
        self.actionSpark_Advance.setText(_translate("MainWindow", "&Spark Advance"))
        self.actionNo_Devices.setText(_translate("MainWindow", "No Devices"))
        self.actionOpen_Profile.setText(_translate("MainWindow", "Open Profile..."))
        self.actionSave_Profile.setText(_translate("MainWindow", "Save Profile"))
        self.actionSave_Profile_As.setText(_translate("MainWindow", "Save Profile As..."))

