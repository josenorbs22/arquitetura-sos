/**
 * Implemente aqui as funções dos sistema de arquivos que simula EXT3
 */

#include "fs.h"
#include <iostream>
#include <string.h>
#include <cstring>
#include <cmath>


INODE searchInode(FILE *file, int inodeAdress, int posInicial){
    INODE inode{};
    fseek(file, posInicial, SEEK_SET);
    fseek(file, sizeof(INODE) * inodeAdress, SEEK_CUR);
    fread(&inode.IS_USED, sizeof(char), 1, file);
    fread(&inode.IS_DIR, sizeof(char), 1, file);
    fread(&inode.NAME, sizeof(char) * 10, 1, file);
    fread(&inode.SIZE, sizeof(char), 1, file);
    fread(&inode.DIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    fread(&inode.INDIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    fread(&inode.DOUBLE_INDIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    return inode;
}

void searchBlock(char* block, FILE *file, int blockAdress, int blockSize, int posInicial){
    fseek(file, posInicial, SEEK_SET);
    fseek(file, blockSize * blockAdress, SEEK_CUR);
    fread(block, sizeof(char) * blockSize, 1, file);
}

int searchFreeBlock(int &mapa, int tamanhoMapa){
    for (int i = 0; i < 8 * tamanhoMapa; i++){
        if(!(mapa & (1 << i))){
            mapa = mapa | (1 << i);
            return i;
        }
    }
    return -1;
}

void copiarDireitaEsquerda(FILE *file, int inodeExcluidoAdress, INODE &inode, int blocoInodeExcluido, int blockSize, int posInicial, int &mapa){
    char* inodeBlock = new char(sizeof(char) * blockSize);
    char* inodeIBlock = new char(sizeof(char) * blockSize);
    char* inodeDIBlock = new char(sizeof(char) * blockSize);
    char dummy1[blockSize];
    for(int i = blocoInodeExcluido; i < 9; i++){
        if((inode.DIRECT_BLOCKS[i] == 0 && i != 0) || (i > 2 && inode.INDIRECT_BLOCKS[i - 3] == 0) || (i > 8 && inode.DOUBLE_INDIRECT_BLOCKS[i - 6] == 0))
            break;
        /*Procura o conteudo do bloco que contem o endereço do inode excluído*/
        if(i < 3){
            searchBlock(inodeBlock, file, inode.DIRECT_BLOCKS[i], blockSize, posInicial);
            for (int j = 0; j < blockSize; j++){
                dummy1[j] = inodeBlock[j];
                if(inodeBlock[j] == inodeExcluidoAdress){
                    if(j != blockSize - 1){
                        inodeBlock[j] = inodeBlock[j + 1];
                        dummy1[j] = inodeBlock[j];
                    }
                } else if(inodeBlock[j] != 0){
                    if(i == 2){ 
                        searchBlock(inodeIBlock, file, inode.INDIRECT_BLOCKS[0], blockSize, posInicial);
                        searchBlock(inodeBlock, file, inodeIBlock[0], blockSize, posInicial);
                        dummy1[j] = inodeBlock[0];
                        if(inodeBlock[1] == 0 && i != 0){
                            mapa &= ~(1 << inode.INDIRECT_BLOCKS[0]);
                            mapa &= ~(1 << inodeIBlock[0]);
                        }
                    } else {
                        if(inode.DIRECT_BLOCKS[i + 1] != 0){
                            searchBlock(inodeBlock, file, inode.DIRECT_BLOCKS[i + 1], blockSize, posInicial);
                            dummy1[j] = inodeBlock[0];
                            if(inodeBlock[1] == 0){
                                mapa &= ~(1 << inode.DIRECT_BLOCKS[i + 1]);
                                inode.DIRECT_BLOCKS[i + 1] = 0;
                            }
                        }
                        
                    }
                }
            }
            fseek(file, posInicial, SEEK_SET);
            fseek(file, inode.DIRECT_BLOCKS[i] * blockSize, SEEK_CUR);
            fwrite(&dummy1, sizeof(char) * blockSize, 1, file);
            /* if(i != 2 && ceil(inode.SIZE / (double)blockSize) - blocoInodeExcluido == 0){
                if(i != 0){
                    mapa = mapa & (0 << inode.DIRECT_BLOCKS[i]);
                    inode.DIRECT_BLOCKS[i] = 0;
                }
            } */
        } else if(i < 6){
            searchBlock(inodeIBlock, file, inode.INDIRECT_BLOCKS[i - 3], blockSize, posInicial);
            for(int j = 0; j < blockSize; j++){
                searchBlock(inodeBlock, file, inodeIBlock[j], blockSize, posInicial);
                for (int k = 0; k < blockSize; k++){
                    if(inodeBlock[k] == 0){
                        if(k != blockSize - 1){
                            inodeBlock[k] = inodeBlock[k + 1];
                        } else{
                            inodeBlock[k] = 0;
                        }
                    }
                }
                fseek(file, posInicial, SEEK_SET);
                fseek(file, inodeIBlock[j] * blockSize, SEEK_CUR);
                fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                if((ceil((inode.SIZE - 3) / (double)blockSize) - blocoInodeExcluido - 3) != 0){
                    searchBlock(inodeDIBlock, file, inode.DOUBLE_INDIRECT_BLOCKS[0], blockSize, posInicial);
                    searchBlock(inodeIBlock, file, inodeDIBlock[0], blockSize, posInicial);
                    searchBlock(inodeBlock, file, inodeIBlock[0], blockSize, posInicial);
                    inodeBlock[0] = 0;
                    fseek(file, posInicial, SEEK_SET);
                    fseek(file, inodeIBlock[0] * blockSize, SEEK_CUR);
                    fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                } else {
                    if(j == 0){
                        mapa = mapa & (0 << inodeIBlock[j]);
                    }
                }
            }
        } else if(i < 9){
            searchBlock(inodeDIBlock, file, inode.DOUBLE_INDIRECT_BLOCKS[i - 6], blockSize, posInicial);
            for(int j = 0; j < blockSize; j++){
                searchBlock(inodeIBlock, file, inodeDIBlock[j], blockSize, posInicial);
                for(int k = 0; k < blockSize; k++){
                    searchBlock(inodeBlock, file, inodeIBlock[k], blockSize, posInicial);
                    for (int l = 0; l < blockSize; l++){
                        if(inodeBlock[l] == 0){
                            if(l != blockSize - 1){
                                inodeBlock[l] = inodeBlock[l + 1];
                            } else{
                                inodeBlock[l] = 0;
                            }
                        }
                    }
                    fseek(file, posInicial, SEEK_SET);
                    fseek(file, inodeIBlock[k] * blockSize, SEEK_CUR);
                    fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                    if((ceil((inode.SIZE - 12) / (double)blockSize) - blocoInodeExcluido - 6) == 0){
                        if(k == 0){
                            mapa = mapa & (0 << inodeIBlock[k]);
                        }
                    }
                }
            }
        }
    }
    delete inodeBlock;
    delete inodeIBlock;
    delete inodeDIBlock;
}
/**
 * @brief Inicializa um sistema de arquivos que simula EXT3
 * @param fsFileName nome do arquivo que contém sistema de arquivos que simula EXT3 (caminho do arquivo no sistema de arquivos local)
 * @param blockSize tamanho em bytes do bloco
 * @param numBlocks quantidade de blocos
 * @param numInodes quantidade de inodes
 */
void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes){
    FILE* file = fopen(fsFileName.c_str(), "wb");
    int fileSize = 3 + ceil(numBlocks/8.0) + sizeof(INODE)*numInodes + 1 + blockSize*numBlocks;
    char zero{0x00}; 

    for(int i{0}; i < fileSize; i++){
        fwrite(&zero, sizeof(char), 1, file);
    }
    rewind(file);
    
    zero = (char)blockSize;
    fwrite(&zero, sizeof(char), 1, file);
    zero = (char)numBlocks;
    fwrite(&zero, sizeof(char), 1, file);
    zero = (char)numInodes;
    fwrite(&zero, sizeof(char), 1, file);

    zero = 0x00;
    fwrite(&zero, sizeof(char), 1, file);
    
    char tamanhoMapa = ceil(numBlocks/8.0);
    zero = 0x00;

    for(int i = 1; i < tamanhoMapa; i++){
        fwrite(&zero, sizeof(char), 1, file);
    }
    int mapa{0x01};
    INODE root{};
    root.IS_DIR = 0x01;
    root.IS_USED = 0x01;
    root.NAME[0] = '/';

    fseek(file, 3, SEEK_SET);
    fwrite(&mapa, tamanhoMapa, 1, file);
    fwrite(&root, sizeof(INODE), 1, file);
    fclose(file);
}

/**
 * @brief Adiciona um novo arquivo dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param filePath caminho completo novo arquivo dentro sistema de arquivos que simula EXT3.
 * @param fileContent conteúdo do novo arquivo
 */
void addFile(std::string fsFileName, std::string filePath, std::string fileContent){
    if((filePath[0] != '/') || (filePath[filePath.size() - 1] == '/')) std::cout << "endereco invalido";
    FILE* file = fopen(fsFileName.c_str(), "rw+");
    rewind(file);

    int blockSize{};
    int numBlocks{0x00};
    int numInodes{0x00};
    int mapa{0x00};
    std::string fileName;

    fread(&blockSize, sizeof(char), 1, file);
    fread(&numBlocks, sizeof(char), 1, file);
    fread(&numInodes, sizeof(char), 1, file);

    unsigned int tamanhoMapa = ceil(numBlocks / 8.0);
    fread(&mapa, tamanhoMapa, 1, file);

    int posInicialInode = sizeof(char) * 3 + tamanhoMapa;
    int posInicialBlock = posInicialInode + sizeof(INODE) * numInodes + 1;

    unsigned int posInode{0x00};
    unsigned int numPais = 1;

    for(int i = 1; i < filePath.size(); i++){
        if(filePath[i] == '/'){
            numPais++;
        }
    }
    std::string nomesPais[numPais];
    nomesPais[0] = '/';
    std::string buffer;
    int cont = 1;

    for(int i = 1; i < filePath.size(); i++){
        if(filePath[i] != '/'){
            buffer = buffer + filePath[i];
        } else{
            nomesPais[cont] = buffer;
            buffer = "";
            cont++;
        }
        if(i == filePath.size() - 1){
            fileName = buffer;
        }
    }

    char* inodeDIBlock = new char(sizeof(char)* blockSize);
    char* inodeIBlock = new char(sizeof(char)* blockSize);
    char* inodeBlock = new char(sizeof(char)* blockSize);
    int posInodePai{0x00};
    INODE inodePai{};
    fread(&inodePai.IS_USED, sizeof(char), 1, file);
    fread(&inodePai.IS_DIR, sizeof(char), 1, file);
    fread(&inodePai.NAME, sizeof(char) * 10, 1, file);
    fread(&inodePai.SIZE, sizeof(char), 1, file);
    fread(&inodePai.DIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    fread(&inodePai.INDIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    fread(&inodePai.DOUBLE_INDIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    
    for(int i = 1; i < numPais; i++){
        //percorre cada um dos inodes pais para encontrar o inode que deve ser atualizado
        for(int j = 0; j < ceil(inodePai.SIZE / (double)blockSize) ; j++){
            //percorre cada um dos blocos do inode atual do loop (inodePai) para achar os inodes filhos
            if(j > 12){
                //verifica se o inodePai tem tamanho o suficiente para ter que ocupar blocos indiretos duplos
                for(int k = 0; k < 3; k++){
                    if(inodePai.IS_DIR == 0x01 && i != numPais)
                        searchBlock(inodeDIBlock, file, inodePai.INDIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    for(int l = 0; l < blockSize ; l++){
                        if(inodePai.IS_DIR == 0x01 && i != numPais)
                            searchBlock(inodeIBlock, file, inodeDIBlock[l], blockSize, posInicialBlock);
                        for(int m = 0; m < blockSize; m++){
                            //percorre todos os blocos diretos
                            if(inodePai.IS_DIR == 0x01 && i != numPais)
                                searchBlock(inodeBlock, file, inodeIBlock[m], blockSize, posInicialBlock);
                            /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                            com o mesmo tamanho do bloco em inodeBlock
                            */
                            for(int n = 0; n < blockSize; n++){
                                //percorre todo o o conteudo de inodeBlock
                                inodePai = searchInode(file, inodeBlock[n], posInicialInode);
                                //acha o inode apontado em cada valor de inodeBlock
                                if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1){
                                    /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                                    quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                                    inodePai atual
                                    */
                                    posInodePai = inodeBlock[n];
                                    break;
                                }
                            }
                            if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1)
                                break;
                        }
                    }
                }
            } else if(j > 3){
                //verifica se o inodePai tem tamanho o suficiente para ter que ocupar blocos indiretos
                for(int k = 0; k < 3 ; k++){
                    if(inodePai.IS_DIR == 0x01 && i != numPais)
                        searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    for(int l = 0; l < blockSize; l++){
                        //percorre todos os blocos diretos
                        if(inodePai.IS_DIR == 0x01 && i != numPais)
                            searchBlock(inodeBlock, file, inodeIBlock[l], blockSize, posInicialBlock);
                        /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                        com o mesmo tamanho do bloco em inodeBlock
                        */
                        for(int m = 0; m < blockSize; m++){
                            //percorre todo o o conteudo de inodeBlock
                            inodePai = searchInode(file, inodeBlock[m], posInicialInode);
                            //acha o inode apontado em cada valor de inodeBlock
                            if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1){
                                /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                                quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                                inodePai atual
                                */
                                posInodePai = inodeBlock[m];
                                break;
                            }
                        }
                        if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1)
                            break;
                    }
                }
            } else{
                for(int k = 0; k < 3; k++){
                    //percorre todos os blocos diretos
                    if(inodePai.IS_DIR == 0x01 && i != numPais)
                        searchBlock(inodeBlock, file, inodePai.DIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                     com o mesmo tamanho do bloco em inodeBlock
                    */
                    for(int l = 0; l < blockSize; l++){
                        //percorre todo o o conteudo de inodeBlock
                        inodePai = searchInode(file, inodeBlock[l], posInicialInode);
                        //acha o inode apontado em cada valor de inodeBlock
                        if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1){
                            /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                              quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                              inodePai atual
                            */
                            posInodePai = inodeBlock[l];
                            break;
                        }
                    }
                    if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1)
                        break;
                }
            }
            if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1)
                break;
        }
    }
    

    

    INODE inodeNovo{};
    inodeNovo.IS_DIR = 0x00;
    inodeNovo.IS_USED = 0x01;
    fileName.copy(inodeNovo.NAME, fileName.length()+1);
    inodeNovo.SIZE = fileContent.size();

    fseek(file, posInicialInode, SEEK_SET);
    for(int i = 0; i < numInodes; i++){
        unsigned char achou;
        
        fread(&achou, sizeof(char), 1, file);
        if(achou == 0x00){
            posInode = i;
            break;
        }else{
            fseek(file, sizeof(INODE) - sizeof(char), SEEK_CUR);
        }
    }
    int numBlocosOcupados = ceil(fileContent.size() / (double)blockSize);
    unsigned int blocoLivre[numBlocosOcupados];
    char fileChar[fileContent.length()];
    fileContent.copy(fileChar, fileContent.length()+1);
    
    for(int i = 0; i < numBlocosOcupados; i++){
        blocoLivre[i] = searchFreeBlock(mapa, tamanhoMapa);
    }

    for(int i = 0; i < numBlocosOcupados; i++){

        if((ceil(inodeNovo.SIZE / (double)blockSize)) > 6){
            for(int j = 0; j < 3; j++){
                if(inodeNovo.DOUBLE_INDIRECT_BLOCKS[j] == 0x00){
                    inodeNovo.DOUBLE_INDIRECT_BLOCKS[j] = blocoLivre[i];
                    break;
                }
            }
        } else if((ceil(inodeNovo.SIZE / (double)blockSize)) > 3){
            for(int j = 0; j < 3; j++){
                if(inodeNovo.INDIRECT_BLOCKS[j] == 0x00){
                    inodeNovo.INDIRECT_BLOCKS[j] = blocoLivre[i];
                    break;
                }
            }
        } else {
            for(int j = 0; j < 3; j++){
                if (inodeNovo.DIRECT_BLOCKS[j] == 0x00){
                    inodeNovo.DIRECT_BLOCKS[j] = blocoLivre[i];
                    break;
                }
            }
        }
    }
    cont = 0;
    for(int i = 0; i < numBlocosOcupados; i++){
        fseek(file, posInicialBlock, SEEK_SET);
        fseek(file, blocoLivre[i] * blockSize, SEEK_CUR);
        for(int j = 0; j < blockSize; j++){
            if(cont < fileContent.size()){
                fwrite(&fileChar[cont], sizeof(char), 1, file);
            }
            cont++;
        }
    }

    int blocosOcupadosPai = ceil(inodePai.SIZE / (double)blockSize);
    
    if(blocosOcupadosPai > 12){
        if(blocosOcupadosPai == 12)
            searchBlock(inodeDIBlock, file, inodePai.DOUBLE_INDIRECT_BLOCKS[0], blockSize, posInicialBlock);
        else
            searchBlock(inodeDIBlock, file, inodePai.DOUBLE_INDIRECT_BLOCKS[blocosOcupadosPai - 12], blockSize, posInicialBlock);
        bool achouBloco{false};
        for(int i = 0; i < blockSize; i++){
            searchBlock(inodeIBlock, file, *inodeDIBlock, blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                searchBlock(inodeBlock, file, *inodeIBlock, blockSize, posInicialBlock);
                for(int k = 0; k < blockSize; k++){
                    if(inodeBlock[k] == 0x00){
                        inodeBlock[k] = posInode;
                        achouBloco = true;
                        fseek(file, posInicialBlock, SEEK_SET);
                        fseek(file, inodePai.DOUBLE_INDIRECT_BLOCKS[blocosOcupadosPai - 13] * blockSize, SEEK_CUR);
                        fwrite(inodeDIBlock, sizeof(char) * blockSize, 1, file);
                        fseek(file, posInicialBlock, SEEK_SET);
                        fseek(file, *inodeDIBlock * blockSize, SEEK_CUR);
                        fwrite(inodeIBlock, sizeof(char) * blockSize, 1, file);
                        fseek(file, posInicialBlock, SEEK_SET);
                        fseek(file, *inodeIBlock * blockSize, SEEK_CUR);
                        fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                        inodePai.SIZE++;
                        break;
                    }
                }
            }
        }
        if(!achouBloco){
            inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 3] = searchFreeBlock(mapa, tamanhoMapa);
            searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 1], blockSize, posInicialBlock);
            searchBlock(inodeBlock, file, *inodeIBlock, blockSize, posInicialBlock);
            inodeIBlock[0] = *inodeBlock;
            inodeBlock[0] = posInode;
            fseek(file, posInicialBlock, SEEK_SET);
            fseek(file, inodePai.DOUBLE_INDIRECT_BLOCKS[blocosOcupadosPai - 12] * blockSize, SEEK_CUR);
            fwrite(inodeDIBlock, sizeof(char) * blockSize, 1, file);
            fseek(file, posInicialBlock, SEEK_SET);
            fseek(file, *inodeDIBlock * blockSize, SEEK_CUR);
            fwrite(inodeIBlock, sizeof(char) * blockSize, 1, file);
            fseek(file, posInicialBlock, SEEK_SET);
            fseek(file, *inodeIBlock * blockSize, SEEK_CUR);
            fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
            inodePai.SIZE++;
        }
    } else if (blocosOcupadosPai > 3){
        if(blocosOcupadosPai == 4)
            searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[0], blockSize, posInicialBlock);
        else
            searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 4], blockSize, posInicialBlock);
        bool achouBloco{false};
        for(int i = 0; i < blockSize; i++){
            searchBlock(inodeBlock, file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 1], blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                if(inodeBlock[j] == 0x00){
                    inodeBlock[j] = posInode;
                    achouBloco = true;
                    fseek(file, posInicialBlock, SEEK_SET);
                    fseek(file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 4] * blockSize, SEEK_CUR);
                    fwrite(inodeIBlock, sizeof(char) * blockSize, 1, file);
                    fseek(file, posInicialBlock, SEEK_SET);
                    fseek(file, *inodeIBlock * blockSize, SEEK_CUR);
                    fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                    inodePai.SIZE++;
                    break;
                }
            }
        }
        if(!achouBloco){
            if(blocosOcupadosPai < 12){
                inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 3] = searchFreeBlock(mapa, tamanhoMapa);
                searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 4], blockSize, posInicialBlock);
                searchBlock(inodeBlock, file, *inodeIBlock, blockSize, posInicialBlock);
                inodeIBlock[0] = *inodeBlock;
                inodeBlock[0] = posInode;
                fseek(file, posInicialBlock, SEEK_SET);
                fseek(file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 3] * blockSize, SEEK_CUR);
                fwrite(inodeIBlock, sizeof(char) * blockSize, 1, file);
                fseek(file, posInicialBlock, SEEK_SET);
                fseek(file, *inodeIBlock * blockSize, SEEK_CUR);
                fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                inodePai.SIZE++;
            }
        }
    } else {
        int tmpBloco{0x00};
        if(blocosOcupadosPai == 0){
            tmpBloco = inodePai.DIRECT_BLOCKS[0];
            searchBlock(inodeBlock, file, inodePai.DIRECT_BLOCKS[0], blockSize, posInicialBlock);
        } else {
            tmpBloco = inodePai.DIRECT_BLOCKS[blocosOcupadosPai - 1];
            searchBlock(inodeBlock, file, inodePai.DIRECT_BLOCKS[blocosOcupadosPai - 1], blockSize, posInicialBlock);
        }
        if(inodePai.SIZE % blockSize != 0 || (inodePai.IS_DIR == 0x01 && inodePai.SIZE == 0)){
            
            inodeBlock[inodePai.SIZE - blocosOcupadosPai] = posInode;
            fseek(file, posInicialBlock, SEEK_SET);
            fseek(file, tmpBloco * blockSize, SEEK_CUR);
            fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
            inodePai.SIZE++;
        } else {
            if(blocosOcupadosPai < 3){
                inodePai.DIRECT_BLOCKS[blocosOcupadosPai] = searchFreeBlock(mapa, tamanhoMapa);
                
                searchBlock(inodeBlock, file, inodePai.DIRECT_BLOCKS[blocosOcupadosPai], blockSize, posInicialBlock);
                for(int i = 0; i < blockSize; i++)
                    inodeBlock[i] = 0;
                inodeBlock[0] = posInode;
                fseek(file, posInicialBlock, SEEK_SET);
                fseek(file, inodePai.DIRECT_BLOCKS[blocosOcupadosPai] * blockSize, SEEK_CUR);
                fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                inodePai.SIZE++;
            }
        }
    }

    
    
    cont = 0;
    rewind(file);
    fwrite(&blockSize,sizeof(char), 1, file);
    fwrite(&numBlocks,sizeof(char), 1, file);
    fwrite(&numInodes,sizeof(char), 1, file);
    fwrite(&mapa,sizeof(char) * tamanhoMapa, 1, file);
    fseek(file, posInicialInode, SEEK_SET);
    fseek(file, sizeof(INODE) * posInodePai, SEEK_CUR);
    fwrite(&inodePai, sizeof(INODE), 1, file);
    fseek(file, posInicialInode, SEEK_SET);
    fseek(file, sizeof(INODE) * posInode, SEEK_CUR);
    fwrite(&inodeNovo, sizeof(INODE), 1, file);
    
    fclose(file);
    delete inodeBlock;
    delete inodeIBlock;
    delete inodeDIBlock;
}

/**
 * @brief Adiciona um novo diretório dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param dirPath caminho completo novo diretório dentro sistema de arquivos que simula EXT3.
 */
void addDir(std::string fsFileName, std::string dirPath){
    if((dirPath[0] != '/') || (dirPath[dirPath.size() - 1] == '/')) std::cout << "endereco invalido";
    FILE* file = fopen(fsFileName.c_str(), "rw+");
    rewind(file);

    int blockSize{};
    int numBlocks{0x00};
    int numInodes{0x00};
    int mapa{0x00};
    std::string dirName;

    fread(&blockSize, sizeof(char), 1, file);
    fread(&numBlocks, sizeof(char), 1, file);
    fread(&numInodes, sizeof(char), 1, file);

    unsigned int tamanhoMapa = ceil(numBlocks / 8.0);
    fread(&mapa, tamanhoMapa, 1, file);

    int posInicialInode = sizeof(char) * 3 + tamanhoMapa;
    int posInicialBlock = posInicialInode + sizeof(INODE) * numInodes + 1;

    unsigned int posInode{0x00};
    unsigned int numPais = 1;

    for(int i = 1; i < dirPath.size(); i++){
        if(dirPath[i] == '/'){
            numPais++;
        }
    }
    std::string nomesPais[numPais];
    nomesPais[0] = '/';
    std::string buffer;
    int cont = 1;

    for(int i = 1; i < dirPath.size(); i++){
        if(dirPath[i] != '/'){
            buffer = buffer + dirPath[i];
        } else{
            nomesPais[cont] = buffer;
            buffer = "";
            cont++;
        }
        if(i == dirPath.size() - 1){
            dirName = buffer;
        }
    }

    char* inodeDIBlock = new char(sizeof(char)* blockSize);
    char* inodeIBlock = new char(sizeof(char)* blockSize);
    char* inodeBlock = new char(sizeof(char)* blockSize);
    int posInodePai{0x00};
    INODE inodePai{};
    fread(&inodePai.IS_USED, sizeof(char), 1, file);
    fread(&inodePai.IS_DIR, sizeof(char), 1, file);
    fread(&inodePai.NAME, sizeof(char) * 10, 1, file);
    fread(&inodePai.SIZE, sizeof(char), 1, file);
    fread(&inodePai.DIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    fread(&inodePai.INDIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    fread(&inodePai.DOUBLE_INDIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    
    for(int i = 1; i < numPais; i++){
        //percorre cada um dos inodes pais para encontrar o inode que deve ser atualizado
        for(int j = 0; j < ceil(inodePai.SIZE / (double)blockSize) ; j++){
            //percorre cada um dos blocos do inode atual do loop (inodePai) para achar os inodes filhos
            if(j > 12){
                //verifica se o inodePai tem tamanho o suficiente para ter que ocupar blocos indiretos duplos
                for(int k = 0; k < 3; k++){
                    if(inodePai.IS_DIR == 0x01 && i != numPais)
                        searchBlock(inodeDIBlock, file, inodePai.INDIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    for(int l = 0; l < blockSize ; l++){
                        if(inodePai.IS_DIR == 0x01 && i != numPais)
                            searchBlock(inodeIBlock, file, inodeDIBlock[l], blockSize, posInicialBlock);
                        for(int m = 0; m < blockSize; m++){
                            //percorre todos os blocos diretos
                            if(inodePai.IS_DIR == 0x01 && i != numPais)
                                searchBlock(inodeBlock, file, inodeIBlock[m], blockSize, posInicialBlock);
                            /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                            com o mesmo tamanho do bloco em inodeBlock
                            */
                            for(int n = 0; n < blockSize; n++){
                                //percorre todo o o conteudo de inodeBlock
                                inodePai = searchInode(file, inodeBlock[n], posInicialInode);
                                //acha o inode apontado em cada valor de inodeBlock
                                if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1){
                                    /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                                    quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                                    inodePai atual
                                    */
                                    posInodePai = inodeBlock[n];
                                    break;
                                }
                            }
                            if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1)
                                break;
                        }
                    }
                }
            } else if(j > 3){
                //verifica se o inodePai tem tamanho o suficiente para ter que ocupar blocos indiretos
                for(int k = 0; k < 3 ; k++){
                    if(inodePai.IS_DIR == 0x01 && i != numPais)
                        searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    for(int l = 0; l < blockSize; l++){
                        //percorre todos os blocos diretos
                        if(inodePai.IS_DIR == 0x01 && i != numPais)
                            searchBlock(inodeBlock, file, inodeIBlock[l], blockSize, posInicialBlock);
                        /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                        com o mesmo tamanho do bloco em inodeBlock
                        */
                        for(int m = 0; m < blockSize; m++){
                            //percorre todo o o conteudo de inodeBlock
                            inodePai = searchInode(file, inodeBlock[m], posInicialInode);
                            //acha o inode apontado em cada valor de inodeBlock
                            if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1){
                                /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                                quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                                inodePai atual
                                */
                                posInodePai = inodeBlock[m];
                                break;
                            }
                        }
                        if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1)
                            break;
                    }
                }
            } else{
                for(int k = 0; k < 3; k++){
                    //percorre todos os blocos diretos
                    if(inodePai.IS_DIR == 0x01 && i != numPais)
                        searchBlock(inodeBlock, file, inodePai.DIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                     com o mesmo tamanho do bloco em inodeBlock
                    */
                    for(int l = 0; l < blockSize; l++){
                        //percorre todo o o conteudo de inodeBlock
                        inodePai = searchInode(file, inodeBlock[l], posInicialInode);
                        //acha o inode apontado em cada valor de inodeBlock
                        if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1){
                            /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                              quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                              inodePai atual
                            */
                            posInodePai = inodeBlock[l];
                            break;
                        }
                    }
                    if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1)
                        break;
                }
            }
            if(inodePai.NAME == nomesPais[i] && inodePai.IS_DIR == 1)
                break;
        }
    }
    

    

    INODE inodeNovo{};
    inodeNovo.IS_DIR = 0x01;
    inodeNovo.IS_USED = 0x01;
    dirName.copy(inodeNovo.NAME, dirName.length()+1);
    inodeNovo.SIZE = 0;

    fseek(file, posInicialInode, SEEK_SET);
    for(int i = 0; i < numInodes; i++){
        unsigned char achou;
        
        fread(&achou, sizeof(char), 1, file);
        if(achou == 0x00){
            posInode = i;
            break;
        }else{
            fseek(file, sizeof(INODE) - sizeof(char), SEEK_CUR);
        }
    }

    int blocosOcupadosPai = ceil(inodePai.SIZE / (double)blockSize);
    
    if(blocosOcupadosPai > 12){
        if(blocosOcupadosPai == 13)
            searchBlock(inodeDIBlock, file, inodePai.DOUBLE_INDIRECT_BLOCKS[0], blockSize, posInicialBlock);
        else
            searchBlock(inodeDIBlock, file, inodePai.DOUBLE_INDIRECT_BLOCKS[blocosOcupadosPai - 12], blockSize, posInicialBlock);
        bool achouBloco{false};
        for(int i = 0; i < blockSize; i++){
            searchBlock(inodeIBlock, file, *inodeDIBlock, blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                searchBlock(inodeBlock, file, *inodeIBlock, blockSize, posInicialBlock);
                for(int k = 0; k < blockSize; k++){
                    if(inodeBlock[k] == 0x00){
                        inodeBlock[k] = posInode;
                        achouBloco = true;
                        fseek(file, posInicialBlock, SEEK_SET);
                        fseek(file, inodePai.DOUBLE_INDIRECT_BLOCKS[blocosOcupadosPai - 13] * blockSize, SEEK_CUR);
                        fwrite(inodeDIBlock, sizeof(char) * blockSize, 1, file);
                        fseek(file, posInicialBlock, SEEK_SET);
                        fseek(file, *inodeDIBlock * blockSize, SEEK_CUR);
                        fwrite(inodeIBlock, sizeof(char) * blockSize, 1, file);
                        fseek(file, posInicialBlock, SEEK_SET);
                        fseek(file, *inodeIBlock * blockSize, SEEK_CUR);
                        fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                        inodePai.SIZE++;
                        break;
                    }
                }
            }
        }
        if(!achouBloco){
            inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 3] = searchFreeBlock(mapa, tamanhoMapa);
            searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 1], blockSize, posInicialBlock);
            searchBlock(inodeBlock, file, *inodeIBlock, blockSize, posInicialBlock);
            inodeIBlock[0] = *inodeBlock;
            inodeBlock[0] = posInode;
            fseek(file, posInicialBlock, SEEK_SET);
            fseek(file, inodePai.DOUBLE_INDIRECT_BLOCKS[blocosOcupadosPai - 12] * blockSize, SEEK_CUR);
            fwrite(inodeDIBlock, sizeof(char) * blockSize, 1, file);
            fseek(file, posInicialBlock, SEEK_SET);
            fseek(file, *inodeDIBlock * blockSize, SEEK_CUR);
            fwrite(inodeIBlock, sizeof(char) * blockSize, 1, file);
            fseek(file, posInicialBlock, SEEK_SET);
            fseek(file, *inodeIBlock * blockSize, SEEK_CUR);
            fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
            inodePai.SIZE++;
        }
    } else if (blocosOcupadosPai > 3){
        if(blocosOcupadosPai == 4)
            searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[0], blockSize, posInicialBlock);
        else
            searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 4], blockSize, posInicialBlock);
        bool achouBloco{false};
        for(int i = 0; i < blockSize; i++){
            searchBlock(inodeBlock, file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 1], blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                if(inodeBlock[j] == 0x00){
                    inodeBlock[j] = posInode;
                    achouBloco = true;
                    fseek(file, posInicialBlock, SEEK_SET);
                    fseek(file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 4] * blockSize, SEEK_CUR);
                    fwrite(inodeIBlock, sizeof(char) * blockSize, 1, file);
                    fseek(file, posInicialBlock, SEEK_SET);
                    fseek(file, *inodeIBlock * blockSize, SEEK_CUR);
                    fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                    inodePai.SIZE++;
                    break;
                }
            }
        }
        if(!achouBloco){
            if(blocosOcupadosPai < 12){
                inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 3] = searchFreeBlock(mapa, tamanhoMapa);
                searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 4], blockSize, posInicialBlock);
                searchBlock(inodeBlock, file, *inodeIBlock, blockSize, posInicialBlock);
                inodeIBlock[0] = *inodeBlock;
                inodeBlock[0] = posInode;
                fseek(file, posInicialBlock, SEEK_SET);
                fseek(file, inodePai.INDIRECT_BLOCKS[blocosOcupadosPai - 3] * blockSize, SEEK_CUR);
                fwrite(inodeIBlock, sizeof(char) * blockSize, 1, file);
                fseek(file, posInicialBlock, SEEK_SET);
                fseek(file, *inodeIBlock * blockSize, SEEK_CUR);
                fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                inodePai.SIZE++;
            }
        }
    } else {
        if(blocosOcupadosPai == 0)
            searchBlock(inodeBlock, file, inodePai.DIRECT_BLOCKS[0], blockSize, posInicialBlock);
        else
            searchBlock(inodeBlock, file, inodePai.DIRECT_BLOCKS[blocosOcupadosPai - 1], blockSize, posInicialBlock);
        bool achouBloco{false};
        for(int i = 0; i < blockSize; i++){
            if(inodeBlock[i] == 0x00){
                inodeBlock[i] = posInode;
                fseek(file, posInicialBlock, SEEK_SET);
                fseek(file, inodePai.DIRECT_BLOCKS[blocosOcupadosPai - 1] * blockSize, SEEK_CUR);
                fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                achouBloco = true;
                inodePai.SIZE++;
                break;
            }

        }
        if(!achouBloco){
            if(blocosOcupadosPai < 3){
                inodePai.DIRECT_BLOCKS[blocosOcupadosPai] = searchFreeBlock(mapa, tamanhoMapa);
                searchBlock(inodeBlock, file, inodePai.DIRECT_BLOCKS[blocosOcupadosPai - 1], blockSize, posInicialBlock);
                
                inodeBlock[0] = posInode;
                fseek(file, posInicialBlock, SEEK_SET);
                fseek(file, inodePai.DIRECT_BLOCKS[blocosOcupadosPai] * blockSize, SEEK_CUR);
                fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                inodePai.SIZE++;
            }
        }
    }

    inodeNovo.DIRECT_BLOCKS[0] = searchFreeBlock(mapa, tamanhoMapa);
    rewind(file);
    fwrite(&blockSize,sizeof(char), 1, file);
    fwrite(&numBlocks,sizeof(char), 1, file);
    fwrite(&numInodes,sizeof(char), 1, file);
    fwrite(&mapa,sizeof(char) * tamanhoMapa, 1, file);
    fseek(file, posInicialInode, SEEK_SET);
    fseek(file, sizeof(INODE) * posInodePai, SEEK_CUR);
    fwrite(&inodePai, sizeof(INODE), 1, file);
    fseek(file, posInicialInode, SEEK_SET);
    fseek(file, sizeof(INODE) * posInode, SEEK_CUR);
    fwrite(&inodeNovo, sizeof(INODE), 1, file);
    fseek(file, posInicialBlock, SEEK_SET);
    fseek(file, inodeNovo.DIRECT_BLOCKS[0] * blockSize, SEEK_CUR);
    char zero{0x00};
    for(int i = 0; i < blockSize; i++){
        fwrite(&zero, sizeof(char), 1, file);
    }
    fclose(file);
    delete inodeBlock;
    delete inodeIBlock;
    delete inodeDIBlock;
}

/**
 * @brief Remove um arquivo ou diretório (recursivamente) de um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param path caminho completo do arquivo ou diretório a ser removido.
 */
void remove(std::string fsFileName, std::string path){
    if((path[0] != '/') || (path[path.size() - 1] == '/')) std::cout << "endereco invalido";
    FILE* file = fopen(fsFileName.c_str(), "rw+");
    rewind(file);

    int blockSize{};
    int numBlocks{0x00};
    int numInodes{0x00};
    int mapa{0x00};
    std::string fileName;

    fread(&blockSize, sizeof(char), 1, file);
    fread(&numBlocks, sizeof(char), 1, file);
    fread(&numInodes, sizeof(char), 1, file);

    unsigned int tamanhoMapa = ceil(numBlocks / 8.0);
    fread(&mapa, tamanhoMapa, 1, file);

    int posInicialInode = sizeof(char) * 3 + tamanhoMapa;
    int posInicialBlock = posInicialInode + sizeof(INODE) * numInodes + 1;

    unsigned int numPais = 1;

    for(int i = 1; i < path.size(); i++){
        if(path[i] == '/'){
            numPais++;
        }
    }
    std::string nomesPais[numPais];
    nomesPais[0] = '/';
    std::string buffer;
    int cont = 1;

    for(int i = 1; i < path.size(); i++){
        if(path[i] != '/'){
            buffer = buffer + path[i];
        } else{
            nomesPais[cont] = buffer;
            buffer = "";
            cont++;
        }
        if(i == path.size() - 1){
            fileName = buffer;
        }
    }

    char* inodeDIBlock = new char(sizeof(char)* blockSize);
    char* inodeIBlock = new char(sizeof(char)* blockSize);
    char* inodeBlock = new char(sizeof(char)* blockSize);
    int posInodePai{0x00};
    int inodeExcluidoAdress{0x00};
    INODE inodeExcluido{};
    INODE inodePai{0x00};
    fread(&inodeExcluido.IS_USED, sizeof(char), 1, file);
    fread(&inodeExcluido.IS_DIR, sizeof(char), 1, file);
    fread(&inodeExcluido.NAME, sizeof(char) * 10, 1, file);
    fread(&inodeExcluido.SIZE, sizeof(char), 1, file);
    fread(&inodeExcluido.DIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    fread(&inodeExcluido.INDIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    fread(&inodeExcluido.DOUBLE_INDIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    //fileName.copy(inodeExcluido.NAME, fileName.length()+1);

    for(int i = 1; i < numPais + 1; i++){
        //percorre cada um dos inodes pais para encontrar o inode que deve ser atualizado
        for(int j = 0; j < ceil(inodeExcluido.SIZE / (double)blockSize) ; j++){
            //percorre cada um dos blocos do inode atual do loop (inodeExcluido) para achar os inodes filhos
            if(j > 12){
                //verifica se o inodeExcluido tem tamanho o suficiente para ter que ocupar blocos indiretos duplos
                for(int k = 0; k < 3; k++){
                    if(inodeExcluido.IS_DIR == 0x01)
                        searchBlock(inodeDIBlock, file, inodeExcluido.INDIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    for(int l = 0; l < blockSize ; l++){
                        if(inodeExcluido.IS_DIR == 0x01)
                            searchBlock(inodeIBlock, file, *inodeDIBlock, blockSize, posInicialBlock);
                        for(int m = 0; m < blockSize; m++){
                            //percorre todos os blocos diretos
                            if(inodeExcluido.IS_DIR == 0x01)
                                searchBlock(inodeBlock, file, *inodeIBlock, blockSize, posInicialBlock);
                            /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                            com o mesmo tamanho do bloco em inodeBlock
                            */
                           inodePai = inodeExcluido;
                            for(int n = 0; n < blockSize; n++){
                                //percorre todo o o conteudo de inodeBlock
                                inodeExcluido = searchInode(file, inodeBlock[n], posInicialInode);
                                //acha o inode apontado em cada valor de inodeBlock
                                if(i == numPais && inodeExcluido.NAME == inodeExcluido.NAME){
                                    //Após o loop percorrer todos os pais do inodeExcluído, ele acha o próprio inodeExcluido e o seu edndereço
                                    inodeExcluidoAdress = inodeBlock[n];
                                    break;
                                } else if(inodeExcluido.NAME == nomesPais[i] && inodeExcluido.IS_DIR == 1){
                                    /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                                    quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                                    inodeExcluido atual
                                    */
                                    posInodePai = inodeBlock[n];
                                    break;
                                }
                            }
                            if((inodeExcluido.NAME == nomesPais[i] && inodeExcluido.IS_DIR == 1) || (i == numPais && strcmp(inodeExcluido.NAME, inodeExcluido.NAME) == 0))
                                break;
                        }
                    }
                }
            } else if(j > 3){
                //verifica se o inodeExcluido tem tamanho o suficiente para ter que ocupar blocos indiretos
                for(int k = 0; k < 3 ; k++){
                    if(inodeExcluido.IS_DIR == 0x01)
                        searchBlock(inodeIBlock, file, inodeExcluido.INDIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    for(int l = 0; l < blockSize; l++){
                        //percorre todos os blocos diretos
                        if(inodeExcluido.IS_DIR == 0x01)
                            searchBlock(inodeBlock, file, *inodeIBlock, blockSize, posInicialBlock);
                        /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                        com o mesmo tamanho do bloco em inodeBlock
                        */
                        inodePai = inodeExcluido;
                        for(int m = 0; m < blockSize; m++){
                            //percorre todo o o conteudo de inodeBlock
                            inodeExcluido = searchInode(file, inodeBlock[m], posInicialInode);
                            //acha o inode apontado em cada valor de inodeBlock
                            if(i == numPais && inodeExcluido.NAME == inodeExcluido.NAME){
                                //Após o loop percorrer todos os pais do inodeExcluído, ele acha o próprio inodeExcluido e o seu edndereço
                                inodeExcluidoAdress = inodeBlock[m];
                                break;
                            } else if(inodeExcluido.NAME == nomesPais[i] && inodeExcluido.IS_DIR == 1){
                                /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                                quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                                inodeExcluido atual
                                */
                                posInodePai = inodeBlock[m];
                                break;
                            }
                        }
                        if((inodeExcluido.NAME == nomesPais[i] && inodeExcluido.IS_DIR == 1) || (i == numPais && strcmp(inodeExcluido.NAME, inodeExcluido.NAME) == 0))
                            break;
                    }
                }
            } else{
                for(int k = 0; k < 3; k++){
                    //percorre todos os blocos diretos
                    if(inodeExcluido.IS_DIR == 0x01)
                        searchBlock(inodeBlock, file, inodeExcluido.DIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                     com o mesmo tamanho do bloco em inodeBlock
                    */
                    inodePai = inodeExcluido;
                    for(int l = 0; l < blockSize; l++){
                        //percorre todo o o conteudo de inodeBlock
                        inodeExcluido = searchInode(file, inodeBlock[l], posInicialInode);
                        //acha o inode apontado em cada valor de inodeBlock
                        if(i == numPais && strcmp(inodeExcluido.NAME, fileName.c_str()) == 0){
                            //Após o loop percorrer todos os pais do inodeExcluído, ele acha o próprio inodeExcluido e o seu endereço
                            inodeExcluidoAdress = inodeBlock[l];
                            break;
                        } else if(inodeExcluido.NAME == nomesPais[i] && inodeExcluido.IS_DIR == 1){
                            /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                              quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                              inodeExcluido atual
                            */
                            posInodePai = inodeBlock[l];
                            break;
                        }
                    }
                    if((inodeExcluido.NAME == nomesPais[i] && inodeExcluido.IS_DIR == 1) || (i == numPais && strcmp(inodeExcluido.NAME, inodeExcluido.NAME) == 0))
                        break;
                }
            }
            if((inodeExcluido.NAME == nomesPais[i] && inodeExcluido.IS_DIR == 1) || (i == numPais && strcmp(inodeExcluido.NAME, inodeExcluido.NAME) == 0))
                break;
        }
    }

    int blocosOcupadosPai = ceil(inodePai.SIZE / (double)blockSize);
    bool achouExcluido{false};
    for(int i = 0; i < blocosOcupadosPai; i++){
        if(achouExcluido)
            break;
        if (i < 3){
            searchBlock(inodeBlock, file, inodePai.DIRECT_BLOCKS[i], blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                if(inodeBlock[j] == inodeExcluidoAdress){
                    fseek(file, posInicialBlock, SEEK_SET);
                    fseek(file, inodePai.DIRECT_BLOCKS[i] * blockSize, SEEK_CUR);
                    fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                    inodePai.SIZE--;
                    if(inodePai.SIZE != 0)
                        copiarDireitaEsquerda(file, inodeExcluidoAdress, inodePai, i, blockSize, posInicialBlock, mapa);
                    achouExcluido = true;
                    break;
                }
            }
        } else if(i < 6){
            searchBlock(inodeIBlock, file, inodePai.INDIRECT_BLOCKS[i - 3], blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                searchBlock(inodeBlock, file, inodeIBlock[j], blockSize, posInicialBlock);
                for(int k = 0; k < blockSize; k++){
                    if(inodeBlock[k] == inodeExcluidoAdress){
                        fseek(file, posInicialBlock, SEEK_SET);
                        fseek(file, inodePai.DIRECT_BLOCKS[i] * blockSize, SEEK_CUR);
                        fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                        inodePai.SIZE--;
                        if(inodePai.SIZE != 0)
                            copiarDireitaEsquerda(file, inodeExcluidoAdress, inodePai, i, blockSize, posInicialBlock, mapa);
                        achouExcluido = true;
                        break;
                    }
                }
            }
        } else if(i < 9){
            searchBlock(inodeDIBlock, file, inodePai.DOUBLE_INDIRECT_BLOCKS[i - 6], blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                searchBlock(inodeIBlock, file, inodeDIBlock[j], blockSize, posInicialBlock);
                for(int k = 0; k < blockSize; k++){
                    searchBlock(inodeBlock, file, inodeIBlock[k], blockSize, posInicialBlock);
                    for(int l = 0; k < blockSize; l++){
                        if(inodeBlock[l] == inodeExcluidoAdress){
                            fseek(file, posInicialBlock, SEEK_SET);
                            fseek(file, inodePai.DIRECT_BLOCKS[i] * blockSize, SEEK_CUR);
                            fwrite(inodeBlock, sizeof(char) * blockSize, 1, file);
                            inodePai.SIZE--;
                            if(inodePai.SIZE != 0)
                                copiarDireitaEsquerda(file, inodeExcluidoAdress, inodePai, i, blockSize, posInicialBlock, mapa);
                            achouExcluido = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    if(inodeExcluido.IS_DIR == 0x01 && inodeExcluido.SIZE == 0){
        searchBlock(inodeBlock, file, inodeExcluido.DIRECT_BLOCKS[0], blockSize, posInicialBlock); 
        mapa &= ~(1 << inodeBlock[0]);
    }
    for(int i = 0; i < ceil(inodeExcluido.SIZE / (double) blockSize) ; i++){
        // Marcar os blocos do inodeExcluido como livres no mapa de bits
        if(blocosOcupadosPai < 3){
            mapa &= ~(1 << inodeExcluido.DIRECT_BLOCKS[i]);
        } else if(blocosOcupadosPai < 12){
            searchBlock(inodeIBlock, file, inodeExcluido.INDIRECT_BLOCKS[i], blockSize, posInicialBlock); 
            for(int j = 0; j < blockSize; j++){
                if(inodeBlock[j] != 0)
                    mapa &= ~(1 << inodeIBlock[j]);
            }
            mapa &= ~(1 << inodeExcluido.INDIRECT_BLOCKS[i]);
        } else{
            searchBlock(inodeDIBlock, file, inodeExcluido.DOUBLE_INDIRECT_BLOCKS[i], blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                
                searchBlock(inodeIBlock, file, inodeDIBlock[j], blockSize, posInicialBlock);
                for(int k = 0; k < blockSize; k++){
                    if(inodeBlock[k] != 0)
                        mapa &= ~(1 << inodeIBlock[k]);
                }    
                if(inodeBlock[j] != 0)
                    mapa &= ~(1 << inodeDIBlock[j]);
            }
            if(inodeBlock[i] != 0)
                mapa &= ~(1 <<inodeExcluido.DOUBLE_INDIRECT_BLOCKS[i]);
        }
    }
    inodeExcluido.IS_USED = 0x00;
    inodeExcluido.IS_DIR = 0x00;
    for(int i = 0; i < 10; i++)
        inodeExcluido.NAME[i] = 0;
    inodeExcluido.SIZE = 0x00;
    for(int i = 0; i < 3; i++){
        inodeExcluido.DIRECT_BLOCKS[i] = 0;
        inodeExcluido.INDIRECT_BLOCKS[i] = 0;
        inodeExcluido.DOUBLE_INDIRECT_BLOCKS[i] = 0;;
    }
    
    rewind(file);
    fwrite(&blockSize,sizeof(char), 1, file);
    fwrite(&numBlocks,sizeof(char), 1, file);
    fwrite(&numInodes,sizeof(char), 1, file);
    fwrite(&mapa,sizeof(char) * tamanhoMapa, 1, file);
    fseek(file, posInicialInode, SEEK_SET);
    fseek(file, sizeof(INODE) * posInodePai, SEEK_CUR);
    fwrite(&inodePai, sizeof(INODE), 1, file);
    fseek(file, posInicialInode, SEEK_SET);
    fseek(file, sizeof(INODE) * inodeExcluidoAdress, SEEK_CUR);
    fwrite(&inodeExcluido, sizeof(INODE), 1, file);
    fclose(file);
    delete inodeBlock;
    delete inodeIBlock;
    delete inodeDIBlock;
}

/**
 * @brief Move um arquivo ou diretório em um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param oldPath caminho completo do arquivo ou diretório a ser movido.
 * @param newPath novo caminho completo do arquivo ou diretório.
 */
void move(std::string fsFileName, std::string oldPath, std::string newPath){
    if((oldPath[0] != '/') || (oldPath[oldPath.size() - 1] == '/') || (newPath[0] != '/') || (newPath[newPath.size() - 1] == '/')) std::cout << "endereco invalido";
    FILE* file = fopen(fsFileName.c_str(), "rw+");
    rewind(file);

    int blockSize{};
    int numBlocks{0x00};
    int numInodes{0x00};
    int mapa{0x00};
    std::string fileNameOld;
    std::string fileNameNew;

    fread(&blockSize, sizeof(char), 1, file);
    fread(&numBlocks, sizeof(char), 1, file);
    fread(&numInodes, sizeof(char), 1, file);

    unsigned int tamanhoMapa = ceil(numBlocks / 8.0);
    fread(&mapa, tamanhoMapa, 1, file);

    int posInicialInode = sizeof(char) * 3 + tamanhoMapa;
    int posInicialBlock = posInicialInode + sizeof(INODE) * numInodes + 1;
    unsigned int posInode{0x00};
    unsigned int numPais = 1;

    for(int i = 1; i < oldPath.size(); i++){
        if(oldPath[i] == '/'){
            numPais++;
        }
    }

    std::string nomesPais[numPais];
    nomesPais[0] = '/';
    std::string buffer;

    for(int i = 1; i < oldPath.size(); i++){
        if(oldPath[i] != '/'){
            buffer = buffer + oldPath[i];
        } else{
            buffer = "";
        }
        if(i == oldPath.size() - 1){
            fileNameOld = buffer;
            buffer = "";
        }
    }
    for(int i = 1; i < newPath.size(); i++){
        if(newPath[i] != '/'){
            buffer = buffer + newPath[i];
        } else{
            buffer = "";
        }
        if(i == newPath.size() - 1){
            fileNameNew = buffer;
        }
    }
    
    char* inodeDIBlock = new char(sizeof(char)* blockSize);
    char* inodeIBlock = new char(sizeof(char)* blockSize);
    char* inodeBlock = new char(sizeof(char)* blockSize);
    int posInodePai{0x00};
    int inodeMovidoAdress{0x00};
    INODE inodeMovido{};
    INODE inodePai{0x00};
    fread(&inodeMovido.IS_USED, sizeof(char), 1, file);
    fread(&inodeMovido.IS_DIR, sizeof(char), 1, file);
    fread(&inodeMovido.NAME, sizeof(char) * 10, 1, file);
    fread(&inodeMovido.SIZE, sizeof(char), 1, file);
    fread(&inodeMovido.DIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    fread(&inodeMovido.INDIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    fread(&inodeMovido.DOUBLE_INDIRECT_BLOCKS, sizeof(char) * 3, 1, file);
    //fileName.copy(inodeMovido.NAME, fileName.length()+1);

    for(int i = 1; i < numPais + 1; i++){
        //percorre cada um dos inodes pais para encontrar o inode que deve ser atualizado
        for(int j = 0; j < ceil(inodeMovido.SIZE / (double)blockSize) ; j++){
            //percorre cada um dos blocos do inode atual do loop (inodeMovido) para achar os inodes filhos
            if(j > 12){
                //verifica se o inodeMovido tem tamanho o suficiente para ter que ocupar blocos indiretos duplos
                for(int k = 0; k < 3; k++){
                    if(inodeMovido.IS_DIR == 0x01)
                        searchBlock(inodeDIBlock, file, inodeMovido.INDIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    for(int l = 0; l < blockSize ; l++){
                        if(inodeMovido.IS_DIR == 0x01)
                            searchBlock(inodeIBlock, file, *inodeDIBlock, blockSize, posInicialBlock);
                        for(int m = 0; m < blockSize; m++){
                            //percorre todos os blocos diretos
                            if(inodeMovido.IS_DIR == 0x01)
                                searchBlock(inodeBlock, file, *inodeIBlock, blockSize, posInicialBlock);
                            /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                            com o mesmo tamanho do bloco em inodeBlock
                            */
                           inodePai = inodeMovido;
                            for(int n = 0; n < blockSize; n++){
                                //percorre todo o o conteudo de inodeBlock
                                inodeMovido = searchInode(file, inodeBlock[n], posInicialInode);
                                //acha o inode apontado em cada valor de inodeBlock
                                if(i == numPais && inodeMovido.NAME == inodeMovido.NAME){
                                    //Após o loop percorrer todos os pais do inodeExcluído, ele acha o próprio inodeMovido e o seu edndereço
                                    inodeMovidoAdress = inodeBlock[n];
                                    break;
                                } else if(inodeMovido.NAME == nomesPais[i] && inodeMovido.IS_DIR == 1){
                                    /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                                    quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                                    inodeMovido atual
                                    */
                                    posInodePai = inodeBlock[n];
                                    break;
                                }
                            }
                            if((inodeMovido.NAME == nomesPais[i] && inodeMovido.IS_DIR == 1) || (i == numPais && strcmp(inodeMovido.NAME, inodeMovido.NAME) == 0))
                                break;
                        }
                    }
                }
            } else if(j > 3){
                //verifica se o inodeMovido tem tamanho o suficiente para ter que ocupar blocos indiretos
                for(int k = 0; k < 3 ; k++){
                    if(inodeMovido.IS_DIR == 0x01)
                        searchBlock(inodeIBlock, file, inodeMovido.INDIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    for(int l = 0; l < blockSize; l++){
                        //percorre todos os blocos diretos
                        if(inodeMovido.IS_DIR == 0x01)
                            searchBlock(inodeBlock, file, *inodeIBlock, blockSize, posInicialBlock);
                        /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                        com o mesmo tamanho do bloco em inodeBlock
                        */
                        inodePai = inodeMovido;
                        for(int m = 0; m < blockSize; m++){
                            //percorre todo o o conteudo de inodeBlock
                            inodeMovido = searchInode(file, inodeBlock[m], posInicialInode);
                            //acha o inode apontado em cada valor de inodeBlock
                            if(i == numPais && inodeMovido.NAME == inodeMovido.NAME){
                                //Após o loop percorrer todos os pais do inodeExcluído, ele acha o próprio inodeMovido e o seu edndereço
                                inodeMovidoAdress = inodeBlock[m];
                                break;
                            } else if(inodeMovido.NAME == nomesPais[i] && inodeMovido.IS_DIR == 1){
                                /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                                quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                                inodeMovido atual
                                */
                                posInodePai = inodeBlock[m];
                                break;
                            }
                        }
                        if((inodeMovido.NAME == nomesPais[i] && inodeMovido.IS_DIR == 1) || (i == numPais && strcmp(inodeMovido.NAME, inodeMovido.NAME) == 0))
                            break;
                    }
                }
            } else{
                for(int k = 0; k < 3; k++){
                    //percorre todos os blocos diretos
                    if(inodeMovido.IS_DIR == 0x01)
                        searchBlock(inodeBlock, file, inodeMovido.DIRECT_BLOCKS[k], blockSize, posInicialBlock);
                    /* pega o endereço do block de direct_blocks e coloca o conteúdo, que será um array de chars
                     com o mesmo tamanho do bloco em inodeBlock
                    */
                    inodePai = inodeMovido;
                    for(int l = 0; l < blockSize; l++){
                        //percorre todo o o conteudo de inodeBlock
                        inodeMovido = searchInode(file, inodeBlock[l], posInicialInode);
                        //acha o inode apontado em cada valor de inodeBlock
                        if(i == numPais && strcmp(inodeMovido.NAME, fileNameOld.c_str()) == 0){
                            //Após o loop percorrer todos os pais do inodeExcluído, ele acha o próprio inodeMovido e o seu endereço
                            inodeMovidoAdress = inodeBlock[l];
                            break;
                        } else if(inodeMovido.NAME == nomesPais[i] && inodeMovido.IS_DIR == 1){
                            /* se o inode achado tiver o mesmo nome que o inode pai atual e ele for um diretório, 
                              quebra o loop recursivamente até voltar a fazer a busca novamente pelos filhos do 
                              inodeMovido atual
                            */
                            posInodePai = inodeBlock[l];
                            break;
                        }
                    }
                    if((inodeMovido.NAME == nomesPais[i] && inodeMovido.IS_DIR == 1) || (i == numPais && strcmp(inodeMovido.NAME, inodeMovido.NAME) == 0))
                        break;
                }
            }
            if((inodeMovido.NAME == nomesPais[i] && inodeMovido.IS_DIR == 1) || (i == numPais && strcmp(inodeMovido.NAME, inodeMovido.NAME) == 0))
                break;
        }
    }
    int blocosOcupados = ceil(inodeMovido.SIZE / (double) blockSize);
    std::string fileContent{};
    int cont = 0;
    for(int i = 0; i < blocosOcupados; i++){
        if(i < 3){
            searchBlock(inodeBlock, file, inodeMovido.DIRECT_BLOCKS[i], blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                fileContent += inodeBlock[j];
                cont++;
                if(cont == inodeMovido.SIZE)
                    break;
            }
        } else if(i < 12){
            int tmp = ceil(i / (double)blockSize) - 3;
            searchBlock(inodeIBlock, file, inodeMovido.INDIRECT_BLOCKS[tmp], blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                searchBlock(inodeBlock, file, inodeIBlock[j], blockSize, posInicialBlock);
                for(int k = 0; k < blockSize; k++){
                    fileContent += inodeBlock[k];
                    cont++;
                    if(cont == inodeMovido.SIZE)
                        break;
                }
            }
        } else{
            int tmp = ceil(i / (double)blockSize) - 12;
            searchBlock(inodeDIBlock, file, inodeMovido.DOUBLE_INDIRECT_BLOCKS[tmp], blockSize, posInicialBlock);
            for(int j = 0; j < blockSize; j++){
                searchBlock(inodeIBlock, file, inodeDIBlock[j], blockSize, posInicialBlock);
                for(int k = 0; k < blockSize; k++){
                    searchBlock(inodeBlock, file, inodeIBlock[k], blockSize, posInicialBlock);
                    for(int l = 0; l < blockSize; l++){
                        fileContent += inodeBlock[l];
                        cont++;
                        if(cont == inodeMovido.SIZE)
                            break;
                    }
                }
            }    
        }
    }
    if(fileNameOld != fileNameNew){
        fileNameNew.copy(inodeMovido.NAME, fileNameNew.length()+1);
        fseek(file, posInicialInode, SEEK_SET);
        fseek(file, inodeMovidoAdress * sizeof(INODE), SEEK_CUR);
        fwrite(&inodeMovido, sizeof(INODE), 1 , file);
        fclose(file);
        delete inodeBlock;
        delete inodeIBlock;
        delete inodeDIBlock;
        return;
    }

    remove(fsFileName, oldPath);
    if(inodeMovido.IS_DIR == 0x01)
        addDir(fsFileName, newPath);
    else
        addFile(fsFileName, newPath, fileContent);
    fclose(file);
    delete inodeBlock;
    delete inodeIBlock;
    delete inodeDIBlock;
}