/*
 * tiny_scanf.c
 * Minimal UART input functions
 * Style-compatible with tiny_printf.c
 */

#include "stm32f4xx_hal.h"   /* adjust MCU family */
#include "tiny_scanf.h"

extern UART_HandleTypeDef huart1;

/* -------------------------------------------------- */
/* Low-level character input                          */
/* -------------------------------------------------- */
void tiny_getc(char *c)
{
    HAL_UART_Receive(&huart1, (uint8_t *)c, 1, HAL_MAX_DELAY);
}

/* -------------------------------------------------- */
/* String input (space/newline terminated)             */
/* -------------------------------------------------- */
void tiny_gets(char *buf)
{
    char c;

    /* skip leading spaces */
    do {
        tiny_getc(&c);
    } while (c == ' ' || c == '\r' || c == '\n' || c == '\t');

    /* read word */
    while (c != ' ' && c != '\r' && c != '\n' && c != '\t') {
        *buf++ = c;
        tiny_getc(&c);
    }

    *buf = '\0';
}

/* -------------------------------------------------- */
/* Unsigned decimal                                   */
/* -------------------------------------------------- */
void tiny_getu(unsigned int *val)
{
    char c;
    unsigned int v = 0;

    /* skip spaces */
    do {
        tiny_getc(&c);
    } while (c < '0' || c > '9');

    while (c >= '0' && c <= '9') {
        v = (v * 10u) + (unsigned int)(c - '0');
        tiny_getc(&c);
    }

    *val = v;
}

/* -------------------------------------------------- */
/* Signed decimal                                     */
/* -------------------------------------------------- */
void tiny_getd(int *val)
{
    char c;
    int v = 0;
    int neg = 0;

    /* skip spaces */
    do {
        tiny_getc(&c);
    } while (c == ' ' || c == '\r' || c == '\n' || c == '\t');

    if (c == '-') {
        neg = 1;
        tiny_getc(&c);
    }

    while (c >= '0' && c <= '9') {
        v = (v * 10) + (c - '0');
        tiny_getc(&c);
    }

    if (neg)
        v = -v;

    *val = v;
}

/* -------------------------------------------------- */
/* Hexadecimal                                        */
/* -------------------------------------------------- */
void tiny_getx(unsigned int *val)
{
    char c;
    unsigned int v = 0;

    /* skip spaces */
    do {
        tiny_getc(&c);
    } while (c == ' ' || c == '\r' || c == '\n' || c == '\t');

    while (1) {
        if (c >= '0' && c <= '9')
            v = (v << 4) | (unsigned int)(c - '0');
        else if (c >= 'a' && c <= 'f')
            v = (v << 4) | (unsigned int)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            v = (v << 4) | (unsigned int)(c - 'A' + 10);
        else
            break;

        tiny_getc(&c);
    }

    *val = v;
}
