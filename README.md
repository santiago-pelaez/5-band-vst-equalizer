# 5-Band VST Equalizer

A professional 5-band parametric equalizer VST3/AU plugin built with JUCE framework, inspired by Ableton EQ Eight and FabFilter Pro-Q.

## ✅ Current Features (September 2025)

- **✅ 5 Parametric Bands** with frequency (20Hz-20kHz), gain (-20dB to +20dB), and Q (0.1-20) controls
- **✅ Band Restrictions**: 
  - **Band 1 (Low)**: Low cut (high-pass) and low shelf filters only
  - **Bands 2, 3, 4 (Mid)**: Full parametric (peak/notch, shelf, cut filters)  
  - **Band 5 (High)**: High cut (low-pass) and high shelf filters only
- **✅ Professional GUI**: Color-coded controls (🟠 Gain, 🔵 Frequency, 🟡 Q) with rotary knobs and sliders
- **✅ Real-time Processing**: Live parameter updates with professional biquad filter DSP
- **✅ Toggle Bypass**: A/B comparison functionality
- **✅ DAW Automation**: Full parameter automation support via APVTS
- **✅ Cross-platform**: Windows support with VS Code + MSBuild workflow

## 🚧 In Development

- Filter type selectors (ComboBox per band)
- Real-time spectrum analyzer display  
- Interactive EQ curve visualization
- Professional visual styling (EQ Eight/Pro-Q inspired)
- Preset management system

## Technical Stack

- **Framework**: JUCE 7.x
- **Language**: C++
- **Plugin Formats**: VST3, AU, AAX
- **DSP**: Biquad filters with smooth parameter changes
- **GUI**: Custom OpenGL-accelerated interface

## Project Structure

```
5-band-vst-equalizer/
├── 5BandEQ/
│   ├── Source/
│   │   ├── PluginProcessor.h/cpp      # ✅ Main audio processing engine
│   │   ├── PluginEditor.h/cpp         # ✅ Parametric GUI interface  
│   │   ├── DSP/
│   │   │   ├── FilterTypes.h          # ✅ Filter type enums and band restrictions
│   │   │   ├── BiquadFilter.h         # ✅ Professional biquad filter DSP
│   │   │   └── EQBand.h               # ✅ Individual EQ band processing
│   │   └── Utils/
│   │       ├── Parameters.h           # ✅ Complete parameter system with APVTS
│   │       └── Constants.h            # ✅ Audio constants and frequency defaults
│   └── Builds/VisualStudio2022/       # ✅ Working build system
├── .vscode/                           # ✅ VS Code integration with tasks
├── IMPLEMENTATION.md                  # ✅ Detailed technical documentation  
└── README.md                          # This file
```

## Screenshots

### Current Interface (v0.7)
- **5-Band Layout**: Each band has dedicated Gain (vertical slider), Frequency and Q (rotary knobs)
- **Color-Coded Controls**: Intuitive orange/cyan/yellow color scheme
- **Real-time Updates**: Live parameter changes with smooth DSP processing
- **Professional Toggle Bypass**: A/B comparison functionality

*Note: GUI screenshots will be added as interface design progresses*

## Build Instructions

### Prerequisites

- **JUCE Framework 7.x** (configured and working)
- **Visual Studio 2022** (Windows) with C++ development tools
- **Git** for version control

### Quick Start

```powershell
# Clone the repository
git clone https://github.com/santiago-pelaez/5-band-vst-equalizer.git
cd 5-band-vst-equalizer

# Build using VS Code tasks (recommended)
# 1. Open in VS Code
# 2. Run task: "Build 5BandEQ (Release)" 
# 3. Launch: "Standalone Plugin\5BandEQ.exe"

# Or build with MSBuild directly
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 5BandEQ\Builds\VisualStudio2022\5BandEQ.sln /p:Configuration=Release /p:Platform=x64
```

### Output Formats
- **✅ Standalone Application**: For testing and development
- **🚧 VST3 Plugin**: Coming soon for DAW integration
- **🚧 AU Plugin**: macOS support planned

## Development Progress

### ✅ Phase 1: Core DSP (Completed)
- [x] Professional biquad filter implementation 
- [x] 5-band parametric EQ with band restrictions
- [x] Real-time parameter processing
- [x] Complete APVTS parameter system
- [x] Build system integration

### 🚧 Phase 2: GUI Development (70% Complete)  
- [x] Parametric interface with gain/frequency/Q controls
- [x] Color-coded professional layout
- [x] Real-time parameter binding and updates
- [x] Toggle bypass functionality
- [ ] Filter type selectors per band
- [ ] Professional visual styling (EQ Eight inspired)

### 📋 Phase 3: Advanced Features (Planned)
- [ ] Interactive frequency response curve
- [ ] Real-time spectrum analyzer  
- [ ] Preset management system
- [ ] VST3/AU plugin formats
- [ ] Professional visual design

## License

Private repository - All rights reserved.
