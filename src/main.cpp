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
std::counting_semaphore<> jogadoresProntos(0);
std::condition_variable music_cv;
std::condition_variable proximaRodada_cv;
std::condition_variable jogadorPronto_cv;
std::mutex ocuparCadeira_mutex;
std::mutex rodada_mutex;
std::mutex music_mutex;
std::mutex pronto_mutex;
std::atomic<bool> musica_parada{false};
std::atomic<bool> jogo_ativo{true};
std::atomic<bool> cadeirasOcupadas{false};
std::atomic<bool> vencedorEncontrado{false};
std::atomic<bool> eliminadoEncontrado{false};
bool proxima = false;
int proxima_rodada = 0;


int tempoVariavel(int min, int max){
    std::random_device numero;
    std::mt19937 gerar(numero());
    std::uniform_int_distribution<> distrubuicao(min, max);

    return distrubuicao(gerar);
}

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

    // Registrar e Manipular Vencedor
    void setVencedor(int valor){
        vencedor = valor;
    }

    int getVencedor(){
        return vencedor;
    }

    // Registrar e Manipular Eliminado
    void setEliminado(int valor){
        eliminado = valor;
    }

    int getEliminado(){
        return eliminado;
    }

    // Alterar Sinais de controle
    void setSinalcontrole(int id, int valor){
        sinalControle[id] = valor;
    }

    int getSinalcontrole(int id){
        return sinalControle[id];
    }

    void limparSinais(){
        sinalControle = {0, 0, 0, 0};
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

    void eliminar_jogador() {
        // TODO: Elimina um jogador que não conseguiu uma cadeira
        std::cout << "Jogador P" << eliminado << " não conseguiu uma cadeira e foi eliminado!" << std::endl;
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
        std::cout << "🏆 Vencedor: Jogador P" << vencedor << "! Parabéns! 🏆" <<std::endl;
        std::cout << "----------------------------------------------" << std::endl;
        std::cout << "Obrigado por jogar o Jogo das Cadeiras Concorrente!" << std::endl;
        return;
    }

    void prepararProximarodada(){
        setValuejogadores(getValuejogadores() - 1); // reduzir número de jogadores
        setValuecadeiras(getValuecadeiras() - 1);  // reduzir número de cadeiras

        cadeira_sem.release(getValuecadeiras()); // libera a quantidade de tokens correspondentes à próxima etapa

        setEliminado(0); // zerar o id de eliminado para a próxima rodada
        eliminadoEncontrado = false;
        cadeirasOcupadas = false;
        proxima = false;
        limparAssentados();
        //limparEspera();
        limparSinais();
        return;
    }

private:
    std::vector<int> sinalControle = {0, 0, 0, 0};
    std::vector<int> listaEliminados;
    std::vector<int> listaEspera;
    std::vector<int> jogadoresAssentados;
    int num_jogadores;
    int cadeiras;
    int vencedor;
    int eliminado;
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

                if(jogo.getValuecadeiras() == 1){
                    jogo.setVencedor(getId());
                    vencedorEncontrado = true;
                }                 
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
            jogo.setEliminado(getId());
            eliminadoEncontrado = true;
        }
        else{
            eliminadoEncontrado = false;
        }
    }

    int getId(){
        return id;
    }

    void joga() {

        while(!vencedorEncontrado){    
            jogo.adicionarEspera(getId()); 
            std::unique_lock<std::mutex> lock3(rodada_mutex);
            proximaRodada_cv.wait(lock3, [this]() { return proxima || jogo.getSinalcontrole(getId()-1) == 0; });

            // TODO: Aguarda a música parar usando a variavel de condicao
            std::unique_lock<std::mutex> lock(music_mutex);
            while(!musica_parada){    
                music_cv.wait(lock);       
            }

            // TODO: Tenta ocupar uma cadeira
            std::lock_guard<std::mutex> bloquear(ocuparCadeira_mutex);
            tentar_ocupar_cadeira();

            // Avisa Coordenadora que já terminou jogada
            std::unique_lock<std::mutex> lock2(pronto_mutex);
            
            jogo.setSinalcontrole(getId()-1, 1);

            // TODO: Verifica se foi eliminado
            if(eliminadoEncontrado){  
                //jogadorPronto_cv.notify_all(); 
                jogadoresProntos.release();            
                break; // eliminar thread
            }

            if(vencedorEncontrado){
                //jogadorPronto_cv.notify_all(); 
                jogadoresProntos.release(); 
                break; // eliminar thread
            }
            
            //jogadorPronto_cv.notify_all();
            jogadoresProntos.release();  
        }
    }

private:
    int id;
    JogoDasCadeiras& jogo;
};

class Coordenador {
public:
    Coordenador(JogoDasCadeiras& jogo)
        : jogo(jogo) {}

    void iniciar_jogo() {
        // TODO: Começa o jogo, dorme por um período aleatório, e então para a música, sinalizando os jogadores 
        jogo.setVencedor(0); // zera o vencedor
        jogo.setEliminado(0);
        // Saudação inicial
        jogo.saudacaoInicial();

        while(proxima_rodada < NUM_JOGADORES-1){
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
            /*
            int jogador = 0; 

            std::unique_lock<std::mutex> lock2(pronto_mutex);            
            while(jogador < jogo.getValuejogadores()){ 
                jogadorPronto_cv.wait(lock2);
                jogador++;
            }
            */
            for(int i = 0; i < jogo.getValuejogadores(); i++){
                jogadoresProntos.acquire();
            }

            // Exibe Status da rodada
            jogo.exibir_estado();

            // Exibe Jogador Eliminado
            jogo.eliminar_jogador();

            // Anunciar o vencedor (caso esteja na ultima rodada)
            if(vencedorEncontrado){
                jogo.anunciarVencedor();
                proximaRodada_cv.notify_all();
                break;
            }
            else{
            // Prepara para começar nova partida
            proxima = true;
            proximaRodada_cv.notify_all();
            jogo.prepararProximarodada();
            proxima_rodada++; 
            }  
        }
    }

    void liberar_threads_eliminadas() {
        // Libera múltiplas permissões no semáforo para destravar todas as threads que não conseguiram se sentar
        cadeira_sem.release(NUM_JOGADORES - 1); // Libera o número de permissões igual ao número de jogadores que ficaram esperando
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
    for (int i = 1; i <= NUM_JOGADORES; ++i) {
        jogadores_objs.emplace_back(i, jogo);
    }

    for (int i = 0; i < NUM_JOGADORES; ++i) {
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

