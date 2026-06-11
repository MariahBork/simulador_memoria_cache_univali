🖥️ Simulador de Memória Cache

- Disciplina: Organização de Computadores
- Instituição: UNIVALI
- Professor: Thiago Felski Pereira, M.Sc
- Autores: Leonardo Carvalho Bikulcius e Mariah Theodora Gondim Bork
- Data: 11/06/2026

---

📝 Descrição

Este projeto implementa um simulador de memória cache capaz de reproduzir o comportamento de políticas de mapeamento direto e associativo por conjunto (set-associative). O programa lê sequências de endereços de memória a partir de um arquivo, simula os acessos na cache configurada e exibe estatísticas detalhadas de desempenho.

Funcionalidades Implementadas

- ✅ Mapeamento direto (associatividade = 1)
- ✅ Mapeamento associativo por conjunto (N-way)
- ✅ Política de substituição LRU (Least Recently Used)
- ✅ Leitura de endereços em formato decimal e hexadecimal (0x)
- ✅ Suporte a comentários nos arquivos de entrada (#)
- ✅ Validação completa de parâmetros
- ✅ Tratamento de erros robusto
- ✅ Modo verbose para rastreamento detalhado
- ✅ Estatísticas de hits, misses e taxas percentuais
- ✅ Menu interativo para configuração

---

🔧 Compilação

Requisitos
- Compilador C++11 ou superior (GCC, Clang, MSVC)
- Sistema operacional: Windows, Linux ou macOS

Compilando

g++ -std=c++11 -O2 main.cpp -o cache_sim

No Windows (MinGW):

g++ -std=c++11 -O2 main.cpp -o cache_sim.exe

No Linux/macOS:

g++ -std=c++11 -O2 main.cpp -o cache_sim

---

🚀 Execução

Modo 1: Linha de Comando (Recomendado)

./cache_sim --cache-size=1024 --block-size=16 --assoc=1 --addr-bits=16 --input=teste.txt

Modo 2: Menu Interativo

./cache_sim

Modo 3: Com Verbose (Mostra Cada Acesso)

./cache_sim --cache-size=256 --block-size=16 --assoc=2 --addr-bits=16 --input=teste.txt --verbose

Modo 4: Ajuda

./cache_sim --help

---

📋 Parâmetros

| Parâmetro | Descrição | Exemplo | Obrigatório |
| --- | --- | --- | --- |
| --cache-size=N | Tamanho total da cache em bytes | --cache-size=1024 | Sim |
| --block-size=N | Tamanho de cada bloco em bytes | --block-size=16 | Sim |
| --assoc=N | Associatividade (1=direta, N=N-way) | --assoc=2 | Sim |
| --addr-bits=N | Número de bits do endereço | --addr-bits=16 | Sim |
| --input=ARQUIVO | Caminho do arquivo de endereços | --input=acessos.txt | Sim |
| --verbose | Ativa modo detalhado | --verbose | Não |
| --help | Exibe ajuda | --help | Não |

Valores Válidos

- cache-size: 128, 256, 512, 1024, 2048, 4096 (potências de 2)
- block-size: 4, 8, 16, 32, 64 (potências de 2)
- assoc: 1 (direta), 2 (2-way), 4 (4-way), 8 (8-way)
- addr-bits: 8, 16, 32 (depende do sistema)

---

📂 Formato do Arquivo de Entrada

O arquivo deve conter um endereço por linha, em formato decimal ou hexadecimal (prefixo 0x). Linhas iniciadas com # são tratadas como comentários e ignoradas.

Exemplo (teste.txt):

# Exemplo de arquivo de endereços
# Formatos aceitos: decimal e hexadecimal
0x0010
0x0024
0x0010
256
0x00FF
16
0xA
4096

Regras:

- Um endereço por linha
- Decimal: 256, 1024, 16
- Hexadecimal: 0x0010, 0xFF, 0xA
- Comentários: linhas começando com #
- Linhas em branco são ignoradas

---

📊 Exemplos de Uso

Exemplo 1: Mapeamento Direto (Básico)

./cache_sim --cache-size=256 --block-size=16 --assoc=1 --addr-bits=16 --input=teste_direto.txt --verbose

Configuração:

- Cache: 256 bytes
- Bloco: 16 bytes
- 16 blocos, 16 conjuntos, 1 via por conjunto
- Decomposição: Tag=8 bits | Index=4 bits | Offset=4 bits

Resultado Esperado: ~30% de taxa de acerto (muitos conflitos de mapeamento)

---

Exemplo 2: Cache 2-Way Associativa (LRU)

./cache_sim --cache-size=256 --block-size=16 --assoc=2 --addr-bits=16 --input=teste_associativo.txt --verbose

Configuração:

- Cache: 256 bytes
- Bloco: 16 bytes
- 16 blocos, 8 conjuntos, 2 vias por conjunto
- Decomposição: Tag=9 bits | Index=3 bits | Offset=4 bits

Resultado Esperado: ~50% de taxa de acerto (menos conflitos)

---

Exemplo 3: Cache Grande 4-Way

./cache_sim --cache-size=1024 --block-size=32 --assoc=4 --addr-bits=16 --input=teste.txt

Configuração:

- Cache: 1024 bytes
- Bloco: 32 bytes
- 32 blocos, 8 conjuntos, 4 vias por conjunto
- Decomposição: Tag=7 bits | Index=3 bits | Offset=5 bits

---

📐 Tabela de Decomposição de Endereços

| Cache | Bloco | Assoc | Offset | Index | Tag | Conjuntos | Vias/Conj |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 256B | 16B | 1 (direta) | 4 bits | 4 bits | 8 bits | 16 | 1 |
| 256B | 16B | 2-way | 4 bits | 3 bits | 9 bits | 8 | 2 |
| 256B | 16B | 4-way | 4 bits | 2 bits | 10 bits | 4 | 4 |
| 1024B | 32B | 1 (direta) | 5 bits | 5 bits | 6 bits | 32 | 1 |
| 1024B | 32B | 2-way | 5 bits | 4 bits | 7 bits | 16 | 2 |
| 128B | 4B | 1 (direta) | 2 bits | 5 bits | 9 bits | 32 | 1 |

---

🎓 Explicação Teórica

Mapeamento Direto (assoc=1)

Cada bloco da memória principal pode ocupar apenas uma linha específica da cache. Se dois endereços diferentes mapeiam para o mesmo conjunto, ocorre um **conflito** e um bloco substitui o outro.

Vantagem: Simples e rápido

Desvantagem: Muitos conflitos, baixa taxa de acerto

Mapeamento Associativo (assoc=N)

A cache é dividida em conjuntos, e cada conjunto tem N vias. Um bloco pode ser alocado em qualquer via dentro do seu conjunto. Usa política **LRU** (Least Recently Used) para escolher qual via substituir.

Vantagem: Menos conflitos, maior taxa de acerto

Desvantagem: Hardware mais complexo

Política LRU

Quando todas as vias de um conjunto estão ocupadas, a via que foi acessada há mais tempo (menos recentemente usada) é escolhida para substituição.

---

🛡️ Tratamento de Erros

O programa valida e trata os seguintes erros:

| Erro | Mensagem | Causa |
| --- | --- | --- |
| Arquivo não encontrado | "Não foi possível abrir o arquivo" | Caminho incorreto ou arquivo inexistente |
| Endereço fora do range | "Endereço excede X bits" | Endereço maior que o espaço de endereçamento |
| Formato inválido | "Formato inválido" | String não numérica no arquivo |
| Cache não potência de 2 | "Cache deve ser potência de 2" | Valor como 100, 200, 500 |
| Bloco maior que cache | "Bloco maior que cache" | block-size > cache-size |
| Parâmetros inconsistentes | "Cache deve ser múltiplo do bloco" | 100 bytes / 16 bytes = inconsistente |

---

📁 Arquivos do Projeto

cache_sim/
├── main.cpp                # Código fonte principal
├── README.md               # Este arquivo
├── teste_direto.txt        # Arquivo de teste - mapeamento direto
└── teste_associativo.txt   # Arquivo de teste - cache associativa

---

🧪 Testes Realizados

Teste 1: Mapeamento Direto

- Entrada: 10 endereços com conflitos
- Cache: 256B, bloco 16B, assoc=1
- Resultado: 3 hits (30%), 7 misses (70%) ✅

Teste 2: Cache 2-Way Associativa

- Entrada: 12 endereços com conflitos
- Cache: 256B, bloco 16B, assoc=2
- Resultado: 6 hits (50%), 6 misses (50%) ✅

Teste 3: Validação de Erros

- Arquivo inexistente: ❌ Detectado ✅
- Endereço fora do range: ❌ Detectado ✅
- Parâmetros inválidos: ❌ Detectado ✅

Professor: Thiago Felski Pereira, M.Sc

UNIVALI - Universidade do Vale do Itajaí
