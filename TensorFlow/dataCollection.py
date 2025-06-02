import serial
import time
import os
import csv
from datetime import datetime

ser = serial.Serial("/dev/cu.usbmodem2101", 115200, timeout=1)
time.sleep(2) 

def sendMessage(message):
    if(message[-1] != '\n'):
        message = message + '\n'
        ser.write(message.encode('utf-8'))

def logData(folder='./TensorFlow/Data/dapup', prefix='dapup', num_points=100):
    print(f"Collecting {num_points} data points")
    for i in range(3, 0, -1):
        print(f"{i}")
        time.sleep(1)

    sendMessage("bigData")
    ser.reset_input_buffer()

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = os.path.join(folder, f"{prefix}_{timestamp}_{num_points}.csv")

    with open(filename, mode='w', newline='') as file:
        header = ['lin_acc_x', 'lin_acc_y', 'lin_acc_z', 'gyro_x', 'gyro_y', 'gyro_z', 'quat_w', 'quat_x', 'quat_y', 'quat_z']
        writer = csv.writer(file)
        writer.writerow(header)
        points_collected = 0 

        try:
            while points_collected < num_points:
                line = ser.readline().decode().strip()
                if line:
                    parts = line.split(',')
                    if len(parts) == 10:
                        try:
                            row = [float(p) for p in parts]
                            writer.writerow(row)
                            points_collected += 1
                            if points_collected % 50 == 0:
                                print(f"Progress: {points_collected}/{num_points} points")

                        except ValueError:
                            print("Fake news")
        
        except serial.SerialException as e:
            print(f"Serial error: {e}")
            return None
        
        except Exception as e:
            print(f"Error: {e}")
            return None
        finally:
            ser.close()
    
    print(f"Successfully collected {prefix} data")

def main():
    logData(num_points=300)
    
if __name__ == "__main__":
    main()