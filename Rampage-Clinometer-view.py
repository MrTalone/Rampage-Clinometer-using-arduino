#from digi.xbee.devices import XBeeDevice
import serial
import tkinter as tk
from tkinter import ttk
from PIL import ImageTk, Image
import os



megaBoard = serial.Serial('COM5', 9600)

root = tk.Tk()
root.title("Rampage Clinometer")
#root.geometry("600*300")

jeep_front = Image.open("jeep_front.jpg").resize((200, 200))
jeep_side = Image.open("jeep_side.jpg").resize((200, 200))

jeep_front_photo = ImageTk.PhotoImage(jeep_front)
jeep_side_photo = ImageTk.PhotoImage(jeep_side)

warning_roll_label = tk.Label(root, text="", fg="red", font=("Arial", 16))
warning_roll_label.grid(row=0, column=0, pady=5)

warning_pitch_label = tk.Label(root, text="", fg="red", font=("Arial", 16))
warning_pitch_label.grid(row=0, column=2, pady=5)

warning_distance_label = tk.Label(root, text="", fg="red", font=("Arial", 16))
warning_distance_label.grid(row=3, column=1, pady=5)


image_label_front = tk.Label(root, image=jeep_front_photo)
image_label_front.grid(row=1, column=0, padx=20, pady=20)

image_label_side = tk.Label(root, image=jeep_side_photo)
image_label_side.grid(row=1, column=2, padx=20, pady=20)


style = ttk.Style()
style.theme_use('default')

style.configure("success.Vertical.TProgressbar",thickness=50,
                troughcolor='white', background='green')

style.configure("danger.Vertical.TProgressbar", thickness=50,
                troughcolor='white', background='red')

distance_meter = ttk.Progressbar(root, orient="vertical", style='success.Vertical.TProgressbar',length=300, mode="determinate", maximum=300 )
distance_meter.grid(row=1, column=1, padx=20, pady=20)




def animate(roll, pitch, distance):
    
    global jeep_front_photo, jeep_side_photo
    
    rotate_front = jeep_front.rotate(roll, expand = True,fillcolor="white")
    rotate_side = jeep_side.rotate(pitch, expand = True, fillcolor="white")
    
    if roll > 40 or roll < -40:
        warning_roll_label.config(text="WARNING: Roll angle too high!")
        image_label_front.config(background="red")
    else:
        warning_roll_label.config(text="")
        image_label_front.config(background="white")

        
    if pitch > 40 or pitch < -40:
        warning_pitch_label.config(text="WARNING: Pitch angle too high!")
        image_label_side.config(background="red")

    else:
        warning_pitch_label.config(text="")
        image_label_side.config(background="white")

        
    if distance >= 250:
        distance_meter.config(style="danger.Vertical.TProgressbar")
        warning_distance_label.config(text="WARNING: Too close to object!")
    else:
        distance_meter.config(style = "success.Vertical.TProgressbar")
        warning_distance_label.config(text="")

    
    jeep_front_photo = ImageTk.PhotoImage(rotate_front)
    jeep_side_photo = ImageTk.PhotoImage(rotate_side)
    
    image_label_front.config(image=jeep_front_photo)
    image_label_side.config(image=jeep_side_photo)
    
    distance_meter["value"] = distance

def read_serial():
    while megaBoard.in_waiting:
        line = megaBoard.readline().decode(errors="ignore").strip()
        #line = input("enter roll, pitch, distance: ")

        print("Received", line)
        
        roll, pitch, distance = map(int, line.split(","))
        distance = 300 - distance
        print(distance)
        animate(roll, pitch, distance)
        

        
        root.after(150, read_serial)  

        '''
distance = 50
roll = 25
pitch = 25

animate(roll,pitch,distance)
'''
read_serial()


root.mainloop()
