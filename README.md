# MagneticSmartStrap
<a id="readme-top"></a>

![PlatformIO](https://img.shields.io/badge/platformio-%23000.svg?style=for-the-badge&logo=platformio&logoColor=F5822A) 
![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white) 
![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![Python](https://img.shields.io/badge/python-3670A0?style=for-the-badge&logo=python&logoColor=ffdd54) 

Hall effect sensor used to read number of magnets imbedded in strap that have passed over as a positional reading to tell if strap is properly tightened. Three zones based off of distance from desiered postion. Zone information transmited over BLE to companion application. 

## Description

TO DO

## Deployment

All the following bash commands must be run either in PlatoformIO CLI via VS-Code extension or by adding platformIO to your path.

### Dependincies

The following tools are required to build and deploy this project:
- [PlatformIO](https://platformio.org/) (VS Code extension reccomended)
- Python 3.x (required by PlatformIO)

PlatformIO will install and manage:
- ESP32 RISC-V toolchain
- ESP-IDF framework
- Other Build dependencies

### Setup

Copy project:
```bash
git clone https://github.com/mmcgee3/MagneticSmartStrap
```

Modify [platformio.ini](platformio.ini) to set the your target board.

ex:
```ini
[env:esp32-c6-devkitc-1]
platform = espressif32
board = esp32-c6-devkitm-1
board_build.mcu = esp32c6
framework = espidf
```

### Build

```bash
pio run
```

Image output in /.pio/(target-board)

### Flashing

```bash
pio run -t upload
```

### Execute

TO DO

## Helpful Commands

Clean up current build:
```bash
pio run -t fullclean
```

## Author

TO DO

## Acknowledgments

<p align="right">(<a href="#readme-top">TOP</a>)</p>