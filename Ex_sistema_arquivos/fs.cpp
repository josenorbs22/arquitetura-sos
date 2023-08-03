#ifndef fs_h
#define fs_h
#include <string>
#include <cstring>
#include <iostream>

typedef struct {
    int estaNaLista;
    char nome[20];
    int proximo;
} Nodo;

/**
 * @param arquivoDaLista nome do arquivo em disco que contem a lista encadeada
 * @param novoNome nome a ser adicionado apos depoisDesteNome
 * @param depoisDesteNome um nome presente na lista
 */
void adiciona(std::string arquivoDaLista, std::string novoNome, std::string depoisDesteNome)
{
    //implemente aqui
    FILE* file = nullptr;
    //arquivoDaLista = std::string("../").append(arquivoDaLista);
    file = fopen(arquivoDaLista.c_str(), "rb+");

    //encontrar o novo nodo
    int primeiro{0x00};
    int byteslidos{0x00};
    byteslidos = fread((void*)&primeiro, sizeof(int), 1, file);

    bool encontrou{false};

    Nodo novoNodo{};
    long int enderecoNovo, enderecoAntigo;
    while(!encontrou){
        enderecoNovo = ftell(file);
        byteslidos = fread(&novoNodo, sizeof(Nodo), 1, file);
        if(byteslidos < 1) abort();
        encontrou = (novoNodo.estaNaLista == 0);
    }

    
    encontrou = false;
    Nodo anterior;
    
    while(!encontrou){
        enderecoAntigo = ftell(file);
        byteslidos = fread(&anterior, sizeof(Nodo), 1, file);
        if(byteslidos < 1) abort();
        if(depoisDesteNome.compare(std::string(anterior.nome))==0) encontrou = true;
        else fseek(file, anterior.proximo, SEEK_SET);
    }

    rewind(file);
    fseek(file, enderecoNovo, SEEK_SET);
    int endereco = ftell(file);
    //rewind(file);
    
    novoNodo.estaNaLista = 1;
    strcpy(novoNodo.nome, novoNome.c_str());
    novoNodo.proximo = anterior.proximo;
    anterior.proximo = enderecoNovo;

    fwrite(&novoNodo.estaNaLista, sizeof(int), 1, file);
    endereco = ftell(file);
    fwrite(&novoNodo.nome, sizeof(char), 20, file);
    endereco = ftell(file);
    fwrite(&novoNodo.proximo, sizeof(int), 1, file);
    endereco = ftell(file);
    rewind(file);
    fseek(file, enderecoAntigo + sizeof(int) + 20, SEEK_SET);
    endereco = ftell(file);
    fwrite(&enderecoNovo, sizeof(int), 1, file);

    fclose(file);
    //gravar novo nodo

    //atualizar
    //1 - encontrar depois do nome
    //2 - NovoNodo->Proximo = depoisDesteNome->Proximo
    //3 - depoisDesteNome->Proximo = 
}

#endif /* fs_h */