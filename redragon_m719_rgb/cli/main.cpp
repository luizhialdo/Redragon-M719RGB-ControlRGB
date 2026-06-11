#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <hidapi/hidapi.h>

// Definições de constantes do hardware do Redragon M719 Ultimate
constexpr uint16_t REDRAGON_VID       = 0x25a7;  // Vendor ID (Redragon)
constexpr uint16_t REDRAGON_PID_CABO = 0xfab0;  // Product ID via Cabo USB
constexpr uint16_t REDRAGON_PID_24GH = 0xfa7e;  // Product ID via Dongle Wireless 2.4GHz
constexpr int TARGET_INTERFACE        = 1;       // Interface USB específica para Custom Features

// --- FUNÇÃO DE CONVERSÃO DO MODO STREAMING ---
uint8_t obter_streaming_brilho(int brilho_porcentagem) {
    if (brilho_porcentagem == 10)      return 0x19; // Brilho Mínimo (10%)
    else if (brilho_porcentagem == 50) return 0x7d; // Brilho Médio (50%)
    return 0xff;                                    // Brilho Máximo (100% - Padrão)
}

// --- FUNÇÕES AUXILIARES DE PARSING DE CORES ---
int hex_to_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int parse_hex_byte(const std::string& hex_pair) {
    if (hex_pair.length() < 2) return -1;
    int high = hex_to_val(hex_pair[0]);
    int low = hex_to_val(hex_pair[1]);
    if (high == -1 || low == -1) return -1;
    return (high << 4) | low;
}

// Auxiliar para converter std::string para minúsculo de forma segura
std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { 
        return std::tolower(c); 
    });
    return str;
}

/**
 * Exibe o menu de ajuda completo detalhando o uso de cada modo.
 */
void exibir_ajuda(const std::string& nome_programa) {
    std::cout << "========================================================================\n"
              << "                REDRAGON M719 ULTIMATE - CONTROLLER HELP                \n"
              << "========================================================================\n\n"
              << "Uso Geral:\n"
              << "  " << nome_programa << " <modo> [argumentos_especificos]\n\n"
              << "------------------------------------------------------------------------\n"
              << "1. MODO ESTÁTICO (static / estatico)\n"
              << "   Define uma cor fixa para os LEDs do mouse.\n"
              << "   Argumentos:\n"
              << "     <cor_hex> : Nome da cor (red, green, blue, yellow, purple, white, orange)\n"
              << "                 OU código hexadecimal de 6 dígitos (ex: ff0055).\n"
              << "     [brilho]  : Opcional. Níveis aceitos: 10, 50 ou 100 (Padrão: 100).\n"
              << "   Exemplos:\n"
              << "     " << nome_programa << " static blue          <- Cor azul fixa no brilho máximo\n"
              << "     " << nome_programa << " static ff5500 50     <- Cor laranja customizada no brilho médio (50%)\n\n"
              << "------------------------------------------------------------------------\n"
              << "2. MODO RESPIRAÇÃO (respiration / respiracao)\n"
              << "   Faz a cor escolhida pulsar (efeito fade in / fade out).\n"
              << "   Argumentos:\n"
              << "     <cor_hex>    : Nome da cor ou código hexadecimal de 6 dígitos.\n"
              << "     [brilho]     : Opcional. Níveis aceitos: 10, 50 ou 100 (Padrão: 100).\n"
              << "     [velocidade] : Opcional. Níveis aceitos: 1 (Lento), 2 (Médio), 3 (Rápido) (Padrão: 2).\n"
              << "   Exemplos:\n"
              << "     " << nome_programa << " respiration red 100 1   <- Vermelho pulsando de forma Lenta\n"
              << "     " << nome_programa << " respiration 00ffff 50 3 <- Ciano pulsando de forma Rápida no brilho de 50%\n\n"
              << "------------------------------------------------------------------------\n"
              << "3. MODO STREAMING (streaming)\n"
              << "   Ativa o efeito RGB dinâmico fluído (onda de cores de fábrica).\n"
              << "   Argumentos:\n"
              << "     [brilho] : Opcional. Níveis aceitos: 10, 50 ou 100 (Padrão: 100).\n"
              << "                Nota: A velocidade neste modo é travada em 50% estável.\n"
              << "   Exemplos:\n"
              << "     " << nome_programa << " streaming      <- Ativa o RGB fluido no brilho máximo (100%)\n"
              << "     " << nome_programa << " streaming 10   <- Ativa o RGB fluido no brilho mínimo (10%)\n\n"
              << "------------------------------------------------------------------------\n"
              << "4. MODO DESLIGADO (off / desligado)\n"
              << "   Apaga completamente a iluminação RGB do mouse.\n"
              << "   Exemplo:\n"
              << "     " << nome_programa << " off\n"
              << "========================================================================\n";
}

int main(int argc, char *argv[]) {
    std::vector<std::string> args(argv, argv + argc);

    if (argc < 2 || args[1] == "help" || args[1] == "-h" || args[1] == "--help") {
        exibir_ajuda(args[0]);
        return 0;
    }

    std::string modo_input = to_lower(args[1]);
    bool modo_off = (modo_input == "off" || modo_input == "desligado");
    bool modo_streaming = (modo_input == "streaming");
    bool modo_static = (modo_input == "static" || modo_input == "estatico");
    bool modo_respiration = (modo_input == "respiration" || modo_input == "respiracao");

    if (!modo_off && !modo_streaming && !modo_static && !modo_respiration) {
        std::cerr << "Erro: Modo '" << args[1] << "' desconhecido. Digite '" << args[0] << " help' para ajuda.\n";
        return -1;
    }

    if (!modo_off && !modo_streaming && argc < 3) {
        std::cerr << "Erro: O modo '" << args[1] << "' requer uma cor. Digite '" << args[0] << " help' para exemplos.\n";
        return -1;
    }

    int brilho_porcentagem = 100;
    int vel_input = 2;
    bool customizado = false;

    // Parser seguro de argumentos usando std::stoi com tratamento de exceção
    try {
        if (modo_streaming && argc >= 3) {
            brilho_porcentagem = std::stoi(args[2]);
            customizado = true;
        } else if (modo_static && argc >= 4) {
            brilho_porcentagem = std::stoi(args[3]);
            customizado = true;
        } else if (modo_respiration) {
            if (argc >= 4) {
                brilho_porcentagem = std::stoi(args[3]);
                customizado = true;
            }
            if (argc >= 5) {
                vel_input = std::stoi(args[4]);
                if (vel_input < 1 || vel_input > 3) {
                    std::cerr << "Erro: Velocidade inválida (" << vel_input << "). Valores aceitos: 1, 2 ou 3.\n";
                    return -1;
                }
            }
        }
    } catch (const std::exception&) {
        std::cerr << "Erro: Parâmetro numérico de brilho ou velocidade inválido.\n";
        return -1;
    }

    if (customizado && brilho_porcentagem != 10 && brilho_porcentagem != 50 && brilho_porcentagem != 100) {
        std::cerr << "Erro: Brilho inválido (" << brilho_porcentagem << "). Valores aceitos: 10, 50 ou 100.\n";
        return -1;
    }

    uint8_t mode_byte = 0x02;
    if (modo_streaming)        mode_byte = 0x00;
    else if (modo_off)         mode_byte = 0x04;
    else if (modo_static)      mode_byte = 0x02;
    else if (modo_respiration) mode_byte = 0x01;

    int r = 0, g = 0, b = 0;
    if (modo_streaming) {
        r = 0xff; g = 0x00; b = 0x00;
    } else if (modo_off) {
        r = 0xff; g = 0x00; b = 0xff;
    } else {
        std::string cor_param = to_lower(args[2]);
        std::string color_str;

        if (cor_param == "red")          color_str = "ff0000";
        else if (cor_param == "green")   color_str = "00ff00";
        else if (cor_param == "blue")    color_str = "0000ff";
        else if (cor_param == "yellow")  color_str = "ffff00";
        else if (cor_param == "purple")  color_str = "ff00ff";
        else if (cor_param == "white")   color_str = "ffffff";
        else if (cor_param == "orange")  color_str = "ff4000";
        else {
            // Remove espaços em branco usando C++ strings de forma idiomática
            for (char c : args[2]) {
                if (!std::isspace(static_cast<unsigned char>(c))) {
                    color_str += c;
                }
            }
            if (color_str.length() != 6) {
                std::cerr << "Erro: O código de cor hexadecimal deve conter exatamente 6 caracteres.\n";
                return -1;
            }
        }

        r = parse_hex_byte(color_str.substr(0, 2));
        g = parse_hex_byte(color_str.substr(2, 2));
        b = parse_hex_byte(color_str.substr(4, 2));

        if (r == -1 || g == -1 || b == -1) {
            std::cerr << "Erro: Caractere hexadecimal inválido na cor informada.\n";
            return -1;
        }
    }

    // Inicialização segura do payload usando std::vector
    std::vector<uint8_t> payload(17, 0x00);

    payload[0] = 0x08; payload[1] = 0x07; payload[2] = 0x00; payload[3] = 0x00;
    payload[4] = 0xa0; payload[5] = 0x07;
    payload[6] = mode_byte;
    payload[7] = static_cast<uint8_t>(r); 
    payload[8] = static_cast<uint8_t>(g); 
    payload[9] = static_cast<uint8_t>(b);

    int cores_ativas = (r > 0) + (g > 0) + (b > 0);

    // --- MONTAGEM DOS BYTES DE CONTROLE E CHECKSUM ---
    if (modo_off) {
        payload[10] = 0x96; payload[11] = 0xff; payload[12] = 0xbe;
    } 
    else if (modo_streaming) {
        uint8_t s_byte = 0x13;
        uint8_t b_byte = obter_streaming_brilho(brilho_porcentagem);

        payload[10] = s_byte;
        payload[11] = b_byte;
        payload[12] = static_cast<uint8_t>(s_byte + b_byte - 0xce);
    } 
    else if (mode_byte == 0x01) {
        // --- MODO RESPIRAÇÃO ---
        uint8_t brightness_byte = 0xff;
        if (brilho_porcentagem == 10)       brightness_byte = 0x19;
        else if (brilho_porcentagem == 50)  brightness_byte = 0x7f;

        uint8_t speed_byte = 0x96;
        if (vel_input == 1)      speed_byte = 0xff;
        else if (vel_input == 3) speed_byte = 0x28;

        payload[10] = speed_byte;
        payload[11] = brightness_byte;
        
        uint8_t base_idx12 = 0xc0;
        if (speed_byte == 0x28) {
            if (brightness_byte == 0xff)      base_idx12 = 0x2e;
            else if (brightness_byte == 0x19) base_idx12 = 0x48;
            else                              base_idx12 = 0x3b;
        } 
        else if (speed_byte == 0x96) {
            if (brightness_byte == 0xff)      base_idx12 = 0xc0;
            else if (brightness_byte == 0x19) base_idx12 = 0xa6;
            else                              base_idx12 = 0xb3;
        } 
        else if (speed_byte == 0xff) {
            if (brightness_byte == 0xff)      base_idx12 = 0x57;
            else if (brightness_byte == 0x19) base_idx12 = 0x3d;
            else                              base_idx12 = 0x4a;
        }

        payload[12] = (cores_ativas > 0) ? (base_idx12 + (cores_ativas - 1)) : base_idx12;
    } 
    else {
        // --- MODO ESTÁTICO ---
        uint8_t brightness_byte = 0xff;
        if (brilho_porcentagem == 10)       brightness_byte = 0x19;
        else if (brilho_porcentagem == 50)  brightness_byte = 0x7f;

        if (brilho_porcentagem == 100) {
            if (cores_ativas <= 1) {
                payload[10] = 0xff; payload[11] = 0xff; payload[12] = 0x56;
            } else {
                payload[10] = 0x96; payload[11] = 0xff;
                if (r == 0xff && g == 0x40 && b == 0x00)       payload[12] = 0x7f; // Laranja
                else if (r == 0xff && g == 0xff && b == 0xff)  payload[12] = 0xc1; // Branco
                else                                           payload[12] = 0xc0;
            }
        } else {
            payload[10] = (brilho_porcentagem == 10) ? 0x96 : 0xcb;
            payload[11] = brightness_byte;
            uint8_t base_idx12_static = (brilho_porcentagem == 10) ? 0xa5 : 0x7d;
            
            payload[12] = (cores_ativas > 1) ? (base_idx12_static + (cores_ativas - 1)) : base_idx12_static;
        }
    }
    
    payload[13] = 0x00; payload[14] = 0x00; payload[15] = 0x00;
    payload[16] = 0x4a;

    // --- COMUNICAÇÃO USB VIA HIDAPI ---
    if (hid_init() < 0) {
        std::cerr << "Falha crítica ao inicializar a biblioteca HIDAPI.\n";
        return -1;
    }

    struct hid_device_info *devs = hid_enumerate(REDRAGON_VID, 0x0000);
    struct hid_device_info *cur_dev = devs;
    std::string path_to_open;
    int pid_encontrado = 0;
    
    while (cur_dev) {
        if ((cur_dev->product_id == REDRAGON_PID_CABO || cur_dev->product_id == REDRAGON_PID_24GH) 
            && cur_dev->interface_number == TARGET_INTERFACE) {
            path_to_open = cur_dev->path;
            pid_encontrado = cur_dev->product_id;
            break;
        }
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);

    hid_device *handle = nullptr;
    if (!path_to_open.empty()) {
        handle = hid_open_path(path_to_open.c_str());
    }

    if (!handle) {
        std::cerr << "Erro: Mouse não encontrado ou sem permissão de acesso. Execute com 'sudo'.\n";
        hid_exit();
        return -1;
    }

    int res = hid_send_feature_report(handle, payload.data(), payload.size());
    if (res < 0) {
        std::wcerr << L"Falha ao enviar comando: " << hid_error(handle) << L"\n";
    } else {
        std::string conexao = (pid_encontrado == REDRAGON_PID_24GH) ? "Sem Fio (2.4G)" : "Com Cabo";
        if (modo_respiration) {
            std::string vel_txt = (vel_input == 1) ? "Lento (1)" : (vel_input == 3) ? "Rápido (3)" : "Médio (2)";
            std::cout << "Sucesso! [" << conexao << "] Modo: respiration | Brilho: " << brilho_porcentagem << "% | Velocidade: " << vel_txt << "\n";
        } else if (modo_streaming || modo_static) {
            std::cout << "Sucesso! [" << conexao << "] Modo: " << args[1] << " | Brilho: " << brilho_porcentagem << "%\n";
        } else {
            std::cout << "Sucesso! [" << conexao << "] Modo: " << args[1] << " aplicado\n";
        }
    }

    hid_close(handle);
    hid_exit();
    return 0;
}
