import os
import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import ttk
import threading
import time
import tkinter.messagebox
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore
import numpy as np
from PyQt5.QtWidgets import QApplication

class ArduDAQ_UI:
    def __init__(self, master):
        self.qt_app = QApplication([])
        self.master = master
        self.master.title("ArduDAQ desktop V0.1")
        self.ser = None
        
        # Initialize plot-related variables but don't create window yet
        self.plot_widget = None
        self.plot_data = [np.array([]) for _ in range(4)]
        self.lines = []
        
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
        
        # Add plot control button
        self.plot_button = ttk.Button(control_frame, text="Show Plot", command=self.toggle_plot)
        self.plot_button.pack(side=tk.LEFT, padx=5)
        
        self.data_text = tk.Text(master, height=10, width=100)
        self.data_text.pack(pady=10)
        
        # Add time window selection frame
        time_window_frame = ttk.Frame(control_frame)
        time_window_frame.pack(side=tk.LEFT, padx=5)
        
        ttk.Label(time_window_frame, text="Time Window:").pack(side=tk.LEFT)
        self.time_window_var = tk.StringVar(value="10")  # Default 10 seconds
        time_window_dropdown = ttk.Combobox(time_window_frame, 
                                          textvariable=self.time_window_var,
                                          values=["10", "20", "30"],
                                          width=3,
                                          state="readonly")
        time_window_dropdown.pack(side=tk.LEFT)
        ttk.Label(time_window_frame, text="s").pack(side=tk.LEFT)
        
        # Add timestamp tracking for each data point
        self.plot_timestamps = [np.array([]) for _ in range(4)]
        
        # Start periodic updates
        self.running = True
        self.master.after(50, self.process_events)
        
        # state variables
        self.continuous_mode = False
        
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
    
    def toggle_plot(self):
        if self.plot_widget is None:
            # Create plot widget
            pg.setConfigOptions(antialias=True)
            self.plot_widget = pg.PlotWidget()
            self.plot_widget.setBackground('w')
            self.plot_widget.setTitle('Voltage vs Time')
            self.plot_widget.setLabel('left', 'Voltage (V)')
            self.plot_widget.setLabel('bottom', 'Time (s)')
            self.plot_widget.setWindowTitle('Voltage Plot')
            
            # Set x-axis range to match the selected time window
            self.plot_widget.setXRange(0, float(self.time_window_var.get()))
            
            self.lines = [self.plot_widget.plot(pen=(i, 4), name=f'Channel {i+1}') for i in range(4)]
            self.plot_widget.show()
            self.plot_button.config(text="Hide Plot")
        else:
            self.plot_widget.close()
            self.plot_widget = None
            self.lines = []
            self.plot_button.config(text="Show Plot")
    
    def process_data(self, data):
        channels = data.split(", ")
        display_data = []
        current_time = time.time()  # Get current Unix timestamp
        timestamp = time.strftime("[%H:%M:%S]")
        
        for i, channel in enumerate(channels):
            if i < len(self.channel_vars) and self.channel_vars[i].get():
                display_data.append(channel)
                try:
                    voltage = float(channel.split(": ")[1].replace("V", "").strip(','))
                    self.plot_data[i] = np.append(self.plot_data[i], voltage)
                    self.plot_timestamps[i] = np.append(self.plot_timestamps[i], current_time)
                    
                    # Keep only data within the selected time window
                    time_window = float(self.time_window_var.get())
                    mask = self.plot_timestamps[i] >= (current_time - time_window)
                    self.plot_data[i] = self.plot_data[i][mask]
                    self.plot_timestamps[i] = self.plot_timestamps[i][mask]
                    
                except (IndexError, ValueError) as e:
                    print(f"Error processing channel {i}: {e}")
                    print(f"Raw data: {channel}")
        
        if display_data:
            self.data_text.insert(tk.END, f"{timestamp} " + " | ".join(display_data) + "\n")
            self.data_text.see(tk.END)
    
    def process_events(self):
        # Process Qt events
        self.qt_app.processEvents()
        
        # Update plot if it exists and is visible
        if self.plot_widget is not None:
            current_time = time.time()
            for i, line in enumerate(self.lines):
                if self.channel_vars[i].get():
                    # Calculate relative time in seconds from the start of the window
                    relative_times = self.plot_timestamps[i] - current_time + float(self.time_window_var.get())
                    line.setData(relative_times, self.plot_data[i])
                    line.show()
                else:
                    line.hide()
        
        # Schedule next update if still running
        if self.running:
            self.master.after(50, self.process_events)

    def on_closing(self):
        self.running = False
        if self.ser:
            if self.continuous_mode:
                self.ser.write(b"STOP_CONTINUOUS\n")
            self.ser.close()
        if self.plot_widget is not None:
            self.plot_widget.close()
        self.qt_app.quit()
        self.master.destroy()

if __name__ == "__main__":
    # Force X11 backend before creating any Qt objects
    os.environ["QT_QPA_PLATFORM"] = "xcb"
    os.environ["XDG_SESSION_TYPE"] = "x11"
    
    root = tk.Tk()
    app = ArduDAQ_UI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()
