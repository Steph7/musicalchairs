#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <semaphore>
#include <atomic>
#include <chrono>
#include <random>
#include <algorithm>

// Global variables for synchronization
constexpr int NUM_JOGADORES = 4;
std::counting_semaphore<NUM_JOGADORES> cadeira_sem(NUM_JOGADORES - 1); // Inicia com n-1 cadeiras, capacidade máxima n
std::condition_variable music_cv;
std::condition_variable proximaRodada_cv;
std::mutex rodada_mutex;
std::mutex music_mutex;
std::mutex jogadores_mutex;
std::atomic<bool> musica_parada{false};
std::atomic<bool> jogo_ativo{true};
std::atomic<bool> eliminado{false};
std::atomic<bool> cadeirasOcupadas{false};
int proxima_rodada = 0;
int proxima_rodadaJ = 0;

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

    // Manipular dados numero de jogadores
    void setValuejogadores(int novoNumjogadores){
        num_jogadores = novoNumjogadores;
    }

    int getValuejogadores(){
        return num_jogadores;
    }

    // Manipular dados numero de cadeiras
    void setValuecadeiras(int novoNumcadeiras){
        cadeiras = novoNumcadeiras ;
    }

    int getValuecadeiras(){
        return cadeiras;
    }

    // Manipular dados lista de espera
    void adicionarEspera(int novoElemento){
        listaEspera.push_back(novoElemento);
    }

    void removerEspera(int elemento){
        auto x = std::find(listaEspera.begin(), listaEspera.end(), elemento);

        if(x != listaEspera.end()){
            listaEspera.erase(x);
        }
    }    

    void limparEspera(){
        listaEspera.clear();
    }

    // Manipular dados Assentados
    void adicionarAssentados(int novoElemento){
        jogadoresAssentados.push_back(novoElemento);
    }

    void removerAssentados(int elemento){
        auto x = std::find(jogadoresAssentados.begin(), jogadoresAssentados.end(), elemento);

        if(x != jogadoresAssentados.end()){
            jogadoresAssentados.erase(x);
        }
    }

    void limparAssentados(){
        jogadoresAssentados.clear();
    }

    // Manipular dados Eliminados
    void adicionarEliminados(int novoElemento){
        listaEliminados.push_back(novoElemento);
    }

    void removerEliminados(int elemento){
        auto x = std::find(listaEliminados.begin(), jogadoresAssentados.end(), elemento);

        if(x != listaEliminados.end()){
            listaEliminados.erase(x);
        }
    }

    void limparEliminados(){
        listaEliminados.clear();
    }

    void saudacaoInicial(){
        std::cout << "----------------------------------------------- " << std::endl;
        std::cout << "  Bem-vindo ao Jogo das Cadeiras Concorrente.   " << std::endl;
        std::cout << "----------------------------------------------- " << std::endl;
        std::cout << std::endl;
    }
    
    void imprimirInicio(int num_jogadores, int cadeiras){
        if(getValuecadeiras() == 3){
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
        std::cout << "> A música parou! Os jogadores estão tentando se sentar..." << std::endl;
        std::cout << std::endl;
        std::cout << "----------------------------------------------" << std::endl;
        return;
    }

    void eliminar_jogador(int jogador_id) {
        // TODO: Elimina um jogador que não conseguiu uma cadeira
        std::cout << "Jogador P" << listaEspera[jogador_id] << " não conseguiu uma cadeira e foi eliminado!" << std::endl;
        std::cout << "----------------------------------------------" << std::endl;
        return;
    }

    void exibir_estado() {
        // TODO: Exibe o estado atual das cadeiras e dos jogadores
        for(int i = 0; i < jogadoresAssentados.size(); i++){
           std::cout << "[Cadeira " << i + 1 << "]: Ocupada por P" << jogadoresAssentados[i] << std::endl;
        }
        std::cout << std::endl;
        return;
    }

    void anunciarVencedor(){
        std::cout << "🏆 Vencedor: Jogador P" << jogadoresAssentados[0] << "! Parabéns! 🏆" <<std::endl;
        std::cout << "----------------------------------------------" << std::endl;
        std::cout << "Obrigado por jogar o Jogo das Cadeiras Concorrente!" << std::endl;
        return;
    }

    void prepararProximarodada(){
        setValuejogadores(getValuejogadores() - 1); // reduzir número de jogadores
        setValuecadeiras(getValuecadeiras() - 1);  // reduzir número de cadeiras

        cadeira_sem.release(getValuecadeiras()); // libera a quantidade de tokens correspondentes à próxima etapa

        eliminado = false;
        cadeirasOcupadas = false;
        limparAssentados();
        limparEspera();
    }

private:
    std::vector<int> listaEliminados;
    std::vector<int> listaEspera;
    std::vector<int> jogadoresAssentados;
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
            if(!cadeirasOcupadas){
                jogo.adicionarAssentados(getId());
                jogo.removerEspera(getId());                  
            }
        }
        else{
            cadeirasOcupadas = true;
            verificar_eliminacao();
        }
    }

    void verificar_eliminacao() {
        // TODO: Verifica se foi eliminado após ser destravado do semáforo
        if(cadeirasOcupadas){
            jogo.removerEspera(getId());
            jogo.adicionarEliminados(getId());
            eliminado = true;
        }
        else{
            eliminado = false;
        }
    }

    int getId(){
        return id;
    }

private:
    int id;
    JogoDasCadeiras& jogo;
};

class Coordenador {
public:
    Coordenador(JogoDasCadeiras& jogo)
        : jogo(jogo) {}

    void liberar_threads_eliminadas() {
        // Libera múltiplas permissões no semáforo para destravar todas as threads que não conseguiram se sentar
        cadeira_sem.release(NUM_JOGADORES - 1); // Libera o número de permissões igual ao número de jogadores que ficaram esperando
    }

private:
    JogoDasCadeiras& jogo;
};


void acoesJogador(JogoDasCadeiras& jogo, Jogador& jogador){

    // TODO: Aguarda a música parar usando a variavel de condicao
    std::unique_lock<std::mutex> lock(music_mutex);
    jogo.adicionarEspera(jogador.getId());    
    while(!musica_parada){    
        music_cv.wait(lock);       
    }

    // TODO: Tenta ocupar uma cadeira
    //std::unique_lock<std::mutex> lock2(jogadores_mutex);
    jogador.tentar_ocupar_cadeira();

    // TODO: Verifica se foi eliminado
    while(eliminado){                
        // Avisa Coordenadora que já terminou jogada
        std::unique_lock<std::mutex> lock3(rodada_mutex);
        proximaRodada_cv.notify_all(); 
        break;
    }

    // Avisa Coordenadora que já terminou jogada
    std::unique_lock<std::mutex> lock3(rodada_mutex);
    proximaRodada_cv.notify_all(); 

    return;
}

int tempoVariavel(int min, int max){
    std::random_device numero;
    std::mt19937 gerar(numero());
    std::uniform_int_distribution<> distrubuicao(min, max);

    return distrubuicao(gerar);
}

void gerenciarJogo(JogoDasCadeiras& jogo, Coordenador& coordenador){

    // Inicia o jogo
    jogo.iniciar_rodada();

    // Espera um tempo (aleatório)
    int tempo = tempoVariavel(100, 150);   // Gerar número aleatório
    std::this_thread::sleep_for(std::chrono::milliseconds(tempo));

    // Para a música
    //std::unique_lock<std::mutex> lock(music_mutex);
    musica_parada = true;        
    music_cv.notify_all();
    jogo.parar_musica();

    // Aguarda sinalização dos jogadores
    int jogadoresProntos = 0;
    while(jogadoresProntos < jogo.getValuejogadores()){
        std::unique_lock<std::mutex> lock2(rodada_mutex);
        proximaRodada_cv.wait(lock2);
        jogadoresProntos++;
    }

    // Exibe Status da rodada
    jogo.exibir_estado();

    // Exibe Jogador Eliminado
    jogo.eliminar_jogador(proxima_rodada);

    // Anunciar o vencedor (caso esteja na ultima rodada)
    if(proxima_rodada == NUM_JOGADORES - 1){
        jogo.anunciarVencedor();
    }

    // Prepara para começar nova partida
    jogo.prepararProximarodada();
    return;
}

void jogarJogo(JogoDasCadeiras& jogo, Coordenador& coordenador){

    // Saudação inicial
    jogo.saudacaoInicial();

    while(proxima_rodada < NUM_JOGADORES){

        gerenciarJogo(std::ref(jogo), std::ref(coordenador));

        
    }
}

void jogarJogador(JogoDasCadeiras& jogo, Jogador& jogador){

    while(proxima_rodadaJ < NUM_JOGADORES){

        acoesJogador(std::ref(jogo), std::ref(jogador));
        
    }
}


// Main function
int main() {
    JogoDasCadeiras jogo(NUM_JOGADORES);
    Coordenador coordenador(jogo);
    std::vector<std::thread> jogadores;

    // Thread do coordenador
    std::thread coordenador_thread(jogarJogo, std::ref(jogo), std::ref(coordenador));

    // Criação das threads dos jogadores
    std::vector<Jogador> jogadores_objs;
    for (int i = 1; i <= NUM_JOGADORES; ++i) {
        jogadores_objs.emplace_back(i, jogo);
    }

    for (int i = 0; i < NUM_JOGADORES; ++i) {
        jogadores.emplace_back(jogarJogador, std::ref(jogo), std::ref(jogadores_objs[i]));
    }

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
