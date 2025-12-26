# Lotus Notebook

A lightweight Jupyter-like notebook application for Python development, built with C++ and Qt (qt5). Made by [Blue Lotus LAB](https://lotuschain.org)

![Lotus Notebook](assets/icon.svg)

## Overview

**Lotus Notebook** is a **desktop** application that provides an **interactive Python programming environment** with notebook-style cells, markdown support, and automatic backup functionality. It combines the ease of use of Jupyter notebooks with the performance and system integration of a **native C++** application.

## Features

### Core Functionality

- **Interactive Python Execution**: Run Python code in individual cells with immediate output display
- **Markdown Support**: Add documentation cells with markdown formatting
- **Cell Management**: Insert, delete, move, and edit cells easily
- **Variable Persistence**: Variables persist between cell executions, just like Jupyter
- **Plot Support**: Inline display of Matplotlib plots

### Security & Data Safety

- **Automatic Backups**: Regular automatic backups protect your work
- **Manual Backup**: Create backups on demand
- **Execution Timeout**: Configurable time limits prevent infinite loops
- **Memory Limits**: Optional memory usage limits protect system stability

### User Interface

- **Clean Design**: Minimalist interface focused on content
- **Syntax Highlighting**: Colorful Python syntax highlighting
- **Cell Prompts**: Clear indication of execution order
- **Responsive Output**: Text and graphical output display
- **Keyboard Shortcuts**: Efficient navigation and operation

## Installation

### Prerequisites

Before installing Lotus Notebook, ensure you have the following dependencies:

- **Qt5**: The GUI framework
  - Ubuntu/Debian: `sudo apt-get install qtbase5-dev qt5-qmake`
  - Fedora/RHEL: `sudo dnf install qt5-qtbase-devel qt5-qttools`
  - Arch Linux: `sudo pacman -S qt5-base`

- **CMake**: Build system (version 3.16 or later)
  - Ubuntu/Debian: `sudo apt-get install cmake`
  - Fedora/RHEL: `sudo dnf install cmake`
  - Arch Linux: `sudo pacman -S cmake`

- **Python 3**: Runtime and development headers
  - Ubuntu/Debian: `sudo apt-get install python3 python3-dev`
  - Fedora/RHEL: `sudo dnf install python3 python3-devel`
  - Arch Linux: `sudo pacman -S python`

### Build and Install

1. Clone or navigate to the Lotus Notebook directory:

```bash
cd lotus-notebook
```

2. Run the installation script:

```bash
chmod +x scripts/install.sh
./scripts/install.sh
```

3. The installation script will:
   - Check and install dependencies
   - Build the application
   - Install to `/opt/lotus-notebook`
   - Create desktop entry and icons
   - Add symlink for command-line access

4. Run Lotus Notebook:

```bash
lotus-notebook
```

### Custom Installation

Install to a custom prefix:

```bash
./scripts/install.sh --prefix=/usr/local
```

### Uninstallation

To uninstall Lotus Notebook:

```bash
chmod +x scripts/uninstall.sh
./scripts/uninstall.sh
```

To also remove user data:

```bash
./scripts/uninstall.sh --purge
```

## Usage

### Getting Started

1. Launch Lotus Notebook from your application menu or run `lotus-notebook`
2. A new empty notebook opens with a code cell
3. Type Python code in the cell
4. Press **Ctrl+Enter** or click the play button to execute
5. Add markdown cells for documentation using the menu or **Ctrl+Shift+M**

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New notebook |
| `Ctrl+O` | Open notebook |
| `Ctrl+S` | Save notebook |
| `Ctrl+Shift+S` | Save notebook as |
| `Ctrl+Enter` | Run current cell |
| `Ctrl+Shift+Enter` | Run all cells |
| `Ctrl+=` | Insert code cell above |
| `Ctrl+Shift+M` | Insert markdown cell above |
| `Ctrl+B` | Toggle backup |
| `Ctrl+Q` | Exit application |

### Python Code

Lotus Notebook executes Python code in a persistent session:

```python
# Cell 1: Define variables
x = 10
y = 20

# Cell 2: Use variables
result = x + y
print(f"The result is: {result}")

# Cell 3: Define a function
def greet(name):
    return f"Hello, {name}!"

# Cell 4: Use the function
message = greet("World")
print(message)
```

### Matplotlib Plots

Lotus Notebook captures Matplotlib plots automatically:

```python
import matplotlib.pyplot as plt
import numpy as np

x = np.linspace(0, 10, 100)
y = np.sin(x)

plt.figure(figsize=(8, 4))
plt.plot(x, y)
plt.title('Sine Wave')
plt.xlabel('x')
plt.ylabel('y')
plt.grid(True)
plt.show()
```

### Markdown Documentation

Add markdown cells for documentation:

```markdown
# Analysis Results

This notebook contains the results of our data analysis.

## Methodology

We used the following approach:
1. Data collection
2. Preprocessing
3. Statistical analysis
4. Visualization
```

## File Format

Lotus Notebook uses a JSON-based file format with `.lotus` extension:

```json
{
  "version": 1,
  "notebook_name": "My Analysis",
  "description": "Data analysis notebook",
  "kernel": "python3",
  "created_at": "2025-01-01T00:00:00",
  "modified_at": "2025-01-01T00:00:00",
  "cells": [
    {
      "type": "code",
      "content": "print('Hello, World!')",
      "execution_count": 1,
      "was_executed": true
    }
  ]
}
```

Lotus Notebook can also import/export Jupyter notebooks (`.ipynb` files).

## Configuration

### Backup Settings

Automatic backups are enabled by default and occur every 5 minutes. You can:

- Toggle auto-backup from the View menu
- Create manual backups with "Create Backup Now"
- Configure backup location: `~/.config/lotus-notebook/backups`

### Execution Settings

- **Timeout**: Default 10 seconds per cell execution
- **Memory**: Optional memory limits (requires system support)

### Environment Variables

| Variable | Description |
|----------|-------------|
| `LOTUS_BACKUP_DIR` | Override backup directory |
| `LOTUS_TIMEOUT` | Default execution timeout (seconds) |

## Architecture

### Components

- **MainWindow**: Application main window and UI management
- **CellWidget**: Individual cell component (code or markdown)
- **PythonExecutor**: Python interpreter integration via C API
- **NotebookManager**: Notebook file I/O and cell management
- **BackupManager**: Automatic and manual backup handling
- **SyntaxHighlighter**: Python syntax highlighting

### Technology Stack

- **GUI**: Qt5 Widgets
- **Python Integration**: Python C API
- **Build System**: CMake
- **Serialization**: Qt JSON classes
- **Language**: C++17

## Development

### Building from Source

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Project Structure

```
lotus-notebook/
├── src/
│   ├── main.cpp           # Application entry point
│   ├── mainwindow.cpp     # Main window implementation
│   ├── mainwindow.h       # Main window header
│   ├── cellwidget.cpp     # Cell widget implementation
│   ├── cellwidget.h       # Cell widget header
│   ├── pythonexecutor.cpp # Python executor implementation
│   ├── pythonexecutor.h   # Python executor header
│   ├── notebookmanager.cpp# Notebook management
│   ├── notebookmanager.h  # Notebook management header
│   ├── backupmanager.cpp  # Backup management
│   ├── backupmanager.h    # Backup management header
│   ├── syntaxhighlighter.cpp # Syntax highlighting
│   └── syntaxhighlighter.h    # Syntax highlighting header
├── scripts/
│   ├── install.sh         # Installation script
│   └── uninstall.sh       # Uninstallation script
├── assets/
│   ├── lotus.desktop      # Desktop entry
│   └── icon.png           # Application icon
├── CMakeLists.txt         # CMake configuration
└── README.md              # This file
```

## Troubleshooting

### Python Not Found

Ensure Python 3 is installed and accessible:
```bash
python3 --version
```

### Qt5 Not Found

Install Qt5 development packages for your distribution (see Prerequisites).

### Permission Denied

If installation fails with permission errors, run the install script with sudo or check directory permissions.

### Execution Timeout

If cells timeout frequently, increase the timeout setting or optimize your code.

### Plots Not Displaying

Ensure matplotlib is installed:
```bash
pip install matplotlib
```

## License

This project is open source and available under the MIT License.\
This is as/is community free opensource edition (LITE edition), call for enterprise solution [Lotus Chain Organization](https://lotuschain.org/contact)

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.\
This is free opensource edition for ever.

## Support

For issues and questions, please open an issue on the project repository.

## Acknowledgments

- Qt Framework: https://www.qt.io/
- Python: https://www.python.org/
- Jupyter Project: For inspiration and the notebook concept

---

> BLUE LOTUS INNOVATION LAB - 2022,2026