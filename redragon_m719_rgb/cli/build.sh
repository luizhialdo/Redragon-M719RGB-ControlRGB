#!/bin/bash

echo "===================================================="
echo "    Instalador de Dependências e Compilador M719 (C++)"
echo "===================================================="

# 1. Detectar a distribuição Linux e instalar dependências básicas (G++ Compiler + HIDAPI)
if [ -f /etc/debian_version ]; then
    echo "[1/3] Sistema baseado em Debian/Ubuntu detectado."
    echo "      Instalando build-essential e libhidapi-dev..."
    sudo apt update
    sudo apt install -y build-essential libhidapi-dev

elif [ -f /etc/fedora-release ]; then
    echo "[1/3] Sistema baseado em Fedora detectado."
    echo "      Instalando gcc-c++ e hidapi-devel..."
    sudo dnf install -y gcc-c++ make hidapi-devel

elif [ -f /etc/arch-release ]; then
    echo "[1/3] Sistema baseado em Arch Linux detectado."
    echo "      Instalando base-devel e hidapi..."
    sudo pacman -Syu --needed --noconfirm base-devel hidapi

else
    echo "Aviso: Distribuição não reconhecida automaticamente."
    echo "Certifique-se de ter o 'g++' e o pacote de desenvolvimento da 'hidapi' instalados manualmente."
fi

# 2. Verificar se o arquivo main.cpp existe no diretório atual
if [ ! -f "main.cpp" ]; then
    echo "----------------------------------------------------"
    echo "Erro: O arquivo 'main.cpp' não foi encontrado nesta pasta!"
    echo "Certifique-se de salvar o código do controlador como 'main.cpp' antes de rodar este script."
    exit 1
fi

# 3. Compilar o programa principal em C++17
echo "[2/3] Compilando o programa 'main.cpp'..."
g++ -std=c++17 main.cpp -o Redragon_M719RGB-PRO -lhidapi-hidraw

# 4. Verificar se a compilação deu certo
if [ $? -eq 0 ]; then
    echo "[3/3] Compilação concluída com sucesso!"
    echo "----------------------------------------------------"
    echo "O executável './controle_m719' foi gerado."
    echo "Você já pode testá-lo rodando (como sudo se necessário):"
    echo "  sudo ./controle_m719"
    echo "===================================================="
else
    echo "Erro: Falha catastrófica durante a compilação do código em C++."
    exit 1
fi
