# Chimera 

Chimera is an educational tool designed for learning and experimentation with embedded systems, particularly using the ESP32 platform. This project aims to provide an extensible and modular framework for developing low-level applications, peripheral interactions, and network-based tools.

Im neither a low level embeded programmer nor a security reasercher this project is by no means complete or adherent to all best practices in either topic. I would be happy to recieve your contributions.

## Features
- Modular architecture with support for various peripherals
- ESP-IDF based development
- Integrated logging and debugging utilities
- Extensible component-based design

## Getting Started

### Prerequisites
- ESP-IDF installed ([Installation Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html))
- CMake & Ninja build system
- Python 3 and required dependencies

### Build & Flash
```sh
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

### Configuration
Use the ESP-IDF menuconfig tool to modify settings:
```sh
idf.py menuconfig
```

## License
This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Disclaimer
This project is an **educational tool** intended for learning and experimentation. **Neither the author nor any contributor is liable for any misuse** of this software. Users are responsible for ensuring compliance with all applicable laws and regulations.


