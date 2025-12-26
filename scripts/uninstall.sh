#!/bin/bash

# Lotus Notebook Uninstallation Script
# This script removes Lotus Notebook from the system

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
        --purge|-p)
            PURGE_DATA=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --prefix=PATH    Installation prefix (default: /opt/lotus-notebook)"
            echo "  --purge, -p      Also remove user data and configuration"
            echo "  --help, -h       Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                        # Uninstall from /opt/lotus-notebook"
            echo "  $0 --purge                # Uninstall and remove user data"
            echo "  $0 --prefix=/usr/local    # Uninstall from custom location"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

PURGE_DATA=${PURGE_DATA:-false}

print_info "Lotus Notebook Uninstallation Script"
print_info "====================================="
echo ""

# Check if installed
if [[ ! -d "$INSTALL_PREFIX" ]]; then
    print_warning "Lotus Notebook does not appear to be installed at $INSTALL_PREFIX"
    print_info "Nothing to uninstall"
    exit 0
fi

# Confirm uninstallation
if [[ "$PURGE_DATA" == "true" ]]; then
    print_warning "This will remove ALL Lotus Notebook data including:"
    echo "  - Application installation"
    echo "  - User notebooks and backups"
    echo "  - Configuration files"
    echo ""

    read -p "Are you sure you want to continue? (yes/no): " confirm
    if [[ "$confirm" != "yes" ]]; then
        print_info "Uninstallation cancelled"
        exit 0
    fi
else
    read -p "Are you sure you want to uninstall Lotus Notebook? (yes/no): " confirm
    if [[ "$confirm" != "yes" ]]; then
        print_info "Uninstallation cancelled"
        exit 0
    fi
fi

echo ""

# Remove symlink from /usr/local/bin
if [[ -L "/usr/local/bin/lotus-notebook" ]]; then
    print_info "Removing symlink from /usr/local/bin..."
    sudo rm -f "/usr/local/bin/lotus-notebook"
fi

# Remove desktop entry
if [[ -f "$SHARE_DIR/applications/lotus.desktop" ]]; then
    print_info "Removing desktop entry..."
    sudo rm -f "$SHARE_DIR/applications/lotus.desktop"
fi

# Remove icon
if [[ -f "$SHARE_DIR/pixmaps/lotus-notebook.png" ]]; then
    print_info "Removing icon..."
    sudo rm -f "$SHARE_DIR/pixmaps/lotus-notebook.png"
fi

# Remove binary
if [[ -f "$BIN_DIR/lotus-notebook" ]]; then
    print_info "Removing binary..."
    rm -f "$BIN_DIR/lotus-notebook"
fi

# Remove empty directories
print_info "Cleaning up directories..."

# Remove bin directory if empty
if [[ -d "$BIN_DIR" ]] && [[ -z "$(ls -A "$BIN_DIR" 2>/dev/null)" ]]; then
    rmdir "$BIN_DIR"
fi

# Remove share directory if empty
if [[ -d "$SHARE_DIR" ]] && [[ -z "$(ls -A "$SHARE_DIR" 2>/dev/null)" ]]; then
    sudo rmdir "$SHARE_DIR" 2>/dev/null || true
    sudo rmdir "$SHARE_DIR/applications" 2>/dev/null || true
    sudo rmdir "$SHARE_DIR/pixmaps" 2>/dev/null || true
fi

# Remove backup directory
if [[ -d "$INSTALL_PREFIX/backups" ]]; then
    print_info "Removing backups directory..."
    rm -rf "$INSTALL_PREFIX/backups"
fi

# Remove data directory
if [[ -d "$INSTALL_PREFIX/data" ]]; then
    print_info "Removing data directory..."
    rm -rf "$INSTALL_PREFIX/data"
fi

# Remove main installation directory if empty
if [[ -d "$INSTALL_PREFIX" ]] && [[ -z "$(ls -A "$INSTALL_PREFIX" 2>/dev/null)" ]]; then
    rmdir "$INSTALL_PREFIX"
fi

# Purge user data if requested
if [[ "$PURGE_DATA" == "true" ]]; then
    echo ""
    print_info "Purging user data..."

    # Remove user config directory
    USER_CONFIG_DIR="$HOME/.config/lotus-notebook"
    if [[ -d "$USER_CONFIG_DIR" ]]; then
        rm -rf "$USER_CONFIG_DIR"
        print_info "Removed config directory: $USER_CONFIG_DIR"
    fi

    # Remove user data directory
    USER_DATA_DIR="$HOME/.local/share/lotus-notebook"
    if [[ -d "$USER_DATA_DIR" ]]; then
        rm -rf "$USER_DATA_DIR"
        print_info "Removed data directory: $USER_DATA_DIR"
    fi

    # Remove user cache directory
    USER_CACHE_DIR="$HOME/.cache/lotus-notebook"
    if [[ -d "$USER_CACHE_DIR" ]]; then
        rm -rf "$USER_CACHE_DIR"
        print_info "Removed cache directory: $USER_CACHE_DIR"
    fi
fi

# Update desktop database (if available)
if command -v update-desktop-database &> /dev/null; then
    print_info "Updating desktop database..."
    update-desktop-database -q
fi

echo ""
print_success "Uninstallation completed!"
echo ""
echo "=============================================="
echo "  Lotus Notebook has been uninstalled!"
echo "=============================================="
echo ""

if [[ "$PURGE_DATA" == "true" ]]; then
    print_warning "All user data has been removed"
else
    print_info "User data and configuration have been preserved"
    print_info "To completely remove user data, run with --purge option"
fi

# Clean up build directory (optional)
if [[ -d "$PROJECT_DIR/build" ]]; then
    print_info ""
    read -p "Remove build directory? (yes/no): " clean_build
    if [[ "$clean_build" == "yes" ]]; then
        rm -rf "$PROJECT_DIR/build"
        print_info "Build directory removed"
    fi
fi
