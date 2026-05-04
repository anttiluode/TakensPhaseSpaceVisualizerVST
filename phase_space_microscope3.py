#!/usr/bin/env python3
"""
Pepsi Can Live Takens Attractor with Sculptable EQ Line
========================================================
- Real-time 2D phase space (Takens embedding)
- 10-band graphical EQ with sculptable line display
- Drag the line to shape the filter profile
- Visual feedback shows exactly what frequencies pass through
"""

import numpy as np
import sounddevice as sd
import tkinter as tk
from tkinter import ttk
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from collections import deque
from scipy.signal import butter, sosfilt

sd._initialize()

class SculptableEQ:
    """10-band EQ with a sculptable line display"""
    def __init__(self, parent, fs=44100, callback=None):
        self.fs = fs
        self.callback = callback
        self.num_bands = 10
        self.bands = [
            (20, 60, "Sub"), (60, 150, "Bass"), (150, 300, "LoMid"),
            (300, 600, "Mid"), (600, 1200, "HiMid"), (1200, 2400, "Pres"),
            (2400, 4000, "Treble"), (4000, 7000, "Air1"),
            (7000, 12000, "Air2"), (12000, 20000, "Air3")
        ]
        self.gains = [0.5] * 10
        self.sliders = []
        self.labels = []
        self.canvas = None
        self.line_points = []
        
        container = tk.Frame(parent, bg="#0f0f23")
        container.pack(fill="x", pady=(10, 5), padx=10)
        
        tk.Label(container, text="10-BAND EQ — drag sliders to sculpt", fg="#00ffcc",
                 bg="#0f0f23", font=("Consolas", 10, "bold")).pack(anchor="w", pady=(0, 3))
        
        # EQ line display (Canvas)
        self.canvas = tk.Canvas(container, bg="#0a0a1a", height=80,
                                highlightthickness=1, highlightbackground="#222244")
        self.canvas.pack(fill="x", pady=(0, 5))
        self.canvas.bind("<B1-Motion>", self._on_line_drag)
        self.canvas.bind("<Button-1>", self._on_line_click)
        self._draw_eq_line()
        
        # Sliders in a single horizontal row
        slider_frame = tk.Frame(container, bg="#0f0f23")
        slider_frame.pack(fill="x")
        
        for i, (lo, hi, name) in enumerate(self.bands):
            col_frame = tk.Frame(slider_frame, bg="#0f0f23")
            col_frame.pack(side="left", fill="x", expand=True, padx=1)
            
            # Band name and frequency range
            tk.Label(col_frame, text=f"{name}", fg="#888888", bg="#0f0f23",
                     font=("Consolas", 6)).pack()
            tk.Label(col_frame, text=f"{lo}-{hi}Hz", fg="#555555", bg="#0f0f23",
                     font=("Consolas", 5)).pack()
            
            # Gain value display
            val_label = tk.Label(col_frame, text="0.50", fg="#ffaa00", bg="#0f0f23",
                                  font=("Consolas", 7, "bold"))
            val_label.pack()
            self.labels.append(val_label)
            
            # Vertical slider
            slider = tk.Scale(col_frame, from_=0.0, to=1.0, resolution=0.01,
                              orient="vertical", length=70,
                              command=lambda v, idx=i: self._on_slider(idx, v),
                              bg="#0f0f23", fg="#00ffcc", highlightthickness=0,
                              troughcolor="#16213e", activebackground="#00ffcc")
            slider.set(0.5)
            slider.pack()
            self.sliders.append(slider)
        
        # Preset buttons
        preset_frame = tk.Frame(container, bg="#0f0f23")
        preset_frame.pack(fill="x", pady=(5, 0))
        for name, gains in [
            ("Flat", [0.5]*10),
            ("Voice", [0.2, 0.3, 0.6, 0.8, 0.9, 1.0, 0.7, 0.4, 0.2, 0.1]),
            ("50Hz Only", [1.0, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1]),
            ("No 50Hz", [0.0, 0.3, 0.5, 0.6, 0.7, 0.8, 0.6, 0.4, 0.2, 0.1]),
            ("High Pass", [0.0, 0.0, 0.0, 0.2, 0.4, 0.6, 0.8, 0.9, 1.0, 1.0]),
            ("Sub Only", [0.9, 0.8, 0.4, 0.2, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0]),
        ]:
            tk.Button(preset_frame, text=name, bg="#16213e", fg="#888888",
                      font=("Consolas", 6), padx=2, pady=1, borderwidth=1,
                      command=lambda g=gains: self.set_preset(g)
                      ).pack(side="left", padx=1)

    def set_preset(self, gains):
        for i, g in enumerate(gains):
            self.sliders[i].set(g)
            self.gains[i] = g
            self.labels[i].config(text=f"{g:.2f}")
        self._draw_eq_line()
        if self.callback: self.callback()

    def _on_slider(self, idx, val):
        self.gains[idx] = float(val)
        self.labels[idx].config(text=f"{self.gains[idx]:.2f}")
        self._draw_eq_line()
        if self.callback: self.callback()

    def _draw_eq_line(self):
        if not self.canvas: return
        self.canvas.delete("all")
        w, h = self.canvas.winfo_width(), self.canvas.winfo_height()
        if w < 10:
            w = 600; h = 80
        
        # Grid lines
        for i in range(5):
            y = int(h * i / 4)
            self.canvas.create_line(0, y, w, y, fill="#1a1a3a", width=1)
        
        # Frequency labels at bottom
        freqs = [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000]
        for f in freqs:
            x = int(np.log10(f/20) / np.log10(20000/20) * w)
            self.canvas.create_text(x, h-8, text=f"{f}" if f<1000 else f"{f/1000:.0f}k",
                                     fill="#444466", font=("Consolas", 6))
        
        # Draw the sculpted line
        band_centers_hz = [np.sqrt(lo*hi) for lo, hi, _ in self.bands]
        xs = [int(np.log10(f/20) / np.log10(20000/20) * w) for f in band_centers_hz]
        ys = [int(h - g * (h-10) - 5) for g in self.gains]
        
        # Smooth curve through control points
        pts = list(zip(xs, ys))
        if len(pts) > 2:
            # Add edge points for smoothness
            pts.insert(0, (0, ys[0]))
            pts.append((w, ys[-1]))
            self.canvas.create_line(pts, fill="#00ffcc", width=2.5, smooth=True)
        
        # Control point handles
        for x, y, g in zip(xs, ys, self.gains):
            r = 4
            color = "#ffaa00" if g > 0.7 else ("#ff4444" if g < 0.2 else "#00ffcc")
            self.canvas.create_oval(x-r, y-r, x+r, y+r, fill=color, outline="white", width=1)
        
        self.line_points = list(zip(xs, ys))

    def _on_line_click(self, event):
        self._on_line_drag(event)

    def _on_line_drag(self, event):
        if not self.line_points: return
        w = self.canvas.winfo_width()
        if w < 10: return
        # Find nearest band to click position
        band_centers_hz = [np.sqrt(lo*hi) for lo, hi, _ in self.bands]
        xs_ideal = [np.log10(f/20) / np.log10(20000/20) * w for f in band_centers_hz]
        dists = [abs(event.x - x) for x in xs_ideal]
        idx = min(range(len(dists)), key=lambda i: dists[i])
        if dists[idx] < 30:
            h = self.canvas.winfo_height()
            if h < 10: h = 80
            gain = max(0.0, min(1.0, 1.0 - event.y / h))
            self.sliders[idx].set(gain)
            self.gains[idx] = gain
            self.labels[idx].config(text=f"{gain:.2f}")
            self._draw_eq_line()
            if self.callback: self.callback()

    def apply(self, audio):
        filtered = np.zeros_like(audio)
        for i, (lo, hi, _) in enumerate(self.bands):
            if self.gains[i] < 0.005:
                continue
            try:
                nyq = self.fs / 2
                lo_norm = max(lo / nyq, 0.001)
                hi_norm = min(hi / nyq, 0.999)
                sos = butter(2, [lo_norm, hi_norm], btype='band', output='sos')
                band = sosfilt(sos, audio)
                filtered += band * self.gains[i] * 2.0
            except:
                pass
        return filtered


class LiveTakensAttractor:
    def __init__(self, root):
        self.root = root
        self.root.title("Pepsi Can — Live Takens Attractor + Sculptable EQ")
        self.root.geometry("1500x920")
        self.root.configure(bg="#0a0a1a")

        self.fs = 44100
        self.block_size = 1024
        self.ring_buffer = deque(maxlen=8000)
        self.filtered_buffer = deque(maxlen=8000)
        self.running = False
        self.stream = None
        self.eq_enabled = True

        self.embedding_delay = 15
        self.display_points = 3000
        self.frozen = False
        self.frozen_x = None
        self.frozen_y = None

        self.input_devices = []
        for i, dev in enumerate(sd.query_devices()):
            if dev['max_input_channels'] > 0:
                api = sd.query_hostapis()[dev['hostapi']]['name']
                self.input_devices.append(f"[{i}] {dev['name']} [{api}]")

        self._build_gui()
        self._update_plot_loop()

    def _build_gui(self):
        top = tk.Frame(self.root, bg="#16213e", height=45)
        top.pack(fill="x", side="top")

        tk.Label(top, text="LIVE TAKENS ATTRACTOR + SCULPTABLE EQ", font=("Consolas", 15, "bold"),
                 fg="#00ffcc", bg="#16213e").pack(side="left", padx=15, pady=8)
        tk.Label(top, text="Input:", fg="#888888", bg="#16213e",
                 font=("Consolas", 9)).pack(side="left", padx=(15, 5))
        self.device_var = tk.StringVar()
        self.device_combo = ttk.Combobox(top, textvariable=self.device_var,
                                          values=self.input_devices, width=45)
        if self.input_devices:
            self.device_combo.current(0)
        self.device_combo.pack(side="left", padx=5)
        self.btn_start = tk.Button(top, text="START", bg="#0a4a0a", fg="white",
                                    font=("Consolas", 10, "bold"), command=self.toggle_stream)
        self.btn_start.pack(side="left", padx=15)
        self.btn_freeze = tk.Button(top, text="FREEZE", bg="#444444", fg="white",
                                     font=("Consolas", 10), command=self.toggle_freeze)
        self.btn_freeze.pack(side="left", padx=3)
        self.btn_eq = tk.Button(top, text="EQ ON", bg="#004466", fg="white",
                                 font=("Consolas", 10), command=self.toggle_eq)
        self.btn_eq.pack(side="left", padx=3)
        self.status_var = tk.StringVar(value="Select input, press START, sculpt the EQ line")
        tk.Label(top, textvariable=self.status_var, fg="#888888", bg="#16213e",
                 font=("Consolas", 8)).pack(side="left", padx=20)

        left = tk.Frame(self.root, bg="#0f0f23", width=520)
        left.pack(side="left", fill="y", padx=8, pady=8)
        left.pack_propagate(False)

        tk.Label(left, text="DELAY τ (samples)", fg="#00ffcc", bg="#0f0f23",
                 font=("Consolas", 10, "bold")).pack(pady=(10, 3))
        self.delay_var = tk.IntVar(value=15)
        tk.Scale(left, from_=1, to=80, orient="horizontal", variable=self.delay_var,
                 command=self._on_delay_change, bg="#0f0f23", fg="#00ffcc",
                 length=340).pack(padx=10)
        self.delay_label = tk.Label(left, text="15 samples (0.34 ms)",
                                     fg="#ffaa00", bg="#0f0f23", font=("Consolas", 10))
        self.delay_label.pack()

        tk.Label(left, text="DISPLAY POINTS", fg="#00ffcc", bg="#0f0f23",
                 font=("Consolas", 10, "bold")).pack(pady=(10, 3))
        self.points_var = tk.IntVar(value=3000)
        tk.Scale(left, from_=500, to=8000, orient="horizontal", variable=self.points_var,
                 bg="#0f0f23", fg="#00ffcc", length=340).pack(padx=10)

        tk.Label(left, text="INPUT GAIN", fg="#00ffcc", bg="#0f0f23",
                 font=("Consolas", 10, "bold")).pack(pady=(10, 3))
        self.gain_var = tk.DoubleVar(value=1.0)
        tk.Scale(left, from_=0.1, to=10.0, resolution=0.1, orient="horizontal",
                 variable=self.gain_var, bg="#0f0f23", fg="#00ffcc",
                 length=340).pack(padx=10)

        # SCULPTABLE EQ
        self.eq = SculptableEQ(left, fs=self.fs, callback=lambda: None)

        right = tk.Frame(self.root, bg="#0a0a1a")
        right.pack(side="right", fill="both", expand=True, padx=(0, 8), pady=8)

        self.fig = Figure(figsize=(9, 8), facecolor="#0a0a1a")
        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor("#0a0a1a")
        self.ax.set_title("Phase Space: S(t) vs S(t+τ)", color="#00ffcc", fontsize=12)
        self.ax.set_xlabel("S(t)", color="#888888")
        self.ax.set_ylabel("S(t+τ)", color="#888888")
        self.ax.tick_params(colors="#666666")
        self.ax.grid(True, alpha=0.15, color="#444444")
        self.scatter = self.ax.scatter([], [], c=[], cmap="plasma",
                                        s=2, alpha=0.7, vmin=0, vmax=1)
        self.canvas = FigureCanvasTkAgg(self.fig, master=right)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)
        self.canvas.draw()

    def _on_delay_change(self, val):
        self.embedding_delay = int(float(val))
        tau_ms = self.embedding_delay / self.fs * 1000
        self.delay_label.config(text=f"{self.embedding_delay} samples ({tau_ms:.2f} ms)")

    def toggle_freeze(self):
        self.frozen = not self.frozen
        if self.frozen:
            self.btn_freeze.config(text="THAW", bg="#ff4444")
            self.status_var.set("FROZEN — geometry captured")
        else:
            self.btn_freeze.config(text="FREEZE", bg="#444444")
            self.frozen_x = None; self.frozen_y = None
            self.status_var.set("LIVE")

    def toggle_eq(self):
        self.eq_enabled = not self.eq_enabled
        self.btn_eq.config(text="EQ ON" if self.eq_enabled else "EQ BYPASS",
                           bg="#004466" if self.eq_enabled else "#444444")

    def _audio_callback(self, indata, frames, time_info, status):
        if indata is not None and len(indata) > 0:
            mono = indata[:, 0] if indata.ndim > 1 else indata
            self.ring_buffer.extend(mono.tolist())
            if self.eq_enabled and len(mono) > 50:
                filtered = self.eq.apply(mono)
                self.filtered_buffer.extend(filtered.tolist())
            else:
                self.filtered_buffer.extend(mono.tolist())

    def toggle_stream(self):
        if not self.running: self._start_stream()
        else: self._stop_stream()

    def _start_stream(self):
        dev_str = self.device_var.get()
        try:
            dev_id = int(dev_str.split("]")[0].replace("[", "").strip())
        except:
            self.status_var.set("ERROR: Could not parse device ID")
            return
        self.ring_buffer.clear(); self.filtered_buffer.clear()
        self.running = True
        try:
            self.stream = sd.InputStream(device=dev_id, channels=1, samplerate=self.fs,
                                          blocksize=self.block_size, callback=self._audio_callback)
            self.stream.start()
        except Exception as e:
            self.status_var.set(f"ERROR: {e}")
            self.running = False
            return
        self.btn_start.config(text="STOP", bg="#8b0000")
        self.status_var.set("LIVE — Sculpt the EQ line, sweep τ, touch the can")

    def _stop_stream(self):
        self.running = False
        if self.stream: self.stream.stop(); self.stream.close(); self.stream = None
        self.btn_start.config(text="START", bg="#0a4a0a")
        self.status_var.set("Stopped")

    def _update_plot_loop(self):
        try:
            if self.running and not self.frozen:
                self._update_plot()
        except Exception as e:
            print(f"Plot error: {e}")
        self.root.after(50, self._update_plot_loop)

    def _update_plot(self):
        delay = self.embedding_delay
        buf = self.filtered_buffer if len(self.filtered_buffer) > delay else self.ring_buffer
        if len(buf) < delay + 10: return
        data = np.array(buf) * self.gain_var.get()
        x_raw, y_raw = data[delay:], data[:-delay]
        m = min(len(x_raw), len(y_raw))
        x_raw, y_raw = x_raw[:m], y_raw[:m]
        n = min(self.points_var.get(), len(x_raw))
        if n < 10: return
        step = max(1, len(x_raw)//n)
        x, y = x_raw[-n*step::step], y_raw[-n*step::step]
        colors = np.linspace(0, 1, len(x))
        self.scatter.set_offsets(np.c_[x, y])
        self.scatter.set_array(colors)
        if len(x) > 1:
            cx, cy = np.mean(x), np.mean(y)
            hr = max(np.std(x), np.std(y)) * 2.5
            if hr > 1e-9:
                self.ax.set_xlim(cx - hr, cx + hr)
                self.ax.set_ylim(cy - hr, cy + hr)
        rms = np.sqrt(np.mean(data[-1000:]**2)) if len(data) > 1000 else 0
        self.ax.set_title(
            f"S(t) vs S(t+τ)  |  τ={delay}  |  RMS={rms:.4f}  |  N={len(x)}  |  EQ={'ON' if self.eq_enabled else 'BYPASS'}",
            color="#00ffcc", fontsize=11)
        self.canvas.draw_idle()

    def run(self):
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self.root.mainloop()

    def _on_close(self):
        if self.running: self._stop_stream()
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = LiveTakensAttractor(root)
    app.run()