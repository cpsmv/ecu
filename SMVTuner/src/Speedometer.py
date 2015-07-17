from PyQt5.QtGui import *
from PyQt5.QtCore import *
from PyQt5.QtWidgets import *

class Speedometer(QWidget):

    def __init__(self, title, unit, min, max, parent=None):
        QWidget.__init__(self, parent)
        self.min = min
        self.max = max
        self.speed = min
        self.displayPowerPath = True
        self.title = title
        self.power = 100.0 * (self.speed-self.min)/(self.max-self.min)

        self.powerGradient =  QConicalGradient(0, 0, 180)
        self.powerGradient.setColorAt(0, Qt.red)
        self.powerGradient.setColorAt(0.375, Qt.yellow)
        self.powerGradient.setColorAt(0.75, Qt.green)
        self.unitTextColor = QColor(Qt.gray)
        self.speedTextColor = QColor(Qt.black)
        self.powerPathColor = QColor(Qt.gray)
        self.unit = unit

    def setSpeed(self, speed):
        self.speed = speed
        self.power = 100.0 * (self.speed-self.min)/(self.max-self.min)
        self.update()

    def setUnit(self, unit):
        self.unit = unit

    def setPowerGradient(self, gradient):
        self.powerGradient = gradient

    def setDisplayPowerPath(self, displayPowerPath):
        self.displayPowerPath = displayPowerPath

    def setUnitTextColor(self, color):
        self.unitTextColor = color

    def setSpeedTextColor(self, color):
        self.speedTextColor = color

    def setPowerPathColor(self, color):
        self.powerPathColor = color

    def paintEvent(self, evt):
        x1 = QPoint(0, -70)
        x2 = QPoint(0, -90)
        x3 = QPoint(-90,0)
        x4 = QPoint(-70,0)
        extRect = QRectF(-90,-90,180,180)
        intRect = QRectF(-70,-70,140,140)
        midRect = QRectF(-44,-80,160,160)
        unitRect = QRectF(-44,60,110,50)

        speedInt = self.speed
        #speedDec = (self.speed * 10.0) - (speedInt * 10)
        s_SpeedInt = speedInt.__str__()[0:4]

        powerAngle = self.power * 270.0 / 100.0

        dummyPath = QPainterPath()
        dummyPath.moveTo(x1)
        dummyPath.arcMoveTo(intRect, 90 - powerAngle)
        powerPath = QPainterPath()
        powerPath.moveTo(x1)
        powerPath.lineTo(x2)
        powerPath.arcTo(extRect, 90, -1 * powerAngle)
        powerPath.lineTo(dummyPath.currentPosition())
        powerPath.arcTo(intRect, 90 - powerAngle, powerAngle)

        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        painter.translate(self.width() / 2, self.height() / 2)
        side = min(self.width(), self.height())
        painter.scale(side / 200.0, side / 200.0)

        painter.save()
        painter.rotate(-135)

        if self.displayPowerPath:
            externalPath = QPainterPath()
            externalPath.moveTo(x1)
            externalPath.lineTo(x2)
            externalPath.arcTo(extRect, 90, -270)
            externalPath.lineTo(x4)
            externalPath.arcTo(intRect, 180, 270)

            painter.setPen(self.powerPathColor)
            painter.drawPath(externalPath)

        painter.setBrush(self.powerGradient)
        painter.setPen(Qt.NoPen)
        painter.drawPath(powerPath)
        painter.restore()
        painter.save()

        painter.translate(QPointF(0, -50))

        painter.setPen(self.unitTextColor)
        fontFamily = self.font().family()
        unitFont = QFont(fontFamily, 18)
        painter.setFont(unitFont)
        painter.drawText(unitRect, Qt.AlignCenter, "{}".format(self.unit))

        painter.restore()

        painter.setPen(self.unitTextColor)
        fontFamily = self.font().family()
        unitFont = QFont(fontFamily, 18)
        painter.setFont(unitFont)
        painter.drawText(unitRect, Qt.AlignCenter, "{}".format(self.title))

        speedColor = QColor(0,0,0)
        speedFont = QFont(fontFamily, 48)
        fm1 = QFontMetrics(speedFont)
        speedWidth = fm1.width(s_SpeedInt)

        #speedDecFont = QFont(fontFamily, 23)
        #fm2 = QFontMetrics(speedDecFont)
        #speedDecWidth = fm2.width(s_SpeedDec)

        leftPos = -1 * speedWidth + 50
        leftDecPos = leftPos + speedWidth
        topPos = 10
        topDecPos = 10
        painter.setPen(self.speedTextColor)
        painter.setFont(speedFont)
        painter.drawText(leftPos, topPos, s_SpeedInt)