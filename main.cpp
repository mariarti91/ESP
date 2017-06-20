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
#include "getline.h"
#include "bitArray.h"


using namespace std;


int ESP_Protect(struct bitArray *message, int mode);
int ESP_Process(struct bitArray *message, struct bitArray *payload);
int encrypt(struct bitArray *payload);
int unpackEsp(char * dest, char * src, int size);
int getBitArraySize(struct bitArray * src);
int getEspPack(char * dest, char * src, int size);
int decrypt(struct bitArray *payload, unsigned char *protocol);


int integrityFunction(struct bitArray payload, uint32_t *hash);

int removePadding(struct bitArray *data);

int seqN = 0;
const int TestSize = 24;

void printHex(char * src, int size) {
    printf("Result: ");
    int i = 0;
    for (i = 0; i < size; i++) {
        printf("0x%x ", src[i]);
    }
    printf("\n");
}

int unpackEsp(char * dest, char * src, int size) {
    struct bitArray recMsg = {
        {0},
        {0}, 0};
    struct bitArray resMsg = {
        {0},
        {0}, 0};
    charToBitArray(&recMsg, src, size);
    if (ESP_Process(&recMsg, &resMsg) == 10) {
        printf("Integrity compromised");
        return -1;
    } else {
        int check = getBitArraySize(&resMsg);
        bitToChar(dest, resMsg, check);
        printBitString(dest, check);
        return check;
    }
}

int getEspPack(char * dest, char * src, int mode, int size) {
    printBitString(src, size);
    struct bitArray msg = {
        {0},
        {0}, 0};
    charToBitArray(&msg, src, size);
    //charToBitArray (&msg, test, sizeof(test));  
    ESP_Protect(&msg, mode);
    int check = getBitArraySize(&msg);
    bitToChar(dest, msg, check);
    printBitString(dest, check);
    printHex(dest, check);
    return check;
}

int getBitArraySize(struct bitArray * src) {
    int check = 0;
    if (src->lastSignificantBit[src->lastIndex] > 0) {
        check = src->lastSignificantBit[src->lastIndex] / 8;
        if (fmod(src->lastSignificantBit[src->lastIndex], 8) > 0) check++;
    }
    check += src->lastIndex * 4;
    return check;
}

int main(int argc, char** argv) {

    char data[979] = "";
    char test[TestSize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 't', 'e', 's', 't'};
    int len = sizeof (test);
    char strmode[3] = "";
    int mode;
    int err = getLine("Enter ESP mode:", strmode, sizeof (strmode));
    mode = atoi(strmode);
    
    char * dest = (char *)malloc(2000);

    int newSize = getEspPack(dest, test, mode, TestSize);

    char * dest2 = (char *)malloc(2000);

    int newNewSize = unpackEsp(dest2, dest, newSize);
    //    struct bitArray recMsg = {{0},{0}, 0};
    //    charToBitArray (&recMsg, dest, check);
    //    if (ESP_Process(&recMsg, &msg) == 10) 
    //    {
    //        printf("Integrity compromised");
    //    }
    //    else
    //    {
    //        check = getBitArraySize(&msg);
    //        bitToChar(string, msg, check);
    //        printBitString(string, check);
    //    }
    return 0;
}

int appendEspTrailer(struct bitArray *payload, unsigned char nextProtocol) {
    unsigned char padLength = 0;
    if (payload->lastSignificantBit[payload->lastIndex] <= 16) {
        padLength = 16 - payload->lastSignificantBit[payload->lastIndex];
        payload->lastSignificantBit[payload->lastIndex] = 16;
        char EspTrailer[4] = {0, 0, padLength, nextProtocol};
        int trailer = charToInt(EspTrailer);
        payload->bits[payload->lastIndex] += trailer;
        payload->lastSignificantBit[payload->lastIndex] = 32;
        payload->lastIndex++;
    } else {
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

int ESP_Protect(struct bitArray *message, int mode) {
    struct bitArray ipHeader = {
        {0},
        {0}, 0};
    copyBitArray(&ipHeader, *message, 160, 0);
    bitArrayOffset(message, 160, message->lastIndex);
    seqN++;

    if (mode == 1) {
        int f_SPI = 0x80000000; //SPI для шифрования

        unsigned char nextProtocol = (ipHeader.bits[2] & 0x00ff0000) >> 16; //Извлечение protocol

        appendEspTrailer(message, nextProtocol);

        encrypt(message);

        //Сбор пакета
        struct bitArray payload = {
            {0},
            {0}, 0};
        appendInt(&payload, f_SPI);
        appendInt(&payload, seqN);
        appendBits(&payload, *message);
        appendBits(&ipHeader, payload);

        //Изменение protocol на 50 - ESP
        ipHeader.bits[2] &= 0xff00ffff;
        ipHeader.bits[2] |= 0x00320000;
    } else if (mode == 2) {
        int f_SPI = 0xc0000000; //SPI для целостности

        unsigned char nextProtocol = (ipHeader.bits[2] & 0x00ff0000) >> 16; //Извлечение protocol

        appendEspTrailer(message, nextProtocol);

        //Сбор пакета
        struct bitArray payload = {
            {0},
            {0}, 0};
        appendInt(&payload, f_SPI);
        appendInt(&payload, seqN);
        appendBits(&payload, *message);

        //Расчёт ICV
        uint32_t hash = 0;
        integrityFunction(payload, &hash);
        appendInt(&payload, hash);
        appendBits(&ipHeader, payload);

        //Изменение protocol на 50 - ESP
        ipHeader.bits[2] &= 0xff00ffff;
        ipHeader.bits[2] |= 0x00320000;
    } else if (mode == 3) {
        int f_SPI = 0xe0000000; //SPI для комбо-режима

        unsigned char nextProtocol = (ipHeader.bits[2] & 0x00ff0000) >> 16; //Извлечение protocol

        appendEspTrailer(message, nextProtocol);

        encrypt(message);

        //Сбор пакета
        struct bitArray payload = {
            {0},
            {0}, 0};
        appendInt(&payload, f_SPI);
        appendInt(&payload, seqN);
        appendBits(&payload, *message);

        //Расчёт ICV
        uint32_t hash = 0;
        integrityFunction(payload, &hash);
        appendInt(&payload, hash);
        appendBits(&ipHeader, payload);

        //Изменение protocol на 50 - ESP
        ipHeader.bits[2] &= 0xff00ffff;
        ipHeader.bits[2] |= 0x00320000;
    }
    *message = ipHeader;
    return 0;
}

int encrypt(struct bitArray *payload) //Здесь шифрование для галочки, потом надо заменить
{
    int i = 0;
    bool check = 1;
    if (payload->lastSignificantBit[payload->lastIndex] > 0) check = 0;
    while (i <= (payload->lastIndex - check)) {
        payload->bits[i] ^= 0x12345678;
        payload->lastSignificantBit[i] = 32;
        i++;
    }
    return 0;
}

int decrypt(struct bitArray *payload, unsigned char *protocol) //Здесь шифрование для галочки, потом надо заменить
{
    int i = 0;
    while (i <= (payload->lastIndex - 1)) {
        payload->bits[i] ^= 0x12345678;
        i++;
    }

    *protocol = payload->bits[payload->lastIndex - 1];
    unsigned char padLength = (payload->bits[payload->lastIndex - 1]) >> 8;

    if (padLength < 16) {
        payload->bits[payload->lastIndex - 1] >>= padLength + 16;
        payload->bits[payload->lastIndex - 1] <<= padLength + 16;
        payload->lastSignificantBit[payload->lastIndex - 1] = 32 - padLength - 16;
        payload->lastIndex--;
    }
    if (padLength == 16) {
        payload->bits[payload->lastIndex - 1] = 0;
        payload->lastSignificantBit[payload->lastIndex - 1] = 0;
        payload->lastIndex--;
    }
    if (padLength > 16) {

        payload->bits[payload->lastIndex - 1] = 0;
        payload->lastSignificantBit[payload->lastIndex - 1] = 0;
        payload->lastIndex--;
        padLength -= 16;
        while (padLength >= 32) {
            payload->bits[payload->lastIndex - 1] = 0;
            payload->lastSignificantBit[payload->lastIndex - 1] = 0;
            payload->lastIndex--;
            padLength -= 32;
        }
        if (padLength > 0) {
            payload->bits[payload->lastIndex - 1] >>= padLength;
            payload->bits[payload->lastIndex - 1] <<= padLength;
            payload->lastSignificantBit[payload->lastIndex - 1] = 32 - padLength;
            payload->lastIndex--;
        }
    }
    return 0;
}

int integrityFunction(struct bitArray payload, uint32_t *hash) {
    int check = 0;
    if (payload.lastSignificantBit[payload.lastIndex] > 0) {
        check = payload.lastSignificantBit[payload.lastIndex] / 4;
        if (fmod(payload.lastSignificantBit[payload.lastIndex], 4) > 0) check++;
    }
    char * string = (char *) malloc(1080);
    memset(string, 0, 1080);
    bitToChar(string, payload, sizeof (string));
    *hash = 0x12345678; // Надо сюда вставить работающую хэш-функцию
}

int ESP_Process(struct bitArray *message, struct bitArray *payload) {
    struct bitArray ipHeader = {
        {0},
        {0}, 0};
    copyBitArray(&ipHeader, *message, 160, 0);
    bitArrayOffset(message, 160, message->lastIndex);
    uint32_t SPI = message->bits[0];
    if (SPI == 0x80000000) {
        bitArrayOffset(message, 64, message->lastIndex);
        unsigned char protocol = 0;
        decrypt(message, &protocol);
        //*payload = *message;
        appendBits(&ipHeader, *message);

        uint32_t temp = 0;
        temp += protocol;
        temp <<= 16;
        ipHeader.bits[2] &= 0xff00ffff;
        ipHeader.bits[2] |= temp;

    }
    else if (SPI == 0xc0000000) {

        //Расчёт ICV
        uint32_t hash = 0;
        uint32_t receivedHash = message->bits[message->lastIndex - 1];
        message->bits[message->lastIndex - 1] = 0;
        message->lastSignificantBit[message->lastIndex - 1] = 0;
        message->lastIndex--;
        integrityFunction(*message, &hash);
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
    else if (SPI == 0xe0000000) {

        //Расчёт ICV
        uint32_t hash = 0;
        integrityFunction(*message, &hash);
        uint32_t receivedHash = message->bits[message->lastIndex - 1];
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
    *payload = ipHeader;
    return 0;

}

int removePadding(struct bitArray *data) {
    unsigned char padLength = (data->bits[data->lastIndex - 1]) >> 8;

    if (padLength < 16) {
        data->bits[data->lastIndex - 1] >>= padLength + 16;
        data->bits[data->lastIndex - 1] <<= padLength + 16;
        data->lastSignificantBit[data->lastIndex - 1] = 32 - padLength - 16;
    }
    if (padLength == 16) {
        data->bits[data->lastIndex - 1] = 0;
        data->lastSignificantBit[data->lastIndex - 1] = 0;
        data->lastIndex--;
    }
    if (padLength > 16) {

        data->bits[data->lastIndex - 1] = 0;
        data->lastSignificantBit[data->lastIndex - 1] = 0;
        data->lastIndex--;
        padLength -= 16;
        while (padLength >= 32) {
            data->bits[data->lastIndex - 1] = 0;
            data->lastSignificantBit[data->lastIndex - 1] = 0;
            data->lastIndex--;
            padLength -= 32;
        }
        if (padLength > 0) {
            data->bits[data->lastIndex - 1] >>= padLength;
            data->bits[data->lastIndex - 1] <<= padLength;
            data->lastSignificantBit[data->lastIndex] = 32 - padLength;
        }
    }
    return 0;
}
