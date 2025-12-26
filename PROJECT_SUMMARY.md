# Lotus Notebook - Project Summary

## Project Structure

```
lotus-notebook/
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # Complete documentation
├── .gitignore                  # Git ignore rules
│
├── assets/                     # Application assets
│   ├── icon.svg               # Application icon (SVG)
│   └── lotus.desktop          # Desktop entry file
│
├── scripts/                    # Installation scripts
│   ├── install.sh             # Installation script
│   └── uninstall.sh           # Uninstallation script
│
├── python_modules/            # Python runtime modules
│   └── lotus_runtime.py       # Python setup and utilities
│
├── src/                        # C++ source code
│   ├── main.cpp               # Application entry point
│   ├── mainwindow.h           # Main window class header
│   ├── mainwindow.cpp         # Main window implementation
│   ├── cellwidget.h           # Cell widget class header
│   ├── cellwidget.cpp         # Cell widget implementation
│   ├── pythonexecutor.h       # Python executor header
│   ├── pythonexecutor.cpp     # Python executor implementation
│   ├── notebookmanager.h      # Notebook manager header
│   ├── notebookmanager.cpp    # Notebook manager implementation
│   ├── backupmanager.h        # Backup manager header
│   ├── backupmanager.cpp      # Backup manager implementation
│   ├── syntaxhighlighter.h    # Syntax highlighter header
│   └── syntaxhighlighter.cpp  # Syntax highlighter implementation
│
└── backups/                    # Backup storage directory (created at runtime)
```

## Core Components

### 1. MainWindow (`mainwindow.h`, `mainwindow.cpp`)

The main application window that manages the notebook interface, menu system, toolbar, and cell container. Handles all user interactions and coordinates between different components.

**Key Features:**
- Notebook file operations (new, open, save, save as)
- Cell management (add code/markdown cells, delete, move)
- Execution control (run cell, run all, restart kernel)
- Auto-save and backup management
- Status bar with kernel state and cell count

### 2. CellWidget (`cellwidget.h`, `cellwidget.cpp`)

Individual cell component that can be either a code cell or markdown cell. Manages code editing, output display, and cell operations.

**Key Features:**
- Syntax-highlighted code editor
- Execution button and state indicator
- Output area for text, errors, and plots
- Context menu for cell operations
- Markdown rendering support

### 3. PythonExecutor (`pythonexecutor.h`, `pythonexecutor.cpp`)

Embedded Python interpreter that executes code and captures output. Uses the Python C API for direct integration.

**Key Features:**
- Persistent Python session
- Stdout/stderr redirection
- Matplotlib plot capture
- Execution timeout handling
- Error handling and reporting

### 4. NotebookManager (`notebookmanager.h`, `notebookmanager.cpp`)

Notebook file I/O and cell data management. Handles loading, saving, and importing notebooks.

**Key Features:**
- JSON-based file format (.lotus)
- Jupyter notebook import (.ipynb)
- HTML export
- Cell data serialization
- Metadata management

### 5. BackupManager (`backupmanager.h`, `backupmanager.cpp`)

Automatic and manual backup system for notebook data protection.

**Key Features:**
- Automatic periodic backups
- Manual backup creation
- Backup rotation (keeps last N backups)
- Backup listing and restoration
- Configurable backup directory

### 6. SyntaxHighlighter (`syntaxhighlighter.h`, `syntaxhighlighter.cpp`)

Python syntax highlighting for code cells using Qt's syntax highlighting system.

**Key Features:**
- Keyword highlighting
- String and comment highlighting
- Function and decorator highlighting
- Number highlighting
- Multi-line comment support

## Installation Scripts

### install.sh

Installation script that:
1. Checks and installs dependencies (Qt5, CMake, Python3)
2. Builds the application using CMake
3. Installs to `/opt/lotus-notebook` (or custom prefix)
4. Creates desktop entry and icon
5. Adds symlink for command-line access

### uninstall.sh

Uninstallation script that:
1. Removes installed files and directories
2. Removes desktop entry and icons
3. Optionally removes user data with `--purge` flag
4. Cleans up empty directories

## Dependencies

### Required

- **Qt5**: GUI framework (Widgets module)
- **CMake**: Build system (3.16+)
- **Python 3**: Runtime and development headers
- **C++ Compiler**: GCC or Clang with C++17 support

### Optional

- **Python Libraries**: numpy, matplotlib (for enhanced functionality)

## Build Instructions

### Quick Build

```bash
cd lotus-notebook
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./lotus-notebook
```

### With Installation

```bash
cd lotus-notebook
chmod +x scripts/install.sh
./scripts/install.sh
lotus-notebook
```

### Custom Installation

```bash
./scripts/install.sh --prefix=/usr/local
```

## File Format

Lotus Notebook uses a JSON-based format with the following structure:

```json
{
  "version": 1,
  "notebook_name": "My Notebook",
  "description": "Optional description",
  "kernel": "python3",
  "created_at": "2025-01-01T00:00:00",
  "modified_at": "2025-01-01T00:00:00",
  "cells": [
    {
      "type": "code",
      "content": "print('Hello')",
      "execution_count": 1,
      "was_executed": true
    }
  ]
}
```

## Security Features

### Execution Safety

- **Timeout Protection**: Cells execute with a configurable timeout (default 10 seconds)
- **Memory Limits**: Optional memory usage limits prevent system overload
- **Signal Handling**: Graceful handling of execution timeouts

### Data Protection

- **Auto-Backup**: Automatic backups every 5 minutes
- **Manual Backup**: Create backups on demand
- **Backup Rotation**: Keeps last 10 backups by default
- **Version History**: Track changes over time

## Usage Example

### Creating a Simple Notebook

```python
# Cell 1: Import libraries
import numpy as np
import matplotlib.pyplot as plt

# Cell 2: Create data
x = np.linspace(0, 10, 100)
y = np.sin(x)

# Cell 3: Create plot
plt.figure(figsize=(8, 4))
plt.plot(x, y)
plt.title('Sine Wave')
plt.show()
```

## Architecture Overview

```
┌─────────────────────────────────────────┐
│           MainWindow                    │
│  ┌─────────────────────────────────────┐│
│  │           Cell Container            ││
│  │  ┌─────────┐  ┌─────────┐           ││
│  │  │ Cell 0  │  │ Cell 1  │  ...      ││
│  │  └─────────┘  └─────────┘           ││
│  └─────────────────────────────────────┘│
│         │         │         │           │
│         ▼         ▼         ▼           │
│  ┌──────────┬──────────┬──────────┐     │
│  │CellWidget│CellWidget│CellWidget│     │
│  └──────────┴──────────┴──────────┘     │
│              │                          │
│              ▼                          │
│  ┌──────────────────────────────┐       │
│  │      PythonExecutor          │       │
│  │  ┌────────────────────────┐  │       │
│  │  │   Python C API         │  │       │
│  │  │   - Execute code       │  │       │
│  │  │   - Capture output     │  │       │
│  │  │   - Handle plots       │  │       │
│  │  └────────────────────────┘  │       │
│  └──────────────────────────────┘       │
│              │                          │
│              ▼                          │
│  ┌──────────────────────────────┐       │
│  │      NotebookManager         │       │
│  │  - Load/Save notebooks       │       │
│  │  - Import Jupyter files      │       │
│  └──────────────────────────────┘       │
│              │                          │
│              ▼                          │
│  ┌──────────────────────────────┐       │
│  │       BackupManager          │       │
│  │  - Auto backup system        │       │
│  │  - Backup rotation           │       │
│  └──────────────────────────────┘       │
└─────────────────────────────────────────┘
```

## Future Enhancements

Possible future enhancements for Lotus Notebook:

1. **Code Completion**: Intelligent code completion for Python
2. **Variable Inspector**: Display all current variables and their values
3. **Debugging**: Integrated debugger with breakpoints
4. **File Browser**: Side panel with file system navigation
5. **Theme Support**: Multiple color themes
6. **Plugin System**: Extensible architecture for plugins
7. **Notebook Sharing**: Share notebooks via cloud or local network
8. **Export Options**: Export to PDF, Markdown, and other formats
9. **Multiple Kernels**: Support for other languages (R, Julia, etc.)
10. **Collaboration**: Real-time collaborative editing

## License

Lotus Notebook is open source software available under the MIT License.

## Contributing

Contributions are welcome! Please feel free to:
- Submit bug reports
- Propose new features
- Create pull requests
- Improve documentation

## Support

For issues, questions, or suggestions, please open an issue on the project repository.
