#!/bin/bash

# Lotus Notebook Installation Script
# This script installs Lotus Notebook to the system

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print functions
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Default installation directory
INSTALL_PREFIX="/opt/lotus-notebook"
BIN_DIR="$INSTALL_PREFIX/bin"
SHARE_DIR="$INSTALL_PREFIX/share"

# Check if we need sudo
need_sudo() {
    # Check if we can write to the installation prefix
    if [[ -w "$INSTALL_PREFIX" ]] 2>/dev/null; then
        return 1  # No sudo needed
    fi
    # Check if the parent directory exists and is writable
    local parent_dir
    parent_dir=$(dirname "$INSTALL_PREFIX")
    if [[ -w "$parent_dir" ]] 2>/dev/null; then
        return 1  # No sudo needed
    fi
    # Need sudo for system directories
    return 0
}

# Get sudo if needed
get_sudo_if_needed() {
    if need_sudo; then
        if command -v sudo &> /dev/null; then
            if [[ -t 0 ]]; then
                # Terminal is attached, prompt for password
                print_info "This installation requires administrator privileges."
                print_info "Please enter your password when prompted."
                SUDO="sudo"
            else
                # No terminal, try non-interactive sudo
                SUDO="sudo -n"
            fi
        else
            # No sudo available, try using su
            if command -v su &> /dev/null; then
                print_info "Using su for administrator privileges."
                print_info "Please enter the root password when prompted."
                SUDO="su -c"
            else
                # No sudo or su available, use default prefix
                print_warning "No sudo or su available. Installing to home directory instead."
                INSTALL_PREFIX="$HOME/.local/lotus-notebook"
                BIN_DIR="$INSTALL_PREFIX/bin"
                SHARE_DIR="$INSTALL_PREFIX/share"
                SUDO=""
            fi
        fi
    else
        SUDO=""
    fi
}

# Execute command with sudo if needed
run_as_user_or_sudo() {
    if [[ -n "$SUDO" ]]; then
        $SUDO "$@"
    else
        "$@"
    fi
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --prefix=*)
            INSTALL_PREFIX="${1#*=}"
            BIN_DIR="$INSTALL_PREFIX/bin"
            SHARE_DIR="$INSTALL_PREFIX/share"
            shift
            ;;
        --prefix)
            INSTALL_PREFIX="$2"
            BIN_DIR="$INSTALL_PREFIX/bin"
            SHARE_DIR="$INSTALL_PREFIX/share"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --prefix=PATH    Installation prefix (default: /opt/lotus-notebook)"
            echo "  --help, -h       Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                        # Install to /opt/lotus-notebook"
            echo "  $0 --prefix=/usr/local    # Install to /usr/local"
            echo "  $0 --prefix=\$HOME/.local  # Install to user home directory"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

print_info "Lotus Notebook Installation Script"
print_info " Lotus Notebook LITE edition free "
print_info "=================================="
echo ""

# Get sudo if needed
get_sudo_if_needed

# Check for required tools
print_info "Checking dependencies..."

# Check for Qt5
if ! command -v qmake &> /dev/null && ! pkg-config --exists Qt5Core 2>/dev/null; then
    print_warning "Qt5 not found. Attempting to install..."

    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y qtbase5-dev qt5-qmake
    elif command -v yum &> /dev/null; then
        sudo yum install -y qt5-qtbase-devel qt5-qttools
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y qt5-qtbase-devel qt5-qttools
    elif command -v pacman &> /dev/null; then
        sudo pacman -S --noconfirm qt5-base
    else
        print_error "Cannot install Qt5 automatically. Please install Qt5 manually."
        exit 1
    fi
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    print_warning "CMake not found. Attempting to install..."

    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y cmake
    elif command -v yum &> /dev/null; then
        sudo yum install -y cmake
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y cmake
    elif command -v pacman &> /dev/null; then
        sudo pacman -S --noconfirm cmake
    else
        print_error "Cannot install CMake automatically. Please install CMake manually."
        exit 1
    fi
fi

# Check for Python3
if ! command -v python3 &> /dev/null; then
    print_error "Python 3 is required but not found."
    exit 1
fi

print_success "All dependencies found"

# Check for Python development headers
if ! python3-config --includes &> /dev/null; then
    print_warning "Python development headers not found. Attempting to install..."

    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y python3-dev
    elif command -v yum &> /dev/null; then
        sudo yum install -y python3-devel
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y python3-devel
    elif command -v pacman &> /dev/null; then
        sudo pacman -S --noconfirm python
    fi
fi

# Build the project
print_info "Building Lotus Notebook..."
echo ""

# Create build directory
BUILD_DIR="$PROJECT_DIR/build"
if [[ -d "$BUILD_DIR" ]]; then
    print_info "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_info "Running CMake..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
      "$PROJECT_DIR"

# Build
print_info "Compiling..."
make -j$(nproc)

echo ""
print_success "Build completed"

# Install
print_info "Installing to $INSTALL_PREFIX..."
echo ""

# Create directories with sudo if needed
run_as_user_or_sudo mkdir -p "$BIN_DIR"
run_as_user_or_sudo mkdir -p "$SHARE_DIR/applications"
run_as_user_or_sudo mkdir -p "$SHARE_DIR/pixmaps"

# Copy binary with sudo if needed
run_as_user_or_sudo cp "$BUILD_DIR/lotus-notebook" "$BIN_DIR/"
run_as_user_or_sudo chmod +x "$BIN_DIR/lotus-notebook"

# Copy desktop entry with sudo if needed
run_as_user_or_sudo cp "$PROJECT_DIR/assets/lotus.desktop" "$SHARE_DIR/applications/"

# Copy icon with sudo if needed
if [[ -f "$PROJECT_DIR/assets/icon.png" ]]; then
    run_as_user_or_sudo cp "$PROJECT_DIR/assets/icon.png" "$SHARE_DIR/pixmaps/lotus-notebook.png"
    # Also create a scalable version
    run_as_user_or_sudo cp "$PROJECT_DIR/assets/icon.png" "$SHARE_DIR/pixmaps/"
elif [[ -f "$PROJECT_DIR/assets/icon.svg" ]]; then
    # Convert SVG to PNG for desktop entry
    if command -v rsvg-convert &> /dev/null; then
        rsvg-convert -w 128 -h 128 "$PROJECT_DIR/assets/icon.svg" -o /tmp/lotus-notebook.png
        run_as_user_or_sudo cp /tmp/lotus-notebook.png "$SHARE_DIR/pixmaps/lotus-notebook.png"
    else
        # Try using ImageMagick
        if command -v convert &> /dev/null; then
            convert -background none -resize 128x128 "$PROJECT_DIR/assets/icon.svg" /tmp/lotus-notebook.png
            run_as_user_or_sudo cp /tmp/lotus-notebook.png "$SHARE_DIR/pixmaps/lotus-notebook.png"
        else
            print_warning "No icon conversion tool found. Desktop icon may not display correctly."
        fi
    fi
fi

# Create symlink in /usr/local/bin for system-wide access (only for default installation)
if [[ "$INSTALL_PREFIX" == "/opt/lotus-notebook" ]]; then
    if [[ -L "/usr/local/bin/lotus-notebook" ]]; then
        sudo rm "/usr/local/bin/lotus-notebook"
    fi
    sudo ln -sf "$BIN_DIR/lotus-notebook" "/usr/local/bin/lotus-notebook"
fi

# Create backups and data directories with sudo if needed
run_as_user_or_sudo mkdir -p "$INSTALL_PREFIX/backups"
run_as_user_or_sudo mkdir -p "$INSTALL_PREFIX/data"

echo ""
print_success "Installation completed!"
echo ""
echo "=============================================="
echo "  Lotus Notebook has been installed!"
echo "=============================================="
echo ""
echo "Installation details:"
echo "  Binary: $BIN_DIR/lotus-notebook"
echo "  Desktop entry: $SHARE_DIR/applications/lotus.desktop"
echo "  Icon: $SHARE_DIR/pixmaps/lotus-notebook.png"
echo ""
echo "To run Lotus Notebook:"
echo "  1. From application menu (if desktop entry is installed)"
echo "  2. From terminal: $BIN_DIR/lotus-notebook"
echo ""
print_info "Run 'lotus-notebook' to start the application"
echo ""
