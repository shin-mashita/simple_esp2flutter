import serial
import time
import numpy as np
import matplotlib.pyplot as plt

port = '/dev/ttyUSB0'
baudrate = 115200


x = np.linspace(50,2500, num=50)
fig, axs = plt.subplots(2,2)


y1 = []
y2 = []
y3 = []
y4 = []

if __name__=="__main__":
    ser = serial.Serial(port,baudrate)
    while(True):
        b = ser.read(ser.in_waiting)
        s = b.decode()

        
        s_list = s.split()

        if len(s_list) == 4:
            try:
                accx = float(s_list[0])
                accy = float(s_list[1])
                accz = float(s_list[2])
                dist = float(s_list[3])

                y1.append(accx)
                y2.append(accy)
                y3.append(accz)
                y4.append(dist)

                if len(y1)>len(x):
                    y1.pop(0)
                if len(y2)>len(x):
                    y2.pop(0)
                if len(y3)>len(x):
                    y3.pop(0)
                if len(y4)>len(x):
                    y4.pop(0)

                axs[0,0].cla()
                axs[0,1].cla()
                axs[1,0].cla()
                axs[1,1].cla()

                axs[0,0].plot(x[0:len(y1)],y1, color="blue")
                axs[0,1].plot(x[0:len(y2)],y2, color="orange")
                axs[1,0].plot(x[0:len(y3)],y3, color="red")
                axs[1,1].plot(x[0:len(y4)],y4, color="violet")

                axs[0,0].set_ylabel('Acceleration X (m/s2)')
                axs[0,1].set_ylabel('Acceleration Y (m/s2)')
                axs[1,0].set_ylabel('Acceleration Z (m/s2)')
                axs[1,1].set_ylabel('Distance (cm)')

                axs[0,0].set_xlabel('Time (ms)')
                axs[0,1].set_xlabel('Time (ms)')
                axs[1,0].set_xlabel('Time (ms)')
                axs[1,1].set_xlabel('Time (ms)')

                axs[0,0].set_ylim([-20,20])
                axs[0,1].set_ylim([-20,20])
                axs[1,0].set_ylim([-20,20])
                axs[1,1].set_ylim([0,500])

                plt.pause(0.2)
            except ValueError:
                print("ValueError")

            
        time.sleep(0.07)
