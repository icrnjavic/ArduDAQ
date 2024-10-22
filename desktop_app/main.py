import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import ttk
import threading
import time
import tkinter.messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import numpy as np

class ArduDAQ_UI:
    def __init__(self, master):
        self.master = master
        self.master.title("ArduDAQ desktop V0.1")
        self.ser = None
        port_frame = ttk.Frame(master)
        port_frame.pack(pady=5, fill=tk.X)
        
        self.port_label = ttk.Label(port_frame, text="Select Port:")
        self.port_label.pack(side=tk.LEFT, padx=(0, 5))
        
        self.port_var = tk.StringVar()
        self.port_dropdown = ttk.Combobox(port_frame, textvariable=self.port_var)
        self.port_dropdown.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=(0, 5))
        self.update_ports()
        
        self.connect_button = ttk.Button(port_frame, text="Connect", command=self.connect_serial)
        self.connect_button.pack(side=tk.LEFT)
        
        control_frame = ttk.Frame(master)
        control_frame.pack(pady=5, fill=tk.X)
        
        self.channel_vars = []
        self.channel_checkboxes = []
        for i in range(4):
            var = tk.BooleanVar()
            cb = ttk.Checkbutton(control_frame, text=f"Channel {i+1}", variable=var)
            cb.pack(side=tk.LEFT, padx=5)
            self.channel_vars.append(var)
            self.channel_checkboxes.append(cb)
        
        self.start_stop_button = ttk.Button(control_frame, text="Start Continuous", command=self.toggle_continuous, state="disabled")
        self.start_stop_button.pack(side=tk.LEFT, padx=5)        
        self.data_text = tk.Text(master, height=10, width=100)
        self.data_text.pack(pady=10)

        self.fig, self.ax = plt.subplots(figsize=(8, 4))
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.master)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=1)
        
        self.plot_data = [[] for _ in range(4)]
        self.plot_times = []
        self.lines = [self.ax.plot([], [], label=f'Channel {i+1}')[0] for i in range(4)]
        self.ax.set_xlabel('Time')
        self.ax.set_ylabel('Voltage')
        self.ax.legend()
        self.ax.set_ylim(0, 5)  # Adjust as needed
        
        # state variables
        self.continuous_mode = False
        self.running = False
        
        # refresh ports button
        self.refresh_button = ttk.Button(master, text="Refresh Ports", command=self.update_ports)
        self.refresh_button.pack(pady=5)
    
    def update_ports(self):
        # update available ports
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_dropdown['values'] = ports
        if ports:
            self.port_dropdown.set(ports[0])
    
    def connect_serial(self):
        # select serial port
        port = self.port_var.get()
        try:
            self.ser = serial.Serial(port, 115200, timeout=1)
            time.sleep(2)  # allow time for serial connection to establish
            self.start_stop_button['state'] = "normal"
            self.connect_button['state'] = "disabled"
            self.port_dropdown['state'] = "disabled"
            self.running = True
            self.read_thread = threading.Thread(target=self.read_serial)
            self.read_thread.start()
        except serial.SerialException as e:
            tk.messagebox.showerror("Connection Error", f"Failed to connect to {port}: {str(e)}")
    
    def toggle_continuous(self):
        # continuous mode toggle
        self.continuous_mode = not self.continuous_mode
        if self.continuous_mode:
            self.ser.write(b"START_CONTINUOUS\n")
            self.start_stop_button.config(text="Stop Continuous")
        else:
            self.ser.write(b"STOP_CONTINUOUS\n")
            self.start_stop_button.config(text="Start Continuous")
    
    def read_serial(self):
        # continuous reading from serial
        while self.running:
            if self.ser.in_waiting:
                line = self.ser.readline().decode('utf-8').strip()
                if line:
                    self.process_data(line)
            time.sleep(0.1)
    
    def process_data(self, data):
        channels = data.split(", ")
        display_data = []
        current_time = time.time()
        for i, channel in enumerate(channels):
            if i < len(self.channel_vars) and self.channel_vars[i].get():
                display_data.append(channel)
                try:
                    voltage = float(channel.split(": ")[1].replace("V", "").strip(','))
                    self.plot_data[i].append(voltage)
                    if len(self.plot_data[i]) > len(self.plot_times):
                        self.plot_times.append(current_time)
                except (IndexError, ValueError) as e:
                    print(f"Error processing channel {i}: {e}")
                    print(f"Raw data: {channel}")
        
        if display_data:
            self.data_text.insert(tk.END, " | ".join(display_data) + "\n")
            self.data_text.see(tk.END)
        
        self.update_plot()
    
    def update_plot(self):
        for i, line in enumerate(self.lines):
            if self.channel_vars[i].get():
                y_data = self.plot_data[i]
                x_data = range(len(y_data))
                line.set_data(x_data, y_data)
                line.set_visible(True)
            else:
                line.set_visible(False)
        
        self.ax.relim()
        self.ax.autoscale_view()
        self.canvas.draw()

    def on_closing(self):
        self.running = False
        if self.ser:
            if self.continuous_mode:
                self.ser.write(b"STOP_CONTINUOUS\n")
            self.ser.close()
        self.master.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = ArduDAQ_UI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()
