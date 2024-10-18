import tkinter as tk
import serial
import threading
import hashlib
import os

# Initialize the serial connection
COM_PORT = '/dev/ttyUSB0'  # Update this to your actual COM port
BAUD_RATE = 9600

try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    exit(1)

# Rolling code variables
last_code = 0
code_increment = 1
max_code = 4294967295  # Maximum value for unsigned long

def send_command(command):
    command_with_crlf = f"{command}\r\n"  # Ensure CR and LF are included
    print(f"[SENT] - {command_with_crlf.strip()}")  # Debugging line
    try:
        ser.write(command_with_crlf.encode())
    except Exception as e:
        print(f"[ERROR] - {e}")

def read_from_lock_station():
    while True:
        if ser.in_waiting > 0:
            response = ser.readline().decode().strip()
            if response:
                print(f"[RECEIVED] - {response}")
                root.after(0, display_message, response)

def display_message(message):
    message_box.insert(tk.END, message + "\n")  # Display received message
    message_box.see(tk.END)  # Scroll to the bottom

def generate_send_command(uid):
    global last_code
    last_code += code_increment
    if last_code > max_code:
        last_code = 0  # Roll back to 0 if max reached
    return f"AT+SEND=0,100,{uid},{last_code}"

def send_message(slot):
    key = key_var.get()
    if key:  # Check if key is not empty
        payload_length = len(key) + 4  # +4 for the extra characters
        message = f"AT+SEND=0,{payload_length},{slot}-{key}"  # Properly formatted message
        send_command(message)
    else:
        print("[ERROR] - KEY CANNOT BE NULL")

def send_manual_command():
    manual_command = manual_command_var.get()
    if manual_command:  # Check if command is not empty
        send_command(manual_command)
    else:
        print("[ERROR] - COMMAND CANNOT BE NULL")

def on_closing():
    if ser.is_open:
        ser.close()
    root.destroy()

# Create the GUI
root = tk.Tk()
root.title("Home Station GUI")
root.protocol("WM_DELETE_WINDOW", on_closing)  # Handle window close event

# Frame for Key Slot Management
slot_frame = tk.Frame(root)
slot_frame.pack(pady=10)

# Input for the key
key_var = tk.StringVar()
key_entry = tk.Entry(slot_frame, textvariable=key_var)
key_entry.pack(side=tk.LEFT, padx=5)

# Buttons for sending messages with preconfigured templates
tk.Button(slot_frame, text="Send EXX", command=lambda: send_message("EXX")).pack(side=tk.LEFT)
tk.Button(slot_frame, text="Send EYX", command=lambda: send_message("EYX")).pack(side=tk.LEFT)
tk.Button(slot_frame, text="Send EZX", command=lambda: send_message("EZX")).pack(side=tk.LEFT)

# Frame for Manual AT Commands
manual_frame = tk.Frame(root)
manual_frame.pack(pady=10)

manual_command_var = tk.StringVar()
manual_command_entry = tk.Entry(manual_frame, textvariable=manual_command_var)
manual_command_entry.pack(side=tk.LEFT, padx=5)

tk.Button(manual_frame, text="Send Manual AT Command", command=send_manual_command).pack(side=tk.LEFT)

# Create a text box to display messages
message_box = tk.Text(root, height=10, width=50)
message_box.pack(pady=10)

# Start a thread to read from the lock station
def start_read_thread():
    threading.Thread(target=read_from_lock_station, daemon=True).start()

start_read_thread()

root.mainloop()
