#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <semaphore>
#include <atomic>
#include <chrono>
#include <random>
#include <queue>

// Global variables for synchronization
constexpr int NUM_JOGADORES = 4;
std::counting_semaphore<NUM_JOGADORES> cadeira_sem(NUM_JOGADORES - 1); // Inicia com n-1 cadeiras, capacidade máxima n
std::condition_variable music_cv;
std::condition_variable proximaRodada_cv;
std::mutex rodada_mutex;
std::mutex music_mutex;
std::mutex impressao_mutex;
std::atomic<bool> musica_parada{false};
std::atomic<bool> jogo_ativo{true};
std::vector<int> listaEspera;
std::vector<int> idEliminados;
std::vector<int> jogadoresAssentados;
std::atomic<bool> eliminado{false};
std::atomic<bool> cadeirasOcupadas{false};
std::mutex jogadores_mutex;
int contadorRodadas = 0;
int contadorJogadoresOK = 1;
std::atomic<bool> ultimaRodada{false};



/*
 * Uso básico de um counting_semaphore em C++:
 * 
 * O `std::counting_semaphore` é um mecanismo de sincronização que permite controlar o acesso a um recurso compartilhado 
 * com um número máximo de acessos simultâneos. Neste projeto, ele é usado para gerenciar o número de cadeiras disponíveis.
 * Inicializamos o semáforo com `n - 1` para representar as cadeiras disponíveis no início do jogo. 
 * Cada jogador que tenta se sentar precisa fazer um `acquire()`, e o semáforo permite que até `n - 1` jogadores 
 * ocupem as cadeiras. Quando todos os assentos estão ocupados, jogadores adicionais ficam bloqueados até que 
 * o coordenador libere o semáforo com `release()`, sinalizando a eliminação dos jogadores.
 * O método `release()` também pode ser usado para liberar múltiplas permissões de uma só vez, por exemplo: `cadeira_sem.release(3);`,
 * o que permite destravar várias threads de uma só vez, como é feito na função `liberar_threads_eliminadas()`.
 *
 * Métodos da classe `std::counting_semaphore`:
 * 
 * 1. `acquire()`: Decrementa o contador do semáforo. Bloqueia a thread se o valor for zero.
 *    - Exemplo de uso: `cadeira_sem.acquire();` // Jogador tenta ocupar uma cadeira.
 * 
 * 2. `release(int n = 1)`: Incrementa o contador do semáforo em `n`. Pode liberar múltiplas permissões.
 *    - Exemplo de uso: `cadeira_sem.release(2);` // Libera 2 permissões simultaneamente.
 */

// Classes
class JogoDasCadeiras {
public:
    JogoDasCadeiras(int num_jogadores)
        : num_jogadores(num_jogadores), cadeiras(num_jogadores - 1) {}

    void iniciar_rodada() {
        // TODO: Inicia uma nova rodada, removendo uma cadeira e ressincronizando o semáforo
        imprimirInicio(getValuejogadores(), getValuecadeiras());
        return;    
    }

    void prepararProximarodada(){
        setValuejogadores(getValuejogadores() - 1); // reduzir número de jogadores
        setValuecadeiras(getValuecadeiras() - 1);  // reduzir número de cadeiras

        if(getValuecadeiras() == 1){
            ultimaRodada = true;
        }
        redefinirSemaforo(getValuecadeiras());      // atualizar número de tokens disponíveis no semáforo

        musica_parada = false;

        std::lock_guard<std::mutex> lock(jogadores_mutex);
        jogadoresAssentados.clear();
        listaEspera.clear();
        eliminado = false;
        cadeirasOcupadas = false;
        contadorRodadas++;
    }

    void redefinirSemaforo(int novoValor){
        int cadeirasAtuais = 0;
        while (cadeirasAtuais < novoValor){
            cadeira_sem.release();
            cadeirasAtuais++;
        }
        while (cadeirasAtuais > novoValor){
            cadeira_sem.release();
            cadeirasAtuais--;
        }
    }

    void imprimirInicio(int num_jogadores, int cadeiras){
        if(contadorRodadas == 0){
            std::cout << "PRIMEIRA RODADA" << std::endl;
            std::cout << "Iniciando rodada com " << getValuejogadores() << " jogadores e " << getValuecadeiras() << " cadeiras." << std::endl;
        }
        else{
            std::cout << "Próxima rodada com " << getValuejogadores() << " jogadores e " << getValuecadeiras() << " cadeiras." << std::endl;
        }
        std::cout << "A música está tocando... 🎵" << std::endl;
        std::cout << std::endl;
        return;
    }

    void parar_musica() {
        // TODO: Simula o momento em que a música para e notifica os jogadores via variável de condição
        std::unique_lock<std::mutex> lock(music_mutex);
        musica_parada = true;        
        music_cv.notify_all();
        
        std::lock_guard<std::mutex> lock3(impressao_mutex);
        std::cout << "> A música parou! Os jogadores estão tentando se sentar..." << std::endl;
        std::cout << std::endl;
        std::cout << "----------------------------------------------" << std::endl;
        return;
    }

    void eliminar_jogador(int jogador_id) {
        // TODO: Elimina um jogador que não conseguiu uma cadeira

        std::lock_guard<std::mutex> lock3(impressao_mutex);
        std::cout << "Jogador P" << jogador_id << " não conseguiu uma cadeira e foi eliminado!" << std::endl;
        std::cout << "----------------------------------------------" << std::endl;
        return;
    }

    void exibir_estado() {
        // TODO: Exibe o estado atual das cadeiras e dos jogadores
        std::lock_guard<std::mutex> lock(jogadores_mutex);
        std::lock_guard<std::mutex> lock3(impressao_mutex);
        for(int i = 1; i <= jogadoresAssentados.size(); i++){
           std::cout << "[Cadeira " << i << "]: Ocupada por P" << jogadoresAssentados[i-1] << std::endl;
        }
        std::cout << std::endl;
        return;
    }

    void anunciarVencedor(){
        std::lock_guard<std::mutex> lock(jogadores_mutex);
        std::lock_guard<std::mutex> lock3(impressao_mutex);
        std::cout << "🏆 Vencedor: Jogador P" << jogadoresAssentados[0] << "! Parabéns! 🏆" <<std::endl;
        std::cout << "----------------------------------------------" << std::endl;
        std::cout << "Obrigado por jogar o Jogo das Cadeiras Concorrente!" << std::endl;
        return;
    }

    void setValuejogadores(int novoNumjogadores){
        num_jogadores = novoNumjogadores;
    }

    int getValuejogadores() const{
        return num_jogadores;
    }

    void setValuecadeiras(int novoNumcadeiras){
        cadeiras = novoNumcadeiras ;
    }

    int getValuecadeiras() const{
        return cadeiras;
    }
 

private:
    int num_jogadores;
    int cadeiras;
};

class Jogador {
public:
    Jogador(int id, JogoDasCadeiras& jogo)
        : id(id), jogo(jogo) {}

    void tentar_ocupar_cadeira() {
        // TODO: Tenta ocupar uma cadeira utilizando o semáforo contador quando a música para (aguarda pela variável de condição)
        if(cadeira_sem.try_acquire()){
            std::lock_guard<std::mutex> lock(jogadores_mutex);
            jogadoresAssentados.push_back(id);
            listaEspera.pop_back();

            //std::cout << "Sentado" << id << std::endl;

            if(ultimaRodada){
                jogo_ativo = false;
            }
        }
        else{
            //std::cout << "Jogador FORA:" << id << std::endl;
            cadeirasOcupadas = true;
        }
    }

    void verificar_eliminacao() {
        // TODO: Verifica se foi eliminado após ser destravado do semáforo        
        std::lock_guard<std::mutex> lock(jogadores_mutex);
        if(listaEspera.back() == id){
            idEliminados.push_back(id); // coloca id na lista de eliminados
            listaEspera.pop_back();     // esvazia a lista de espera
            eliminado = true;
            return;
        }
        
        return;
    }

    void joga() {
        while(contadorRodadas < 3){
            // TODO: Aguarda a música parar usando a variavel de condicao
            std::unique_lock<std::mutex> lock(music_mutex);
            listaEspera.push_back(id);
            music_cv.wait(lock);

            // TODO: Tenta ocupar uma cadeira
            if(musica_parada){
                tentar_ocupar_cadeira();
            }
            
            // TODO: Verifica se foi eliminado
            if(cadeirasOcupadas){
                verificar_eliminacao();
            }

            if(eliminado){
                jogo.eliminar_jogador(id);

                std::unique_lock<std::mutex> lock2(rodada_mutex);
                proximaRodada_cv.notify_all(); 
                break;
            }
            
            std::unique_lock<std::mutex> lock2(rodada_mutex);
            proximaRodada_cv.notify_all(); // avisar a Thread coordenadora que o jogador já terminou essa rodada
            //std::cout << "Jogador OK" << id << std::endl;
        }
    }

private:
    int id;
    JogoDasCadeiras& jogo;
};

int tempoVariavel(int min, int max){
    std::random_device numero;
    std::mt19937 gerar(numero());
    std::uniform_int_distribution<> distrubuicao(min, max);

    return distrubuicao(gerar);
}

class Coordenador {
public:
    Coordenador(JogoDasCadeiras& jogo)
        : jogo(jogo) {}

    void iniciar_jogo() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // TODO: Começa o jogo, dorme por um período aleatório, e então para a música, sinalizando os jogadores 
        std::cout << "----------------------------------------------- " << std::endl;
        std::cout << "  Bem-vindo ao Jogo das Cadeiras Concorrente.   " << std::endl;
        std::cout << "----------------------------------------------- " << std::endl;
        std::cout << std::endl;

        //while((jogo_ativo) && (contadorRodadas < 3)){
        while(contadorRodadas < 3){
        //while(jogo_ativo){
            // Começar o jogo
            jogo.iniciar_rodada();
            
            if(!jogo_ativo){
                jogo.anunciarVencedor();
                break;
            }

            // Espera um tempo aleatório
            int tempo = tempoVariavel(100, 150);   // Gerar número aleatório
            std::this_thread::sleep_for(std::chrono::milliseconds(tempo));

            // Desliga a música
            jogo.parar_musica();

            // Exibe Status da rodada
            if(cadeirasOcupadas){
                jogo.exibir_estado();
            }

            while(contadorJogadoresOK < jogo.getValuejogadores()){
                std::unique_lock<std::mutex> lock2(rodada_mutex);
                proximaRodada_cv.wait(lock2);
                contadorJogadoresOK++;
            }
        
        
            // Reiniciar para a próxima rodada
            jogo.prepararProximarodada();
            //liberar_threads_eliminadas();
        }
    }

    void liberar_threads_eliminadas() {
        // Libera múltiplas permissões no semáforo para destravar todas as threads que não conseguiram se sentar
        std::lock_guard<std::mutex> lock(jogadores_mutex);
        cadeira_sem.release(idEliminados.size()); // Libera o número de permissões igual ao número de jogadores que ficaram esperando
    }

private:
    JogoDasCadeiras& jogo;
};

// Main function
int main() {
    JogoDasCadeiras jogo(NUM_JOGADORES);
    Coordenador coordenador(jogo);
    std::vector<std::thread> jogadores;

    // Criação das threads dos jogadores
    std::vector<Jogador> jogadores_objs;
    for (int i = 1; i <= NUM_JOGADORES; i++) {
        jogadores_objs.emplace_back(i, jogo);
    }

    for (int i = 0; i < NUM_JOGADORES; i++) {
        jogadores.emplace_back(&Jogador::joga, &jogadores_objs[i]);
    }

    // Thread do coordenador
    std::thread coordenador_thread(&Coordenador::iniciar_jogo, &coordenador);

    
    // Esperar pelas threads dos jogadores
    for (auto& t : jogadores) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Esperar pela thread do coordenador
    if (coordenador_thread.joinable()) {
        coordenador_thread.join();
    }
    

    std::cout << "Jogo das Cadeiras finalizado." << std::endl;
    return 0;
}

