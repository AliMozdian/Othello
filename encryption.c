// ^_^Ali^_^
// project Othello: encryption.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_FILENAME_SIZE
#define MAX_FILENAME_SIZE 50
#endif

#ifndef MAX_FILE_SIZE
#define MAX_FILE_SIZE 200// 50 + 72 + if needed
#endif

int password[9]; // student's code

int const_a, const_k;
int const_b, const_c;


void hard_code()
{
    const_a = 3;
    const_k = 1;
    const_b = -85;
    const_c = 85;
}



void int2arr(int num, int arr[], int size)
{
    // bug
    for (int i=0; i<size; i++)
    {
        arr[size-1-i] = num % 10;
        num /= 10;
    }
}

void set_consts_from_password(int password[])
{
    // bug
    // for encoding
    int arr[9]; // 9 is the length of password (student's code)
    int counter=0;
    for (int i=0; i<9; i++)
    {
        if (password[i] % 2 != 0)
        {
            arr[counter] = password[i];
            counter++;
        }
    }
    const_a = arr[0];
    const_b = arr[1];
    // for decoding
    int i;
    for (i=0; i<9; i++)
    {
        if (password[i] % 2 == 1)
        {
            break;
        }
    }
    const_b = -(password[i]*10 + password[i+1] - 1);
    const_c = const_b * 2 - 1;
}

char encode_char(char ch)
{
    int ans = ((const_a * (int)ch + const_k) % 256);
    if (ans < 0)
        ans += 256;
    return (char) ans;
}

char decode_char(char ch)
{
    int ans = ((const_b * (int)ch + const_c) % 256);
    if (ans < 0)
        ans += 256;
    return (char) ans;
}

void encode(char str[], int size)
{
    for (int i=0; i<size; i++)
    {
        str[i] = encode_char(str[i]);
    }
}

void decode(char str[], int size)
{
    for (int i=0; i<size; i++)
    {
        str[i] = decode_char(str[i]);
    }
}


int read_to_buffer(char filename[], char buffer[])
{
    // returns the used lentgh of buffer
    FILE *fptr = fopen(filename, "r");
    if (fptr == NULL)
    {
        fclose(fptr);
        fptr = fopen(filename, "w");
        fclose(fptr);
        return 0;
    }
    int i = 0;
    char temp_Ch = fgetc(fptr);
    while (temp_Ch != EOF)
    {
        buffer[i] = temp_Ch;
        i++;
        temp_Ch = fgetc(fptr);
    }
    fclose(fptr);
    return i;
}


void write_from_buffer(char filename[], char buffer[], int size)
{
    FILE *fptr = fopen(filename, "w");
    char temp_ch;
    for (int i=0; i<size; i++)
    {
        fputc(buffer[i], fptr);
    }
    fclose(fptr);
}

void encode_manager(char filename[])
{
    char path[MAX_FILENAME_SIZE+15];
    strcpy(path, "./saved_files/");
    strcat(path, filename);
    char buffer[MAX_FILE_SIZE];
    int size = read_to_buffer(path, buffer);
    if (size == 0)
        return;
    encode(buffer, size);
    write_from_buffer(path, buffer, size);
}

void decode_manager(char filename[])
{
    char path[MAX_FILENAME_SIZE+15];
    strcpy(path, "./saved_files/");
    strcat(path, filename);
    char buffer[MAX_FILE_SIZE];
    int size = read_to_buffer(path, buffer);
    if (size == 0)
        return;
    decode(buffer, size);
    write_from_buffer(path, buffer, size);
}
