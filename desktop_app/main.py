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
        self.plot_widget = None
        self.plot_data = [np.array([]) for _ in range(4)]
        self.plot_timestamps = [np.array([]) for _ in range(4)]
        self.lines = []
        self.running = True
        self.continuous_mode = False

        # UI Layout
        port_frame = ttk.Frame(master)
        port_frame.pack(pady=5, fill=tk.X)

        self.port_label = ttk.Label(port_frame, text="Select Port:")
        self.port_label.pack(side=tk.LEFT, padx=(0, 5))        
        self.port_var = tk.StringVar()
        self.port_dropdown = ttk.Combobox(port_frame, textvariable=self.port_var)
        self.port_dropdown.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=(0, 5))
        self.update_ports()
        
        self.connect_button = ttk.Button(port_frame, text="Connect", command=self.connect_serial)
        self.connect_button.pack(side=tk.LEFT, padx=5)
        
        self.disconnect_button = ttk.Button(port_frame, text="Disconnect", command=self.disconnect_serial, state="disabled")
        self.disconnect_button.pack(side=tk.LEFT, padx=5)

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

        self.plot_button = ttk.Button(control_frame, text="Show Plot", command=self.toggle_plot)
        self.plot_button.pack(side=tk.LEFT, padx=5)

        self.data_text = tk.Text(master, height=10, width=100)
        self.data_text.pack(pady=10)

        self.refresh_ports_button = ttk.Button(master, text="Refresh Ports", command=self.update_ports)
        self.refresh_ports_button.pack(pady=5)

        self.master.after(50, self.process_events)

    def update_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_dropdown['values'] = ports
        if ports:
            self.port_dropdown.set(ports[0])

    def connect_serial(self):
        port = self.port_var.get()
        try:
            self.ser = serial.Serial(port, 115200, timeout=1)
            time.sleep(2)
            self.start_stop_button['state'] = "normal"
            self.connect_button['state'] = "disabled"
            self.disconnect_button['state'] = "normal"
            self.port_dropdown['state'] = "disabled"
            self.running = True
            self.read_thread = threading.Thread(target=self.read_serial)
            self.read_thread.start()
        except serial.SerialException as e:
            tk.messagebox.showerror("Connection Error", f"Failed to connect to {port}: {str(e)}")

    def disconnect_serial(self):
        if self.ser:
            self.running = False
            if self.continuous_mode:
                self.ser.write(b"STOP_CONTINUOUS\n")
                self.continuous_mode = False
            
            if hasattr(self, 'read_thread') and self.read_thread.is_alive():
                self.read_thread.join()
            
            self.ser.close()
            self.ser = None

        self.connect_button['state'] = "normal"
        self.disconnect_button['state'] = "disabled"
        self.start_stop_button['state'] = "disabled"
        self.port_dropdown['state'] = "normal"

    def toggle_continuous(self):
        if not self.ser:
            return

        self.continuous_mode = not self.continuous_mode
        if self.continuous_mode:
            self.ser.reset_input_buffer()
            self.ser.write(b"START_CONTINUOUS\n")
            self.start_stop_button.config(text="Stop Continuous")
        else:
            self.ser.write(b"STOP_CONTINUOUS\n")
            self.start_stop_button.config(text="Start Continuous")

    def read_serial(self):
        buffer = ""
        while self.running:
            if self.ser is None:
                break
            
            try:
                if self.ser.in_waiting:
                    data = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
                    if data:
                        buffer += data
                        while "\n" in buffer:
                            line, buffer = buffer.split("\n", 1)
                            self.process_data(line.strip())
            except serial.SerialException:
                break
            except AttributeError:
                break


    def toggle_plot(self):
        if self.plot_widget is None:
            pg.setConfigOptions(antialias=True)
            self.plot_widget = pg.PlotWidget()
            self.plot_widget.setBackground('w')
            self.plot_widget.setTitle('Measurement / Time')
            self.plot_widget.setLabel('left', 'Voltage (V)')
            self.plot_widget.setLabel('bottom', 'Time (s)')
            self.plot_widget.setWindowTitle('Voltage Plot')
            
            self.lines = [self.plot_widget.plot(pen=(i, 4), name=f'Channel {i+1}') for i in range(4)]
            self.plot_widget.show()
            self.plot_button.config(text="Hide Plot")
        else:
            self.plot_widget.close()
            self.plot_widget = None
            self.lines = []
            self.plot_button.config(text="Show Plot")

    def process_data(self, data):
        #print(f"Processing: {data}")
        
        channels = data.split(", ")
        display_data = []
        current_time = time.time()
        timestamp = time.strftime("[%H:%M:%S]")

        for ch in channels:
            try:
                parts = ch.split(": ")
                if len(parts) != 2:
                    continue
                ch_name, value = parts
                ch_index = int(ch_name.replace("CH_", "").strip())
                voltage = float(value.replace("V", "").replace(",", "").strip())

                if 0 <= ch_index < len(self.plot_data):
                    self.plot_data[ch_index] = np.append(self.plot_data[ch_index], voltage)
                    self.plot_timestamps[ch_index] = np.append(self.plot_timestamps[ch_index], current_time)

                    if self.channel_vars[ch_index].get():
                        display_data.append(f"CH_{ch_index}: {voltage:.4f}V")

            except Exception as e:
                print(f"=================>Error processing data: {ch} | {e}")

        if display_data:
            self.data_text.insert(tk.END, f"{timestamp} " + " | ".join(display_data) + "\n")
            self.data_text.see(tk.END)

    def process_events(self):
        self.qt_app.processEvents()

        if self.plot_widget is not None:
            for i, line in enumerate(self.lines):
                if self.channel_vars[i].get():
                    try:
                        min_length = min(len(self.plot_timestamps[i]), len(self.plot_data[i]))
                        self.plot_timestamps[i] = self.plot_timestamps[i][-min_length:]
                        self.plot_data[i] = self.plot_data[i][-min_length:]

                        if min_length > 0:
                            relative_times = self.plot_timestamps[i] - self.plot_timestamps[i][0]
                            line.setData(relative_times, self.plot_data[i])
                            line.show()
                        else:
                            line.setData([], [])
                    except Exception as e:
                        print(f"=================>Error updating CH_{i}: {e}")
                else:
                    line.hide()

        self.master.update_idletasks()

        if self.running:
            self.master.after(5, self.process_events)



def on_closing():
    if app.ser:
        app.disconnect_serial()
    root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = ArduDAQ_UI(root)
    root.protocol("WM_DELETE_WINDOW", on_closing)
    root.mainloop()
