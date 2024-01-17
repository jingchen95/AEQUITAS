import sys
import os
import numpy

filePtr = open("test.txt", "r+")
filePtr1 = open("data.sh", "w+")
# filePtr2 = open("CP_Time.txt", "w+")
i = 0
j = 5

for line in filePtr:
    if '50000 ' in line:
        i += 1
        token=line.strip().split(' ')
        numtask = numpy.int64(token[0])
        TimeStart = numpy.int64(token[2])
        TimeEnd = numpy.int64(token[3])
        Time = float(token[4])
        filePtr1.write("python Energy.py ")
        filePtr1.write(str(TimeStart))
        filePtr1.write(" ")
        filePtr1.write(str(TimeEnd))
        filePtr1.write("; \n")
        filePtr1.write("echo \"%f\"; \n" %Time)
        # filePtr1.write("printf \\n; \n")
        if i % 2 == 0:
            filePtr1.write("\n\n")
    else:
        continue

filePtr.close()
filePtr1.close()