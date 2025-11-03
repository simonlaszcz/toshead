/* vim: set ts=4 sw=4 expandtab */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#define BRANCH      (0x601a)
#define FLABLEN     (16)
#define plong(f,n)  printf("%*s = %"PRIu32"\r\n", FLABLEN, f, (uint32_t)n)
#define plhex(f,n)  printf("%*s = $%08"PRIx32"\r\n", FLABLEN, f, (uint32_t)n)
#define pword(f,n)  printf("%*s = %"PRIu16"\r\n", FLABLEN, f, (uint16_t)n)
#define pflag(f,n)  printf("%*s = %s\r\n", FLABLEN, f, n ? "true" : "false")
#define ptext(f,t)  printf("%*s = %s\r\n", FLABLEN, f, t)

static bool read_word(FILE *fin, uint16_t *b);
static bool read_long(FILE *fin, uint32_t *b);

int main(int argc, char *argv[])
{
    FILE *fin = NULL;

    if (argc < 2)
        goto abend;
    if ((fin = fopen(argv[1], "rb")) == NULL)
        goto abend;

    uint32_t tseg = 0;
    uint32_t dseg = 0;
    uint32_t bseg = 0;
    uint32_t sseg = 0;
    uint16_t w = 0;
    uint32_t l = 0;

    if (!read_word(fin, &w))
        goto abend;

    if (w != BRANCH) {
        printf("e: invalid start branch. got 0x%04x\r\n", w);
        goto abend;
    }

    if (!read_long(fin, &tseg))
        goto abend;
    plong("text", tseg);

    if (!read_long(fin, &dseg))
        goto abend;
    plong("data", dseg);

    if (!read_long(fin, &bseg))
        goto abend;
    plong("bss", bseg);
    plong("sum", tseg + dseg + bseg);

    if (!read_long(fin, &sseg))
        goto abend;
    plong("symbols", sseg);

    /* ignore reserved */
    if (!read_long(fin, &l))
        goto abend;

    if (!read_long(fin, &l))
        goto abend;
    pflag("fastload", l & 0x0001);
    pflag("alt-ram runnable", l & 0x0002);
    pflag("alt-ram malloc", l & 0x0004);

    switch ((l & 0xf0) >> 4) {
    case 0:
        ptext("memory mode", "private");
        break;
    case 1:
        ptext("memory mode", "global");
        break;
    case 2:
        ptext("memory mode", "super");
        break;
    case 3:
        ptext("memory mode", "read-only");
        break;
    default:
        ptext("memory mode", "undefined");
        break;
    }

    pflag("shared text", l & 0x1000);
    plong("TPA size", (((l & 0xF0000000) >> 28) + 1) * (128 * 1024));

    if (!read_word(fin, &w))
        goto abend;
    pflag("relocatable", ~w);

    if (w == 0) {
        if (fseek(fin, tseg + dseg + sseg, SEEK_CUR) != 0)
            goto abend;

        if (!read_long(fin, &l))
            goto abend;
        plhex("first offset", l);
        if (l & 1)
            printf("w: first offset is not even\r\n");

        long noffsets = 1;
        long sz = 4;
        int offset = fgetc(fin);

        while (!(offset == EOF || offset == 0)) {
            ++sz;
            if (offset > 1) {
                ++noffsets;
                if (offset & 1)
                    printf("w: odd offset %d @ %ld\r\n", offset, sz);
            }
            offset = fgetc(fin);
        }

        if (offset == 0)
            ++sz;

        plong("count", noffsets);
        plong("segment size", sz);
    }

    goto ok;

abend:
    printf("invalid TOS image\r\n");
    if (fin != NULL)
        fclose(fin);
    return 1;

ok:
    fclose(fin);
    return 0;
}

static bool
read_word(FILE *fin, uint16_t *b)
{
    if (fin == NULL || b == NULL)
        return false;

    *b = 0;
    
    if (fread(b, sizeof(uint16_t), 1, fin) == 0)
        return false;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint16_t low = *b >> 8;
    *b <<= 8;
    *b |= low;
#endif

    return true;
}

static bool
read_long(FILE *fin, uint32_t *b)
{
    if (fin == NULL || b == NULL)
        return false;

    *b = 0;
    
    if (fread(b, sizeof(uint32_t), 1, fin) == 0)
        return false;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t b0 = *b & 0xff000000;
    uint32_t b1 = *b & 0x00ff0000;
    uint32_t b2 = *b & 0x0000ff00;
    uint32_t b3 = *b & 0x000000ff;

    *b = b3 << 24;
    *b |= b2 << 8;
    *b |= b1 >> 8;
    *b |= b0 >> 24;
#endif

    return true;
}
