/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   bitArray.h
 * Author: Nikita
 *
 * Created on June 10, 2017, 9:27 AM
 */

#ifndef BITARRAY_H
#define BITARRAY_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

struct bitArray
{
    uint32_t bits[125]; //Предполагается, что сообщение не будет больше 1000 байт 
    unsigned char lastSignificantBit[125];
    uint32_t lastIndex;
};

int appendBits (struct bitArray *dest, struct bitArray src);
int charToBitArray (struct bitArray *dest, char src[]);
uint32_t charToInt(char src[]);
int firstCut(char dest[], struct bitArray *src, unsigned char reqBits);
int bitArrayOffset(struct bitArray *src, uint32_t bitsCut, int lastIndex);
int appendInt(struct bitArray *dest, int value); 
int bitToChar(unsigned char string[], struct bitArray *src);
int appendEspTrailer(struct bitArray *payload, unsigned char nextProtocol);
int copyBitArray(struct bitArray *dest, struct bitArray src, int bitNum, int offset);//Добавление padding, padLength и nextHeader

int copyBitArray(struct bitArray *dest, struct bitArray src, int bitNum, int offset)
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

int bitToChar(unsigned char string[], struct bitArray *src)
{
    int i=0,k=0;
    int temp;
    while (i<= src->lastIndex)
    {
        k=0;
        while (k < 4)
        {
            temp = src->bits[i] >> (24 - k*8);
            string[i*4 + k] = (unsigned char) temp;
            k++;
        }
        i++;
    }
    
    return 0;
}

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
        int temp = 0;
        i = 0;
        while (i<len)
        {
            temp += src[j + i] << (3-i)*8;
            i++;
        }
        dest->bits[j] = temp;
        dest->lastSignificantBit[j] = 8*i;
        j++;
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

#endif /* BITARRAY_H */

