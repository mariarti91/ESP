/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: Nikita
 *
 * Created on May 23, 2017, 5:41 AM
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "hash.h"

using namespace std;

struct bitArray
{
    uint32_t bits[125]; //Предполагается, что сообщение не будет больше 1000 байт 
    unsigned char lastSignificantBit[125];
    uint32_t lastIndex;
};

int ESP_Protect(struct bitArray *message, int mode);
int ESP_Process(struct bitArray *message, struct bitArray *payload);
int encrypt(struct bitArray *payload);
int decrypt (struct bitArray *payload, unsigned char *protocol);
int appendBits (struct bitArray *dest, struct bitArray src);
int charToBitArray (struct bitArray *dest, char src[]);
uint32_t charToInt(char src[]);
int firstCut(char dest[], struct bitArray *src, unsigned char reqBits);
int bitArrayOffset(struct bitArray *src, uint32_t bitsCut, int lastIndex);
int integrityFunction (struct bitArray payload, uint32_t *hash);
int appendInt(struct bitArray *dest, int value); 
int bitToChar(char string[], struct bitArray *src);
int appendEspTrailer(struct bitArray *payload, unsigned char nextProtocol);
int copyBitArray(struct bitArray *dest, struct bitArray src, int bitNum, int offset);//Добавление padding, padLength и nextHeader
int removePadding(struct bitArray *data);

int seqN = 0;

uint32_t charToInt(char src[])
{
    uint32_t temp = 0, i = 0;
    while (i<4)
    {
        unsigned char temp1 = src[i];
        uint32_t value = temp1 << (3-i)*8;
        temp += value;
        i++;
    }
    return temp;
            
}
int charToBitArray (struct bitArray *dest, char src[])
{
    int i,j, len;
    len = strlen(src);
    
    if (len == 0) return 10; //Error "Source is empty"
    
    j = 0; 
    if ((len/4) != 0)
    {
        while (j < len / 4 + 1)
        {
            int temp = 0;
            i = 0;
            while (i<4)
            {
                temp += src[j * 4 + i] << (3-i)*8;
                i++;
            }
            dest->bits[j] = temp;
            if (j < len / 4) 
            {
                dest->lastSignificantBit[j] = 32;
            }
            else 
            {
                dest->lastSignificantBit[j] = fmod(len, 4)*8;
            }
            j++;
        }
    }
    else
    {
        while (j <= len / 4)
        {
            int temp = 0;
            i = 0;
            while (i<4)
            {
                temp += src[j + i] << (3-i)*8;
                i++;
            }
            dest->bits[j] = temp;
            dest->lastSignificantBit[j] = 32;
            j++;
        }
    }
    dest->lastIndex = j - 1;
    return 0;
    
    
}

int firstCut(char dest[], struct bitArray *src, unsigned char reqBits)
{
    int k = 0;
    int reqBytes = reqBits / 8;
    int reqBitsTemp = reqBits;
    reqBitsTemp -=reqBytes*8;
    int offset = 3;
    if (reqBitsTemp > 0)
    {
        dest[3] = (src->bits[k] >> ((4 - reqBytes - 1)*8)) & 0xff;
        dest[3] >>= 8 - reqBitsTemp;

        while (reqBytes > 0)
        {
            dest[4 - reqBytes - 1] = (src->bits[k] >> (offset*8) ) & 0xff;
            reqBytes -= 1;
            offset--;
        }

        char copy;
        offset = 3 - offset;
        while (offset > 0)
        {
            copy = dest[3-offset];
            dest[3-offset] <<=(8 - reqBitsTemp);
            dest[4-offset] |= dest[3-offset];
            copy >>=reqBitsTemp;
            dest[3-offset] = copy;
            offset--;
        }
    }
    else
    {
        while (reqBytes > 0)
        {
            unsigned char value = src->bits[k] >> (offset*8);
            dest[4-reqBytes] = value;
            reqBytes -= 1;
            offset--;
        }
    }
    uint32_t temp = charToInt(dest);
    temp <<= (32-reqBits);
    src->bits[k] -= temp;
    return 0;
}

int bitArrayOffset(struct bitArray *src, uint32_t bitsCut, int lastIndex)
{
    int i = 0;
    char temp;
    while (bitsCut>= 32)
    {
        int k = 0;
        while (k < src->lastIndex)
        {
            src->bits[k] = src->bits[k+1];
            src->lastSignificantBit[k] = src->lastSignificantBit[k+1];
            k++;
        }
        src->bits[k] = 0;
        src->lastSignificantBit[k] = 0;
        src->lastIndex--;
        bitsCut -= 32;
    }
    if (bitsCut > 0)
    {
        while (i < lastIndex)
        {
            int copy;
            copy = src->bits[i+1];
            src->bits[i] <<=bitsCut;
            copy >>= (32 - bitsCut);
            src->bits[i] += copy;
            src->bits[i+1] <<= (32 - bitsCut);
            if (bitsCut < src->lastSignificantBit[i+1]) 
            {
                temp = bitsCut;
            }
            else 
            {
                temp = src->lastSignificantBit[i+1];
            }
            src->lastSignificantBit[i] = 32 - bitsCut + temp;
            i++;
        }
        if (bitsCut < src->lastSignificantBit[i]) 
        {
            src->lastSignificantBit[i] -= bitsCut;
        }
        else 
        {
            src->lastSignificantBit[i] = 0;
        }

        if (lastIndex > 0)
            if ((src->lastSignificantBit[lastIndex] == 0)
            &&(src->lastSignificantBit[lastIndex - 1] != 32))
            src->lastIndex -= 1;
    }
    
    return 0;
}

int appendBits (struct bitArray *dest, struct bitArray src)
{
    int j,k;
    // Проверка на размер 
    if ((dest->lastIndex+src.lastIndex)*8 < 1000)
    {
        j = dest->lastIndex, k = 0;
        char reqBits;
        reqBits = 32 - dest->lastSignificantBit[dest->lastIndex];
        if (src.lastSignificantBit[k] >= reqBits)
        {
            char tempByteArray[4] = {0};
            char *ptr = tempByteArray;
            firstCut(ptr, &src, reqBits);
            bitArrayOffset(&src, reqBits, src.lastIndex);
            uint32_t temp = charToInt(tempByteArray);
            dest->bits[j] += temp;
            dest->lastSignificantBit[j] = 32;
            dest->lastIndex++;
            while (k<=src.lastIndex)
            {
                j++;
                dest->bits[j] = src.bits[k];
                dest->lastSignificantBit[j] = src.lastSignificantBit[k];
                dest->lastIndex++;
                k++;
            }
        }
        else
        {
            src.bits[k] >>= dest->lastSignificantBit[dest->lastIndex];
            dest->bits[j] += src.bits[k];
            dest->lastSignificantBit[j] += src.lastSignificantBit[k];
        }
    }
    else
    {
        return 100; // Ошибка "Суммарный размер массивов больше 1000 байт" 
    }
    if (dest->lastSignificantBit[dest->lastIndex - 1] != 32) dest->lastIndex--;
    return 0;
}

int main(int argc, char** argv) {
    
    char data[979] = ""; 
    struct bitArray ipHeader = {{0,0,0x00110000,0,0},{32,32,32,32,32}, 5}; //Размер Ip header - 20 байт. Данный заголовок заполнен 0 кроме поля протокол (там стоит 17 - UDP)
    struct bitArray msg = {{0},{0}, 0}; 
    char strmode[3] = "";
    int mode;
    scanf("%s", strmode);
    mode = atoi(strmode);
    printf("%d \n", mode);
    scanf("%s", data);
    charToBitArray (&msg, data);    
    //strcpy(message, ipHeader);
    appendBits(&ipHeader, msg);
    ESP_Protect(&ipHeader, mode);
    if (ESP_Process(&ipHeader, &msg) == 10) 
    {
        printf("Integrity compromised");
    }
    else
    {
        int check = 0;
        if (msg.lastSignificantBit[msg.lastIndex] > 0) check = 1; 
        char string[msg.lastIndex + check] = {0};
        bitToChar(string, &msg);
        const char * finalString = (const char *) string;
        printf(finalString);
    }
    return 0;
}

int copyBitArray(struct bitArray *dest, struct bitArray src, int bitNum, int offset = 0)
{

    if (offset != 0)
    {
        if (offset > 32)
        {
            while (offset > 32)
            {
                int k = 0;
                while (k <= src.lastIndex)
                {
                    src.bits[k] = src.bits[k+1];
                    src.lastSignificantBit[k] = src.lastSignificantBit[k+1];
                    k++;
                }
                src.lastIndex--;
                offset -= 32;
            }
            bitArrayOffset(&src, offset, src.lastIndex);
        }
        else
        {
            bitArrayOffset(&src, offset, src.lastIndex);
        }
    }
    int i = 0;
    
    dest->lastIndex = 0;
    memset(dest->bits, 0, 125);
    memset(dest->lastSignificantBit, 0, 125);
    while (bitNum>=32)
    {
        dest->bits[i] = src.bits[0];
        dest->lastSignificantBit[i] = src.lastSignificantBit[0];
        dest->lastIndex++;
        int k = 0;
        while (k <= src.lastIndex)
        {
            src.bits[k] = src.bits[k+1];
            src.lastSignificantBit[k] = src.lastSignificantBit[k+1];
            k++;
        }
        src.lastIndex--;
        i++;
        bitNum -= 32;
    }
    if (bitNum>0)
    {
        char temp[4] = {0};
        char *ptr = temp;
        firstCut(ptr, &src, bitNum);
        dest->bits[i] = charToInt(temp);
        dest->lastSignificantBit[i] = src.lastSignificantBit[i] - bitNum;
    }
    else dest->lastIndex++;
    if (dest->lastSignificantBit[dest->lastIndex - 1] != 32) dest->lastIndex--;
    return 0;
}

int appendEspTrailer(struct bitArray *payload, unsigned char nextProtocol)
{
    unsigned char padLength = 0;
    if (payload->lastSignificantBit[payload->lastIndex] <=16)
    {
        padLength = 16 - payload->lastSignificantBit[payload->lastIndex];
        payload->lastSignificantBit[payload->lastIndex] = 16;
        char EspTrailer[4] = {0, 0, padLength, nextProtocol};
        int trailer = charToInt(EspTrailer);
        payload->bits[payload->lastIndex] += trailer;
        payload->lastSignificantBit[payload->lastIndex] = 32;
        payload->lastIndex++;
    }
    else
    {
        padLength = 48 - payload->lastSignificantBit[payload->lastIndex];
        payload->lastSignificantBit[payload->lastIndex] = 32;
        payload->lastIndex++;
        char EspTrailer[4] = {0, 0, padLength, nextProtocol};
        int trailer = charToInt(EspTrailer);
        payload->bits[payload->lastIndex] += trailer;
        payload->lastSignificantBit[payload->lastIndex] = 32;
        payload->lastIndex++;
    }
    
}

int appendInt(struct bitArray *dest, int value) 
{
    if (dest->lastSignificantBit[dest->lastIndex] == 0)
    {
        dest->bits[dest->lastIndex] = value;
        dest->lastSignificantBit[dest->lastIndex] = 32;
        dest->lastIndex++;
    }
    else
    {
        int temp = value;
        temp >>= dest->lastSignificantBit[dest->lastIndex];
        dest->bits[dest->lastIndex] += temp;
        dest->lastSignificantBit[dest->lastIndex + 1] = dest->lastSignificantBit[dest->lastIndex];
        dest->lastSignificantBit[dest->lastIndex] = 32;
        dest->lastIndex++;
        value <<= (32 - dest->lastSignificantBit[dest->lastIndex]);
        dest->bits[dest->lastIndex] = value;
    }
    return 0;
    
}

int bitToChar(char string[], struct bitArray *src)
{
    int i=0,k=0;
    int temp;
    while (i<= src->lastIndex)
    {
        k=0;
        while (k < 4)
        {
            temp = src->bits[i] >> (24 - k*8);
            string[i*4 + k] = (char) temp;
            k++;
        }
        i++;
    }
    
    return 0;
}

int ESP_Protect(struct bitArray *message, int mode)
{
    struct bitArray ipHeader = {{0},{0}, 0};
    copyBitArray(&ipHeader, *message, 160, 0);
    bitArrayOffset(message, 160, message->lastIndex);
    seqN++;
    
    if (mode == 1)
    {
        int f_SPI = 0x80000000; //SPI для шифрования
        
        unsigned char nextProtocol = (ipHeader.bits[2] & 0x00ff0000) >> 16; //Извлечение protocol
        
        appendEspTrailer(message, nextProtocol); 
        
        encrypt(message); 
        
        //Сбор пакета
        struct bitArray payload = {{0},{0}, 0};
        appendInt(&payload, f_SPI);
        appendInt(&payload, seqN);
        appendBits(&payload, *message);  
        struct bitArray temp = {{0},{0},0};
        temp = *message;
        decrypt(&temp, &nextProtocol);
        appendBits(&ipHeader, payload);
        
        //Изменение protocol на 50 - ESP
        ipHeader.bits[2] &= 0xff00ffff;
        ipHeader.bits[2] |= 0x00320000;   
    }
    else if (mode == 2)
    {
        int f_SPI = 0xc0000000; //SPI для целостности
        
        unsigned char nextProtocol = (ipHeader.bits[2] & 0x00ff0000) >> 16; //Извлечение protocol
        
        appendEspTrailer(message, nextProtocol);
        
        //Сбор пакета
        struct bitArray payload = {{0},{0}, 0};
        appendInt(&payload, f_SPI);
        appendInt(&payload, seqN);
        appendBits(&payload, *message); 
        
        //Расчёт ICV
        uint32_t hash = 0;
        integrityFunction (payload, &hash);
        appendInt(&payload, hash);
        appendBits(&ipHeader, payload);
        
        //Изменение protocol на 50 - ESP
        ipHeader.bits[2] &= 0xff00ffff;
        ipHeader.bits[2] |= 0x00320000;   
    }
    else if (mode == 3)
    {
        int f_SPI = 0xe0000000; //SPI для комбо-режима
        
        unsigned char nextProtocol = (ipHeader.bits[2] & 0x00ff0000) >> 16; //Извлечение protocol
        
        appendEspTrailer(message, nextProtocol); 
        
        encrypt(message); 
        
        //Сбор пакета
        struct bitArray payload = {{0},{0}, 0};
        appendInt(&payload, f_SPI);
        appendInt(&payload, seqN);
        appendBits(&payload, *message); 
        
        //Расчёт ICV
        uint32_t hash = 0;
        integrityFunction (payload, &hash);
        appendInt(&payload, hash);
        appendBits(&ipHeader, payload);
        
        //Изменение protocol на 50 - ESP
        ipHeader.bits[2] &= 0xff00ffff;
        ipHeader.bits[2] |= 0x00320000;
    }
    *message = ipHeader;
    return 0;
}

int encrypt (struct bitArray *payload) //Здесь шифрование для галочки, потом надо заменить
{
    int i = 0;
    bool check = 1;
    if (payload->lastSignificantBit[payload->lastIndex] > 0) check = 0; 
    while (i<= (payload->lastIndex - check))
    {
        payload->bits[i] += 0x12345678;
        payload->lastSignificantBit[i] = 32;
        i++;
    }
    return 0;
}

int decrypt (struct bitArray *payload, unsigned char *protocol) //Здесь шифрование для галочки, потом надо заменить
{
    int i = 0;
    bool check = 1;
    if (payload->lastSignificantBit[payload->lastIndex] > 0) check = 0; 
    while (i<= (payload->lastIndex - check))
    {
        payload->bits[i] -= 0x12345678;
        i++;
    }
    
    *protocol = payload->bits[payload->lastIndex];
    int padLength = (payload->bits[payload->lastIndex]) >> 8;
    
    if (padLength < 16)
    {
        payload->bits[payload->lastIndex] >>= padLength + 16;
        payload->bits[payload->lastIndex] <<= padLength + 16;
        payload->lastSignificantBit[payload->lastIndex] = 32 - padLength + 16;
    }
    if (padLength == 16)
    {
        payload->bits[payload->lastIndex] = 0;
        payload->lastSignificantBit[payload->lastIndex] = 0;
        payload->lastIndex--;
    }
    if (padLength > 16)
    {
        
        payload->bits[payload->lastIndex] = 0;
        payload->lastSignificantBit[payload->lastIndex] = 0;
        payload->lastIndex--;
        padLength -= 16;
        while (padLength >= 32)
        {
            payload->bits[payload->lastIndex] = 0;
            payload->lastSignificantBit[payload->lastIndex] = 0;
            payload->lastIndex--;
            padLength -= 32;
        }
        if (padLength > 0)
        {
            payload->bits[payload->lastIndex] >>= padLength;
            payload->bits[payload->lastIndex] <<= padLength;
            payload->lastSignificantBit[payload->lastIndex] = 32 - padLength;
        }
    }
    return 0;
}

int integrityFunction (struct bitArray payload, uint32_t *hash)
{
    int check = 0;
    if (payload.lastSignificantBit[payload.lastIndex] > 0) check = 1; 
    char string[payload.lastIndex + check] = {0};
    bitToChar(string, &payload);
    const char * finalString = (const char *) string;
    *hash = SuperFastHash(finalString, 5);
}

ESP_Process(struct bitArray *message, struct bitArray *payload)
{
    struct bitArray ipHeader = {{0},{0}, 0};
    copyBitArray(&ipHeader, *message, 160, 0);
    bitArrayOffset(message, 160, message->lastIndex);
    uint32_t SPI = message->bits[0];
    if (SPI == 0x80000000)
    {
        bitArrayOffset(message, 64, message->lastIndex);
        unsigned char protocol = 0;
        decrypt(message, &protocol);
        *payload = *message;
        appendBits(&ipHeader, *message);
        
        uint32_t temp = 0;
        temp += protocol;
        temp <<= 16;
        ipHeader.bits[2] &= 0xff00ffff;
        ipHeader.bits[2] |= temp; 
        
    }
    
    else if (SPI == 0xc0000000)
    {
        
        //Расчёт ICV
        uint32_t hash = 0;
        uint32_t receivedHash = message->bits[message->lastIndex - 1];
        message->bits[message->lastIndex - 1] = 0;
        message->lastSignificantBit[message->lastIndex - 1] = 0;
        message->lastIndex--;
        integrityFunction (*message, &hash);
        if (hash != receivedHash) return 10;
        
        bitArrayOffset(message, 64, message->lastIndex);
        unsigned char protocol = message->bits[message->lastIndex];
        removePadding(message);
        *payload = *message;
             
        appendBits(&ipHeader, *message);
        
        uint32_t temp = 0;
        temp += protocol;
        temp <<= 16;
        ipHeader.bits[2] &= 0xff00ffff;
        ipHeader.bits[2] |= temp; 
        
    }
    
    else if (SPI == 0xe0000000)
    {
        
        //Расчёт ICV
        uint32_t hash = 0;
        integrityFunction (*message, &hash);
        uint32_t receivedHash = message->bits[message->lastIndex];
        if (hash != receivedHash) return 10;
        
        bitArrayOffset(message, 64, message->lastIndex);
        unsigned char protocol = 0;
        decrypt(message, &protocol);
        *payload = *message;
        appendBits(&ipHeader, *message);
        
        uint32_t temp = 0;
        temp += protocol;
        temp <<= 16;
        ipHeader.bits[2] &= 0xff00ffff;
        ipHeader.bits[2] |= temp;
        
    }
    
    
}

int removePadding(struct bitArray *data)
{
    int padLength = (data->bits[data->lastIndex] & 0x0000ff00) >> 8;

    if (padLength < 16)
    {
        data->bits[data->lastIndex] >>= padLength + 16;
        data->bits[data->lastIndex] <<= padLength + 16;
        data->lastSignificantBit[data->lastIndex] = 32 - padLength + 16;
    }
    if (padLength == 16)
    {
        data->bits[data->lastIndex] = 0;
        data->lastSignificantBit[data->lastIndex] = 0;
        data->lastIndex--;
    }
    if (padLength > 16)
    {

        data->bits[data->lastIndex] = 0;
        data->lastSignificantBit[data->lastIndex] = 0;
        data->lastIndex--;
        padLength -= 16;
        while (padLength >= 32)
        {
            data->bits[data->lastIndex] = 0;
            data->lastSignificantBit[data->lastIndex] = 0;
            data->lastIndex--;
            padLength -= 32;
        }
        if (padLength > 0)
        {
            data->bits[data->lastIndex] >>= padLength;
            data->bits[data->lastIndex] <<= padLength;
            data->lastSignificantBit[data->lastIndex] = 32 - padLength;
        }
    }
    return 0;
}
