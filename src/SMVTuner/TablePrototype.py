from PyQt5.QtWidgets import QMainWindow, QApplication, QHeaderView, QMessageBox, QHBoxLayout, QTableView
from PyQt5.QtCore import QAbstractTableModel, QVariant, Qt, QSize, QModelIndex
from ui_tablewindow import Ui_MainWindow
from PyQt5.QtGui import QBrush
import sys
import pickle

class TableWindow(QMainWindow):
    def __init__(self, title):
        super(TableWindow, self).__init__()

        # Set up the user interface from Designer.
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.setWindowTitle(title)

        self.layout = QHBoxLayout(self.ui.centralwidget)
        self.ui.windowTitle = title
        self.ui.centralwidget.setLayout(self.layout)
        self.tableView = QTableView()
        self.layout.addWidget(self.tableView)
        self.tableView.setHorizontalHeader(MyHeaderView(Qt.Horizontal))
        self.tableView.setVerticalHeader(MyHeaderView(Qt.Vertical))

    def setModel(self, model):
        self.tableView.setModel(model)
        margins = self.layout.contentsMargins()
        self.resize((
            margins.left() + margins.right() + self.tableView.frameWidth() * 2 + self.tableView.verticalHeader().width() + self.tableView.horizontalHeader().length()),
            self.height())
        

class TableModel(QAbstractTableModel):
    def __init__(self, table, parent=None):
        super(TableModel, self).__init__(parent)
        self.table = table
        self.highlighted = []

    def rowCount(self, parent):
        return len(self.table.yaxis)

    def columnCount(self, parent):
        return len(self.table.xaxis)

    def data(self, index, role):
        if not index.isValid():
            return QVariant()
        if role == Qt.DisplayRole:
            return QVariant(self.table.data[index.row()][index.column()])
        elif role == Qt.BackgroundColorRole:
            for spot in self.highlighted:
                if spot[0] == index.row() and spot[1] == index.column():
                    return QBrush(Qt.red)
        elif role == Qt.TextAlignmentRole:
            return Qt.AlignCenter
        return QVariant()

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal:
                return QVariant(self.table.xaxis[section])
            elif orientation == Qt.Vertical:
                return QVariant(self.table.yaxis[section])
            return QVariant()
        elif role == Qt.TextAlignmentRole:
            return Qt.AlignCenter
        return QVariant()

    def flags(self, index):
        return Qt.ItemIsEditable | Qt.ItemIsEnabled | Qt.ItemIsSelectable

    def setData(self, index, value, role=Qt.EditRole):
        if index.isValid():
            try:
                int(value)
                self.table.data[index.row()][index.column()] = value
                return True
            except:
                return False
            
            return True
        else:
            return False

    def setHighlight(self, x, y):
        self.highlighted = []
        xhighlight = -1
        yhighlight = -1
        for val in self.table.xaxis:
            if x >= val:
                xhighlight = xhighlight + 1

        for val in self.table.yaxis:
            if y >= val:
                yhighlight = yhighlight + 1 
        self.highlighted.append((yhighlight, xhighlight))
        self.highlighted.append((yhighlight+1, xhighlight))
        self.highlighted.append((yhighlight, xhighlight+1))
        self.highlighted.append((yhighlight+1, xhighlight+1))

class ModelVE:
    def __init__(self):
        self.xaxis = [1000, 1050, 1101, 1401, 2001, 2601, 3101, 3700, 4300, 4900, 5400, 6000, 6500, 7000, 7200, 7500]
        self.yaxis = [30.1, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 98, 100]
        self.data = [
            [28, 30, 30, 37, 36, 36, 36, 36, 35, 35, 35, 35, 34, 34, 34, 34],
            [31, 31, 31, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38],
            [31, 31, 31, 39, 39, 39, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42],
            [32, 32, 32, 40, 40, 41, 41, 42, 43, 43, 44, 44, 45, 45, 46, 46],
            [32, 33, 33, 41, 42, 42, 43, 44, 45, 46, 47, 48, 48, 49, 49, 50],
            [33, 33, 34, 39, 40, 41, 45, 46, 48, 49, 50, 51, 52, 53, 53, 54],
            [33, 34, 35, 31, 32, 34, 47, 48, 50, 51, 53, 54, 55, 57, 57, 58],
            [34, 35, 36, 32, 33, 35, 49, 51, 52, 54, 56, 57, 59, 60, 61, 62],
            [35, 36, 37, 33, 35, 37, 51, 53, 55, 57, 59, 61, 62, 64, 65, 66],
            [35, 36, 38, 34, 36, 38, 52, 55, 57, 60, 62, 64, 66, 68, 69, 70],
            [36, 37, 38, 35, 37, 40, 54, 57, 60, 62, 65, 67, 70, 72, 73, 74],
            [36, 38, 37, 38, 43, 46, 48, 51, 54, 57, 68, 71, 73, 76, 76, 78],
            [68, 69, 68, 74, 82, 85, 86, 86, 89, 92, 91, 92, 89, 87, 91, 93],
            [68, 70, 69, 75, 81, 83, 84, 84, 87, 91, 90, 91, 88, 87, 91, 93],
            [69, 72, 75, 79, 82, 84, 86, 86, 88, 92, 91, 93, 90, 89, 94, 95],
            [69, 72, 76, 80, 83, 85, 86, 87, 90, 93, 92, 94, 92, 91, 95, 97]
        ]

class ModelSA:
    def __init__(self):
        self.xaxis = [1000, 1001, 1200, 1500, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000]
        self.yaxis = [20.1, 25, 30, 35, 40, 45, 50, 60, 70, 80, 90, 100]
        self.data = [
            [18.6, 19.2, 20.0, 20.8, 22.4, 24.3, 25.3, 27.0, 28.7, 29.5, 30.2, 31.0],
            [18.5, 19.0, 19.9, 20.7, 22.3, 24.2, 25.2, 26.9, 28.6, 29.3, 30.0, 37.0],
            [18.3, 18.9, 19.7, 20.6, 22.2, 24.1, 25.1, 26.8, 28.5, 29.2, 29.8, 30.5],
            [18.2, 18.7, 19.6, 20.4, 22.0, 23.9, 24.9, 26.6, 28.3, 29.0, 29.5, 30.2],
            [18.0, 18.6, 19.4, 20.3, 21.9, 23.8, 24.8, 26.5, 28.2, 28.8, 29.3, 29.9],
            [17.9, 18.4, 19.3, 20.1, 21.7, 23.6, 24.7, 26.4, 28.1, 28.6, 29.1, 29.7],
            [17.7, 18.3, 19.1, 20.0, 21.6, 23.5, 24.5, 26.2, 28.0, 28.5, 28.9, 29.4],
            [17.4, 18.0, 18.8, 19.7, 21.3, 23.2, 24.3, 26.0, 27.7, 28.1, 28.4, 28.9],
            [17.8, 18.4, 19.2, 20.1, 21.7, 23.7, 24.7, 26.4, 28.8, 28.5, 28.7, 29.0],
            [17.8, 18.4, 19.2, 20.1, 21.8, 23.7, 24.7, 26.5, 28.8, 29.0, 29.2, 29.4],
            [17.5, 18.1, 18.9, 19.8, 21.5, 23.4, 24.5, 26.2, 28.6, 28.7, 28.7, 28.8],
            [17.2, 17.8, 18.7, 19.5, 21.2, 23.1, 24.2, 25.9, 28.3, 28.3, 28.3, 28.3]
        ]

class MyHeaderView(QHeaderView):
    def __init__(self, orientation, parent=None):
        QHeaderView.__init__(self, orientation, parent)
        self.orientation = orientation

    def sizeHint(self):
        return QSize(60, 30)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = TableWindow()

    tablemodel.setHighlight(1200, 50)
    window.ui.tableView.setModel(tablemodel)
    window.show()
    sys.exit(app.exec_())
