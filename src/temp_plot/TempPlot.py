import numpy
import matplotlib.pyplot as mpl
import serial
import scipy
from scipy import stats
from sympy import Symbol
from sympy.solvers import solve
import sys
import itertools

class TempPlotter:

    def calibrateTempCurve(self):
        self.resistances = numpy.array([2.46, 0.318])
        self.temperatures = numpy.array([20, 80])
        with scipy.errstate(divide='ignore'):
            slope, intercept, r_value, p_value, std_err = \
                stats.linregress(self.temperatures, self.resistances)

        self.t = Symbol('t')
        self.a = Symbol('a')

        self.calibrationcurve = solve((intercept + slope*self.t) /
            (intercept + slope*self.t + 2.49) * 1024 - self.a, self.t)

    def inputDataFile(self, path):
        datafile = open(path)
        self.readData(datafile)

    def collectSerialData(self):
        s = serial.Serial("/dev/ttyACM0", 115200, timeout=2)
        s.write("q".encode())
        lines = s.readlines()
        self.readData(lines)
        s.close()

    def readData(self, rawdata):
        data = []
        for line in rawdata:
            try:
                inval = int(line)
                val = \
                    float(self.calibrationcurve[0].subs(self.a, inval).evalf())
                print(val)
                data.append(val * 9/5 + 32)
            except:
                #if "start" in line:
                #    print("found start")
                #elif "end" in line:
                #    print("found end")
                pass
        self.data = numpy.array(data)

    def getStats(self):
        print("Average: {}".format(numpy.average(self.data)))

    def plotData(self):
        mpl.plot(scipy.arange(0, len(self.data)*0.5, 0.5), self.data)
        mpl.show()


def main():
    tp = TempPlotter()
    tp.calibrateTempCurve()
    if len(sys.argv) > 1:
        tp.inputDataFile(sys.argv[1])
    else:
        tp.collectSerialData()
    tp.getStats()
    tp.plotData()

if __name__ == '__main__':
    main()
