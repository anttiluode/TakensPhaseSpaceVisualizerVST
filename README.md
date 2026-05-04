# Takens Phase Space Visualizer
A Real-Time Topological Window into Audio Dynamics

The Takens Phase Space Visualizer is a JUCE-based audio analysis plugin that transforms your incoming audio into a living, breathing geometric portrait. Unlike traditional oscilloscopes or spectrum analyzers, this plugin reveals the hidden topology of your sound by reconstructing its phase space manifold in real time.

By applying Takens' Embedding Theorem to your audio stream, the plugin unfolds the 1D waveform into a 2D phase portrait — revealing the underlying deterministic structure of even the most complex signals.

---

## 🔭 The Mathematics

### Takens' Embedding Theorem
Any dynamical system can be reconstructed from a single observed variable by plotting the signal against its time-delayed copy:

```
Phase Space: (S(t), S(t+τ))
```

This simple transformation exposes the attractor geometry hidden within your audio:

- Pure sine waves → perfect circles or ellipses  
- Noise → chaotic, space-filling clouds  
- Voice → structured, vowel-shaped trajectories  
- Drum hits → transient spikes radiating from the origin  
- Reverb tails → slow, decaying spirals  

### The τ Parameter (Embedding Delay)

The delay parameter τ acts as a microscope focus:

| τ value | What you see |
|--------|-------------|
| Very short (1–5 ms) | High-frequency detail, transients, sibilance |
| Medium (10–20 ms) | Voice formants, instrument body resonance |
| Long (40–80 ms) | Reverb tails, ambient decay, low-frequency structure |

---

## 🎛️ Controls

### Delay τ (ms)
**1.0 – 80.0 ms**

Adjusts the time offset between S(t) and S(t+τ).

- Lower values → zoom into high-frequency detail  
- Higher values → reveal long-term correlation structure  

### Input Gain
**0.1x – 10.0x**

Controls the signal level entering the phase space reconstruction.

### Display Points
**500 – 8000**

- Fewer points → cleaner, faster, transient-focused  
- More points → reveals long-term attractor structure  

### RMS Meter
Real-time level display.

---

## 📊 What You're Looking At

### The Grid
- Horizontal axis → Current sample S(t)  
- Vertical axis → Delayed sample S(t+τ)  
- Origin → Silence  

### Visual Patterns

| Pattern | Meaning |
|--------|--------|
| Perfect circle | Pure sine wave |
| Ellipse | Phase-shifted sine |
| Lissajous figure | Multiple frequencies |
| Dense spiral | Reverb decay |
| Structured cloud | Voice or instrument |
| Fuzz | Noise or distortion |
| Line | Highly correlated signal |
| Multiple lobes | Harmonics |

---

## 🎛️ DAW Integration

This plugin is fully transparent (no audio alteration).

Use it as:
- Insert effect  
- Visualization tool  
- Teaching aid  
- Sound design assistant  

---

## 🛠️ Building from Source

### Requirements
- CMake 3.15+
- C++17 compiler
- JUCE framework

### Build Steps
```bash
git clone [<your-repo-url>](https://github.com/anttiluode/TakensPhaseSpaceVisualizerVST/)
cd TakensPhaseSpace

git clone https://github.com/juce-framework/JUCE.git JUCE

cmake -B build
cmake --build build --config Release
```

### Output Locations

| Format | Path |
|-------|------|
| Standalone | build/.../Standalone/ |
| VST3 | build/.../VST3/ |

---

## 📦 Installation

### Windows
```
C:\Program Files\Common Files\VST3\
```

### macOS
```
/Library/Audio/Plug-Ins/VST3/
```

---

## 🎨 Interface

Resizable from **500×500 → 1500×1500**

---

## 📖 Theory

Takens' theorem (1981):

> “The attractor can be reconstructed from a single time series.”

Used in:
- EEG/ECG analysis  
- Finance  
- Chaos theory  

---

## 🔬 Applications

### Audio Engineering
- Detect phase issues  
- Analyze compression  
- Tune reverbs  

### Sound Design
- Explore harmonics  
- Understand distortion  
- Create visual textures  

### Education
- Teach nonlinear dynamics  
- Compare signals  

---

## 📁 Structure

```
TakensPhaseSpace/
├── CMakeLists.txt
├── PluginProcessor.*
├── PluginEditor.*
```

---

## 🧪 Tested With
- FL Studio  
- Ableton Live  
- REAPER  
- Standalone  

---

## 📄 License
MIT

---

## 🔮 Future
- 3D embedding  
- Density coloring  
- Dimension estimation  
- Export frames  

---

Created by PerceptionLab
