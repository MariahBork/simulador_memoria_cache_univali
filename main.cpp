#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <cmath>
#include <iomanip>
#include <algorithm>
using namespace std;

// ============================================
// ESTRUTURAS DE DADOS
// ============================================

// Configuração completa da cache
struct CacheConfig {
    int cache_size = 1024;   // Tamanho total da cache (bytes)
    int block_size = 16;     // Tamanho de cada bloco (bytes)
    int assoc = 1;           // Associatividade (1=direta, N=N-way)
    int addr_bits = 16;      // Bits de endereçamento
    string input_file = "";  // Caminho do arquivo de entrada
    bool verbose = false;    // Modo detalhado
    
    // Campos calculados automaticamente
    int num_blocks = 0;      // Número total de blocos
    int num_sets = 0;        // Número de conjuntos
    int offset_bits = 0;     // Bits para offset
    int index_bits = 0;      // Bits para índice
    int tag_bits = 0;        // Bits para tag
};

// Uma linha da cache
struct CacheLine {
    bool valid = false;      // Bit de validade
    int tag = 0;             // Tag armazenada
    int last_used = 0;       // Timestamp para LRU
};

// Estatísticas de desempenho
struct Stats {
    int total_accesses = 0;
    int hits = 0;
    int misses = 0;
};

// ============================================
// FUNÇÕES AUXILIARES
// ============================================

// Verifica se um número é potência de 2
bool is_power_of_two(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

// Calcula log2 de uma potência de 2
int log2_int(int n) {
    int result = 0;
    while (n > 1) {
        n >>= 1;
        result++;
    }
    return result;
}

// Converte string para inteiro (suporta decimal e hex 0x)
int parse_address_string(const string& str) {
    string s = str;
    // Remove espaços em branco
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    
    // Verifica se é hexadecimal (0x ou 0X)
    if (s.length() >= 2 && (s.substr(0, 2) == "0x" || s.substr(0, 2) == "0X")) {
        return stoi(s.substr(2), nullptr, 16);
    }
    // Decimal
    return stoi(s);
}

// Extrai campos tag, index e offset de um endereço
void extract_address_fields(int address, int offset_bits, int index_bits,
                           int& tag, int& index, int& offset) {
    int offset_mask = (1 << offset_bits) - 1;
    int index_mask = (1 << index_bits) - 1;
    
    offset = address & offset_mask;
    index = (address >> offset_bits) & index_mask;
    tag = address >> (offset_bits + index_bits);
}

// ============================================
// LEITURA DO ARQUIVO DE ENDEREÇOS
// ============================================
vector<int> load_addresses_from_file(const string& filename, int addr_bits) {
    vector<int> addresses;
    ifstream file(filename);
    
    // Verifica se arquivo existe
    if (!file.is_open()) {
        cerr << "\n==============================================" << endl;
        cerr << " ERRO CRÍTICO!" << endl;
        cerr << "==============================================" << endl;
        cerr << "Não foi possível abrir o arquivo:" << endl;
        cerr << "'" << filename << "'" << endl;
        cerr << "\nVerifique:" << endl;
        cerr << "  1. Se o arquivo existe no diretório atual" << endl;
        cerr << "  2. Se o caminho está correto" << endl;
        cerr << "  3. Se você tem permissão de leitura" << endl;
        cerr << "==============================================" << endl;
        exit(1);
    }
    
    int max_address = (1 << addr_bits) - 1;
    string line;
    int line_number = 0;
    
    cout << "\nLendo arquivo: " << filename << endl;
    
    while (getline(file, line)) {
        line_number++;
        
        // Remove espaços e tabs do início/fim
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        if (line.empty()) continue;
        
        // Remove espaços do final
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // Ignora linhas de comentário
        if (line[0] == '#') {
            if (addresses.empty() || true) {  // Debug opcional
                cout << "  [Linha " << line_number << "] Comentário ignorado" << endl;
            }
            continue;
        }
        
        // Ignora linhas vazias após remoção
        if (line.empty()) continue;
        
        try {
            int addr = parse_address_string(line);
            
            // Valida endereço
            if (addr < 0) {
                cerr << "  ERRO linha " << line_number 
                     << ": Endereço negativo (" << addr << ")" << endl;
                exit(1);
            }
            
            if (addr > max_address) {
                cerr << "  ERRO linha " << line_number 
                     << ": Endereço 0x" << hex << addr 
                     << " excede " << addr_bits << " bits (max=0x" 
                     << max_address << ")" << dec << endl;
                exit(1);
            }
            
            addresses.push_back(addr);
            cout << "  [Linha " << line_number << "] Endereço carregado: 0x" 
                 << hex << setw(4) << setfill('0') << addr << dec << endl;
            
        } catch (const invalid_argument& e) {
            cerr << "  ERRO linha " << line_number 
                 << ": Formato inválido -> '" << line << "'" << endl;
            cerr << "  Use formato decimal (ex: 256) ou hex (ex: 0x00FF)" << endl;
            exit(1);
        } catch (const out_of_range& e) {
            cerr << "  ERRO linha " << line_number 
                 << ": Número muito grande -> '" << line << "'" << endl;
            exit(1);
        }
    }
    
    file.close();
    cout << "\nTotal de endereços carregados: " << addresses.size() << endl;
    
    if (addresses.empty()) {
        cerr << "\nAVISO: Nenhum endereço válido encontrado no arquivo!" << endl;
        cerr << "Verifique se o arquivo contém endereços no formato correto." << endl;
    }
    
    return addresses;
}

// ============================================
// SIMULADOR DE CACHE
// ============================================
class CacheSimulator {
private:
    CacheConfig config;
    vector<vector<CacheLine>> cache;  // cache[conjunto][via]
    Stats stats;
    int global_time = 0;
    
public:
    // Construtor: inicializa e valida configuração
    CacheSimulator(CacheConfig cfg) : config(cfg) {
        // ===== VALIDAÇÕES =====
        if (!is_power_of_two(config.cache_size)) {
            throw runtime_error(
                "Tamanho da cache (" + to_string(config.cache_size) + 
                ") deve ser potência de 2!\n" +
                "Exemplos válidos: 128, 256, 512, 1024, 2048, 4096"
            );
        }
        
        if (!is_power_of_two(config.block_size)) {
            throw runtime_error(
                "Tamanho do bloco (" + to_string(config.block_size) + 
                ") deve ser potência de 2!\n" +
                "Exemplos válidos: 4, 8, 16, 32, 64"
            );
        }
        
        if (!is_power_of_two(config.assoc)) {
            throw runtime_error(
                "Associatividade (" + to_string(config.assoc) + 
                ") deve ser potência de 2!\n" +
                "Exemplos válidos: 1 (direta), 2, 4, 8"
            );
        }
        
        if (config.block_size > config.cache_size) {
            throw runtime_error(
                "Bloco (" + to_string(config.block_size) + 
                " bytes) maior que cache (" + to_string(config.cache_size) + 
                " bytes)!"
            );
        }
        
        if (config.cache_size % config.block_size != 0) {
            throw runtime_error(
                "Tamanho da cache deve ser múltiplo do tamanho do bloco!"
            );
        }
        
        // ===== CÁLCULO DOS CAMPOS =====
        config.num_blocks = config.cache_size / config.block_size;
        config.num_sets = config.num_blocks / config.assoc;
        
        if (config.num_blocks % config.assoc != 0) {
            throw runtime_error(
                "Número de blocos (" + to_string(config.num_blocks) + 
                ") deve ser múltiplo da associatividade (" + 
                to_string(config.assoc) + ")!"
            );
        }
        
        config.offset_bits = log2_int(config.block_size);
        config.index_bits = log2_int(config.num_sets);
        config.tag_bits = config.addr_bits - config.index_bits - config.offset_bits;
        
        if (config.tag_bits < 0) {
            throw runtime_error(
                "Configuração inválida: bits de endereço insuficientes!\n" +
                string("Tente aumentar --addr-bits ou reduzir cache/bloco.")
            );
        }
        
        // ===== INICIALIZA ESTRUTURA DA CACHE =====
        cache.resize(config.num_sets);
        for (int i = 0; i < config.num_sets; i++) {
            cache[i].resize(config.assoc);
        }
        
        cout << "\nCache inicializada com sucesso!" << endl;
    }
    
    // Simula um acesso à cache
    bool access(int address) {
        int tag, index, offset;
        extract_address_fields(address, config.offset_bits, 
                              config.index_bits, tag, index, offset);
        
        global_time++;
        stats.total_accesses++;
        
        // ===== VERIFICA HIT =====
        for (int way = 0; way < config.assoc; way++) {
            if (cache[index][way].valid && cache[index][way].tag == tag) {
                // HIT!
                stats.hits++;
                cache[index][way].last_used = global_time;
                
                if (config.verbose) {
                    cout << "  0x" << hex << setw(4) << setfill('0') << address
                         << " | tag=0x" << setw(2) << tag
                         << " idx=" << dec << setw(2) << index
                         << " off=" << setw(2) << offset
                         << " | HIT  [via " << way << "]" << endl;
                }
                return true;
            }
        }
        
        // ===== MISS: ESCOLHE VÍTIMA =====
        stats.misses++;
        int victim_way = 0;
        
        // Para mapeamento direto, sempre usa via 0
        if (config.assoc == 1) {
            victim_way = 0;
        } else {
            // Primeiro: procura via inválida (vazia)
            bool found_empty = false;
            for (int way = 0; way < config.assoc; way++) {
                if (!cache[index][way].valid) {
                    victim_way = way;
                    found_empty = true;
                    break;
                }
            }
            
            // Se não tem vazia, usa LRU (menor timestamp)
            if (!found_empty) {
                int min_time = cache[index][0].last_used;
                victim_way = 0;
                for (int way = 1; way < config.assoc; way++) {
                    if (cache[index][way].last_used < min_time) {
                        min_time = cache[index][way].last_used;
                        victim_way = way;
                    }
                }
            }
        }
        
        // Carrega o bloco na via vítima
        cache[index][victim_way].valid = true;
        cache[index][victim_way].tag = tag;
        cache[index][victim_way].last_used = global_time;
        
        if (config.verbose) {
            cout << "  0x" << hex << setw(4) << setfill('0') << address
                 << " | tag=0x" << setw(2) << tag
                 << " idx=" << dec << setw(2) << index
                 << " off=" << setw(2) << offset
                 << " | MISS [via " << victim_way << "]" << endl;
        }
        
        return false;
    }
    
    // Exibe configuração detalhada
    void print_configuration() {
        cout << "╔══════════════════════════════════════════════╗" << endl;
        cout << "║     CONFIGURAÇÃO DA CACHE                    ║" << endl;
        cout << "╚══════════════════════════════════════════════╝" << endl;
        cout << "  Tamanho total:      " << config.cache_size << " bytes" << endl;
        cout << "  Tamanho do bloco:   " << config.block_size << " bytes" << endl;
        cout << "  Associatividade:    " << config.assoc;
        if (config.assoc == 1) {
            cout << " (Mapeamento Direto)";
        } else {
            cout << "-way associativa (LRU)";
        }
        cout << endl;
        cout << "  Bits de endereço:   " << config.addr_bits << endl;
        cout << "  ─────────────────────────────────────────" << endl;
        cout << "  DECOMPOSIÇÃO DO ENDEREÇO:" << endl;
        cout << "    Tag:    " << config.tag_bits << " bits" << endl;
        cout << "    Index:  " << config.index_bits << " bits" << endl;
        cout << "    Offset: " << config.offset_bits << " bits" << endl;
        cout << "  ─────────────────────────────────────────" << endl;
        cout << "  Total de blocos:    " << config.num_blocks << endl;
        cout << "  Total de conjuntos: " << config.num_sets << endl;
        cout << "  Vias por conjunto:  " << config.assoc << endl;
        cout << "╚══════════════════════════════════════════════╝" << endl;
    }
    
    // Exibe estatísticas finais
    void print_statistics() {
        float hit_rate = 0.0f;
        float miss_rate = 0.0f;
        
        if (stats.total_accesses > 0) {
            hit_rate = (float)stats.hits / stats.total_accesses * 100.0f;
            miss_rate = (float)stats.misses / stats.total_accesses * 100.0f;
        }
        
        cout << "╔══════════════════════════════════════════════╗" << endl;
        cout << "║     RESULTADOS DA SIMULAÇÃO                  ║" << endl;
        cout << "╚══════════════════════════════════════════════╝" << endl;
        cout << "  Total de acessos: " << stats.total_accesses << endl;
        cout << "  ─────────────────────────────────────────" << endl;
        cout << "  Cache Hits:  " << setw(6) << stats.hits 
             << "  (" << fixed << setprecision(2) << setw(6) << hit_rate << "%)" 
             << endl;
        cout << "  Cache Misses:" << setw(6) << stats.misses 
             << "  (" << fixed << setprecision(2) << setw(6) << miss_rate << "%)" 
             << endl;
        cout << "╚════════════════════════════════════════════════╝" << endl;
        
        // Análise rápida
        cout << "\n  Análise: ";
        if (hit_rate >= 90) {
            cout << "Excelente taxa de acerto! Cache bem dimensionada.";
        } else if (hit_rate >= 70) {
            cout << "Boa taxa de acerto. Cache adequada para o padrão de acesso.";
        } else if (hit_rate >= 50) {
            cout << "Taxa de acerto moderada. Considere aumentar cache ou associatividade.";
        } else {
            cout << "Baixa taxa de acerto. Reveja dimensionamento da cache.";
        }
        cout << endl;
    }
};

// ============================================
// FUNÇÃO PARA MENU INTERATIVO
// ============================================
CacheConfig interactive_menu() {
    CacheConfig cfg;
    
    cout << "╔══════════════════════════════════════════════╗" << endl;
    cout << "║   SIMULADOR DE MEMÓRIA CACHE                 ║" << endl;
    cout << "║   Configuração Interativa                    ║" << endl;
    cout << "╚══════════════════════════════════════════════╝" << endl;
    cout << "\nPressione ENTER para usar valor padrão [entre colchetes]\n" << endl;
    
    string input;
    
    // Tamanho da cache
    cout << "Tamanho da cache em bytes [1024]: ";
    getline(cin, input);
    cfg.cache_size = input.empty() ? 1024 : stoi(input);
    
    // Tamanho do bloco
    cout << "Tamanho do bloco em bytes [16]: ";
    getline(cin, input);
    cfg.block_size = input.empty() ? 16 : stoi(input);
    
    // Associatividade
    cout << "Associatividade (1=direta, 2=2-way, 4=4-way) [1]: ";
    getline(cin, input);
    cfg.assoc = input.empty() ? 1 : stoi(input);
    
    // Bits de endereço
    cout << "Bits de endereçamento [16]: ";
    getline(cin, input);
    cfg.addr_bits = input.empty() ? 16 : stoi(input);
    
    // Arquivo de entrada
    cout << "Arquivo de endereços [teste.txt]: ";
    getline(cin, input);
    cfg.input_file = input.empty() ? "teste.txt" : input;
    
    // Modo verbose
    cout << "Modo detalhado? (s/N): ";
    getline(cin, input);
    cfg.verbose = (input == "s" || input == "S" || input == "sim" || input == "SIM");
    
    return cfg;
}

// ============================================
// PROGRAMA PRINCIPAL
// ============================================
int main(int argc, char* argv[]) {
    cout << "\n==============================================" << endl;
    cout << "  SIMULADOR DE MEMÓRIA CACHE" << endl;
    cout << "  Disciplina: Organização de Computadores" << endl;
    cout << "  UNIVALI - Prof. Thiago Felski Pereira" << endl;
    cout << "==============================================" << endl;
    
    CacheConfig config;
    bool has_arguments = false;
    
    // Parse de argumentos da linha de comando
    if (argc > 1) {
        has_arguments = true;
        for (int i = 1; i < argc; i++) {
            string arg = argv[i];
            
            if (arg.find("--cache-size=") == 0)
                config.cache_size = stoi(arg.substr(13));
            else if (arg.find("--block-size=") == 0)
                config.block_size = stoi(arg.substr(13));
            else if (arg.find("--assoc=") == 0)
                config.assoc = stoi(arg.substr(8));
            else if (arg.find("--addr-bits=") == 0)
                config.addr_bits = stoi(arg.substr(12));
            else if (arg.find("--input=") == 0)
                config.input_file = arg.substr(8);
            else if (arg == "--verbose")
                config.verbose = true;
            else if (arg == "--help" || arg == "-h") {
                cout << "\nUso: " << argv[0] << " [OPÇÕES]\n" << endl;
                cout << "Opções:" << endl;
                cout << "  --cache-size=N    Tamanho da cache em bytes (padrão: 1024)" << endl;
                cout << "  --block-size=N    Tamanho do bloco em bytes (padrão: 16)" << endl;
                cout << "  --assoc=N         Associatividade: 1=direta, N=N-way (padrão: 1)" << endl;
                cout << "  --addr-bits=N     Bits de endereçamento (padrão: 16)" << endl;
                cout << "  --input=ARQUIVO   Arquivo com endereços" << endl;
                cout << "  --verbose         Modo detalhado (mostra cada acesso)" << endl;
                cout << "\nExemplos:" << endl;
                cout << "  " << argv[0] << " --input=teste.txt" << endl;
                cout << "  " << argv[0] << " --cache-size=256 --block-size=16 --assoc=2 --input=acessos.txt --verbose" << endl;
                cout << "  " << argv[0] << "  (para menu interativo)\n" << endl;
                return 0;
            }
        }
    }
    
    // Se não tem argumentos ou não tem input, abre menu
    if (!has_arguments || config.input_file.empty()) {
        if (has_arguments && config.input_file.empty()) {
            cerr << "\nAVISO: Arquivo de entrada não especificado!" << endl;
            cerr << "Use --input=arquivo.txt ou execute sem argumentos para menu.\n" << endl;
        }
        config = interactive_menu();
    }
    
    try {
        // Carrega endereços do arquivo
        vector<int> addresses = load_addresses_from_file(
            config.input_file, 
            config.addr_bits
        );
        
        if (addresses.empty()) {
            cerr << "\nERRO: Nenhum endereço para simular!" << endl;
            return 1;
        }
        
        // Cria e configura simulador
        CacheSimulator sim(config);
        sim.print_configuration();
        
        // Executa simulação
        cout << "\n==============================================" << endl;
        cout << "  SIMULANDO ACESSOS..." << endl;
        cout << "==============================================" << endl;
        
        if (config.verbose) {
            cout << "\n  ENDEREÇO          | DECOMPOSIÇÃO         | RESULTADO" << endl;
            cout << "  ─────────────────────────────────────────────────────" << endl;
        }
        
        for (size_t i = 0; i < addresses.size(); i++) {
            sim.access(addresses[i]);
        }
        
        // Exibe resultados
        sim.print_statistics();
        
        cout << "\nSimulação concluída com sucesso!\n" << endl;
        
    } catch (const exception& e) {
        cerr << "╔══════════════════════════════════════════════╗" << endl;
        cerr << "║  ERRO NA EXECUÇÃO                            ║" << endl;
        cerr << "╚══════════════════════════════════════════════╝" << endl;
        cerr << "  " << e.what() << endl;
        cerr << "\n  Verifique os parâmetros e tente novamente.\n" << endl;
        return 1;
    }
    
    return 0;
}