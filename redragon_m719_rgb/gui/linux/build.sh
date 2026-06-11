#!/bin/bash

echo "===================================================="
echo "   Compilador do M719 com Interface Gráfica GTK 4 (C++)"
echo "===================================================="

# 1. Instalar as dependências do GTK 4 + HIDAPI + Compilador C++ nativo do sistema
if [ -f /etc/debian_version ]; then
    echo "[1/2] Instalando dependências de desenvolvimento no Ubuntu/Debian..."
    sudo apt update
    sudo apt install -y libgtk-4-dev libhidapi-dev pkg-config build-essential

elif [ -f /etc/fedora-release ]; then
    echo "[1/2] Instalando dependências no Fedora..."
    sudo dnf install -y gtk4-devel hidapi-devel pkgconf-pkg-config gcc-c++

elif [ -f /etc/arch-release ]; then
    echo "[1/2] Instalando dependências no Arch Linux..."
    sudo pacman -Syu --needed --noconfirm gtk4 hidapi pkgconf gcc
fi

# 2. Compilar o programa injetando as diretivas do pkg-config em C++17
echo "[2/2] Compilando interface gráfica main.cpp..."
g++ -std=c++17 main.cpp -o Redragon_M719RGB-PRO `pkg-config --cflags --libs gtk4 hidapi-libusb`

if [ $? -eq 0 ]; then
    echo "----------------------------------------------------"
    echo "Compilação concluída com sucesso!"
    echo "Abra o painel gráfico rodando o executável:"
    echo "  sudo ./Redragon_M719RGB-PRO"
    echo "===================================================="
else
    echo "Erro: Ocorreu uma falha ao compilar a interface GTK em C++."
    exit 1
fi
