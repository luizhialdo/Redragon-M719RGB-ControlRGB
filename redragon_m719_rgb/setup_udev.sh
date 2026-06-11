#!/bin/bash

# Define o nome do arquivo de regras do udev
REGRAS_FILE="/etc/udev/rules.d/99-redragon-m719.rules"

echo "===================================================="
# Nome amigável para o processo
echo "  Configurando regras udev para o Redragon M719"
echo "===================================================="

# Verifica se o script está sendo executado como root (necessário para gravar em /etc)
if [ "$EUID" -ne 0 ]; then
  echo "Erro: Por favor, execute este script usando sudo!"
  echo "Exemplo: sudo $0"
  exit 1
fi

echo "[1/3] Criando arquivo de regras em $REGRAS_FILE..."

# Grava as regras udev para o Vendor ID 25a7
# Cobre tanto o PID do cabo (fab0) quanto o do dongle sem fio (fa7e)
cat << 'EOF' > "$REGRAS_FILE"
# Redragon M719 Ultimate - Conexao por Cabo USB
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="25a7", ATTRS{idProduct}=="fab0", MODE="0666", GROUP="plugdev", TAG+="uaccess"
SUBSYSTEM=="usb", ATTRS{idVendor}=="25a7", ATTRS{idProduct}=="fab0", MODE="0666", GROUP="plugdev", TAG+="uaccess"

# Redragon M719 Ultimate - Conexao Sem Fio (Dongle 2.4GHz)
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="25a7", ATTRS{idProduct}=="fa7e", MODE="0666", GROUP="plugdev", TAG+="uaccess"
SUBSYSTEM=="usb", ATTRS{idVendor}=="25a7", ATTRS{idProduct}=="fa7e", MODE="0666", GROUP="plugdev", TAG+="uaccess"
EOF

echo "[2/3] Recarregando o gerenciador de dispositivos udev..."
# Recarrega as regras do udev no sistema
udevadm control --reload-rules
# Força o udev a aplicar as novas regras aos dispositivos já conectados
udevadm trigger

echo "[3/3] Permissões atualizadas com sucesso!"
echo "----------------------------------------------------"
echo "Pronto! Se o mouse já estiver conectado, desconecte"
echo "e conecte-o novamente na porta USB para validar."
echo "Depois disso, você poderá rodar o seu programa"
echo "diretamente sem o uso de 'sudo'."
echo "===================================================="
