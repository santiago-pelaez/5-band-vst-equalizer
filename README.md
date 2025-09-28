# 5-Band VST Equalizer

A professional 5-band parametric equalizer VST3/AU plugin built with JUCE framework, inspired by Ableton EQ Eight and FabFilter Pro-Q.

## Features

- **5 Parametric Bands** with frequency, gain, and Q controls
- **Band 1 (Low)**: Low cut (high-pass) and low shelf filters only  
- **Bands 2, 3, 4 (Mid)**: Full parametric (peak/notch, shelf, cut filters)
- **Band 5 (High)**: High cut (low-pass) and high shelf filters only
- Real-time spectrum analyzer display
- Draggable EQ nodes for intuitive control
- Preset management (save/load)
- DAW automation support
- Cross-platform (Windows/macOS)

## Technical Stack

- **Framework**: JUCE 7.x
- **Language**: C++
- **Plugin Formats**: VST3, AU, AAX
- **DSP**: Biquad filters with smooth parameter changes
- **GUI**: Custom OpenGL-accelerated interface

## Project Structure

```
5-band-vst-equalizer/
├── Source/
│   ├── PluginProcessor.h/cpp      # Main audio processing
│   ├── PluginEditor.h/cpp         # GUI interface
│   ├── DSP/
│   │   ├── FilterTypes.h          # Filter type enums and restrictions
│   │   ├── BiquadFilter.h/cpp     # Biquad filter implementation
│   │   └── EQBand.h/cpp           # Individual EQ band class
│   ├── GUI/
│   │   ├── EQDisplay.h/cpp        # Spectrum and EQ curve display
│   │   ├── BandControls.h/cpp     # Individual band control panels
│   │   └── PresetManager.h/cpp    # Preset save/load functionality
│   └── Utils/
│       ├── Parameters.h           # Parameter definitions
│       └── Constants.h            # Audio constants and defaults
├── JuceLibraryCode/               # JUCE framework files
├── Builds/                        # Platform-specific build files
├── Presets/                       # Factory and user presets
└── Documentation/                 # Technical documentation
```

## Build Instructions

### Prerequisites
- JUCE Framework 7.x
- Visual Studio 2022 (Windows) or Xcode 14+ (macOS)
- CMake 3.15+

### Building
1. Download and install JUCE from juce.com
2. Open the .jucer file in Projucer
3. Configure your target platforms (VST3, AU, etc.)
4. Export to your IDE and build

## Development Roadmap

- [x] Project setup and structure
- [ ] Core DSP implementation
- [ ] Band restriction logic
- [ ] GUI framework
- [ ] Real-time spectrum analyzer
- [ ] Parameter automation
- [ ] Preset management
- [ ] Testing and optimization

## License

Private repository - All rights reserved.