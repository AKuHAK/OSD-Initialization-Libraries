#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kernel.h>
#include <sifcmd.h>
#include <libcdvd.h>
#include "libcdvd_add.h"

#include "main.h"
#include "ps2.h"
#include "OSDInit.h"
#include "OSDHistory.h"

void BootError(void)
{
    char *args[0];

    args[0] = "BootBrowser";
    CleanUp();
    SifExitCmd();
    ExecOSD(1, args);
}

#define CNF_PATH_LEN_MAX 64
#define CNF_LEN_MAX      1024

static const char *CNFGetToken(const char *cnf, const char *key)
{
    for (; isspace(*cnf); cnf++)
    {
    }

    for (; *key != '\0'; key++, cnf++)
    {
        // End of file
        if (*cnf == '\0')
            return (const char *)-1;

        if (*cnf != *key)
            return NULL; // Non-match
    }

    return cnf;
}

static const char *CNFAdvanceLine(const char *start, const char *end)
{
    for (; start <= end; start++)
    {
        if (*start == '\n')
            return start;
    }

    BootError();

    return NULL;
}

#define CNF_PATH_LEN_MAX 64

static const char *CNFGetKey(const char *line, char *key)
{
    int i;

    // Skip leading whitespace
    for (; isspace(*line); line++)
    {
    }

    if (*line == '\0')
    { // Unexpected end of file
        return (const char *)-1;
    }

    for (i = 0; i < CNF_PATH_LEN_MAX && *line != '\0'; i++)
    {
        if (isgraph(*line))
        {
            *key = *line;
            line++;
            key++;
        }
        else if (isspace(*line))
        {
            *key = '\0';
            break;
        }
        else if (*line == '\0')
        { // Unexpected end of file. This check exists, along with the other similar check above.
            return (const char *)-1;
        }
    }

    return line;
}

static int CNFCheckBootFile(const char *value, const char *key)
{
    int i;

    for (; *key != ':'; key++)
    {
        if (*key == '\0')
            return 0;
    }

    ++key;
    if (*key == '\\')
        key++;

    for (i = 0; i < 10; i++)
    {
        if (value[i] != key[i])
            return 0;
    }

    return 1;
}

// The TRUE way (but not the only way!) to get the boot file. Read the comment above PS2DiscBoot().
static int PS2GetBootFile(char *boot)
{
    u32 k32;
    u8 key1[16];
#ifdef XCDVD_READKEY
    u8 key2[16];
#endif

    scr_printf("sceCdReadKey(0, 0, 0x004B, key1 start");
    if (sceCdReadKey(0, 0, 0x004B, key1) == 0)
    {
        scr_printf("sceCdReadKey(0, 0, 0x004B, key1");
        return 2;
    }

    scr_printf("sceCdGetError start");
    switch (sceCdGetError())
    {
        case SCECdErREAD:
            return 3;
        case 0x37:
            return 3;
        case 0: // Success condition
            break;
        default:
            scr_printf("sceCdGetError = %X", sceCdGetError());
            return 2;
    }

#ifdef XCDVD_READKEY
    if (sceCdReadKey(0, 0, 0x0C03, key2) == 0)
        return 2;

    switch (sceCdGetError())
    {
        case SCECdErREAD:
            return 3;
        case 0x37:
            return 3;
        case 0: // Success condition
            break;
        default:
            return 2;
    }

    if (OSDGetConsoleRegion() != CONSOLE_REGION_CHINA)
    { // The rest of the world
        if (key1[0] == 0 && key1[1] == 0 && key1[2] == 0 && key1[3] == 0 && key1[4] == 0)
            return 3;

        if ((key2[0] != key1[0]) || (key2[1] != key1[1]) || (key2[2] != key1[2]) || (key2[3] != key1[3]) || (key2[4] != key1[4]))
            return 3;

        if ((key1[15] & 5) != 5)
            return 3;

        if ((key2[15] & 1) == 0)
            return 3;
#endif

        boot[11] = '\0';

        k32      = (key1[4] >> 3) | (key1[14] >> 3 << 5) | ((key1[0] & 0x7F) << 10);
        boot[10] = '0' + (k32 % 10);
        boot[9]  = '0' + (k32 / 10 % 10);
        boot[8]  = '.';
        boot[7]  = '0' + (k32 / 10 / 10 % 10);
        boot[6]  = '0' + (k32 / 10 / 10 / 10 % 10);
        boot[5]  = '0' + (k32 / 10 / 10 / 10 / 10 % 10);
        boot[4]  = '_';
        boot[3]  = (key1[0] >> 7) | ((key1[1] & 0x3F) << 1);
        boot[2]  = (key1[1] >> 6) | ((key1[2] & 0x1F) << 2);
        boot[1]  = (key1[2] >> 5) | ((key1[3] & 0xF) << 3);
        boot[0]  = ((key1[4] & 0x7) << 4) | (key1[3] >> 4);

        return 0;
#ifdef XCDVD_READKEY
    }
    else
    { // China
        if (key1[0] == 0 && key1[1] == 0 && key1[2] == 0 && key1[3] == 0 && key1[4] == 0)
            return 3;

        if (key1[5] == 0)
        {
            if (key1[6] == 0 && key1[7] == 0 && key1[8] == 0 && key1[9] == 0)
                return 3;
        }

        if ((key2[0] != key1[0]) || (key2[1] != key1[1]) || (key2[2] != key1[2]) || (key2[3] != key1[3]) || (key2[4] != key1[4]) || (key2[5] != key1[5]) || (key2[6] != key1[6]) || (key2[7] != key1[7]) || (key2[8] != key1[8]) || (key2[9] != key1[9]))
            return 3;

        if ((key1[5] != key1[0]) || (key1[6] != key1[1]) || (key1[7] != key1[2]) || (key1[8] != key1[3]) || (key1[9] != key1[4]))
            return 3;

        if ((key1[15] & 7) != 7)
            return 3;

        if ((key2[15] & 3) != 3)
            return 3;

        boot[11] = '\0';

        k32      = (key1[4] >> 3 << 5) | (key1[9] >> 3) | ((key1[5] & 0x7F) << 10);
        boot[10] = '0' + (k32 % 10);
        boot[9]  = '0' + (k32 / 10 % 10);
        boot[8]  = key1[3];
        boot[7]  = '0' + (k32 / 10 / 10 % 10);
        boot[6]  = '0' + (k32 / 10 / 10 / 10 % 10);
        boot[5]  = '0' + (k32 / 10 / 10 / 10 / 10 % 10);
        boot[4]  = key1[5];
        boot[3]  = ((key1[6] & 0x3F) << 1) | (key1[5] >> 7);
        boot[2]  = ((key1[7] & 0x1F) << 2) | (key1[6] >> 6);
        boot[1]  = ((key1[8] & 0xF) << 3) | (key1[7] >> 5);
        boot[0]  = ((key1[9] & 0x7) << 4) | (key1[8] >> 4);

        return 0;
    }
#endif
}

/*  While this function uses sceCdReadKey() to obtain the filename,
    it is possible to actually parse SYSTEM.CNF and get the boot filename from BOOT2.
    Lots of homebrew software do that. */
int PS2DiscBoot(void)
{
    char ps2disc_boot[CNF_PATH_LEN_MAX] = "";             // This was originally static/global.
    char system_cnf[CNF_LEN_MAX], line[CNF_PATH_LEN_MAX]; // These were originally globals/static.
    char *args[1];
    const char *pChar, *cnf_start, *cnf_end;
    int fd, size, size_remaining, size_read;

    scr_printf("ExecutePs2GameDisc\n");

    switch (PS2GetBootFile(ps2disc_boot))
    {
        case 2:
            return 2;
        case 3:
            return 3;
    }

    scr_printf("cdrom0:\\SYSTEM.CNF;1 parse\n");
    // The browser uses open mode 5 when a specific thread is created, otherwise mode 4.
    if ((fd = open("cdrom0:\\SYSTEM.CNF;1", O_RDONLY)) < 0)
    {
        scr_printf("Can't open SYSTEM.CNF\n");
        BootError();
    }

    size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if (size >= CNF_LEN_MAX)
        size = CNF_LEN_MAX - 1;

    for (size_remaining = size; size_remaining > 0; size_remaining -= size_read)
    {
        if ((size_read = read(fd, system_cnf, size_remaining)) <= 0)
        {
            scr_printf("Can't read SYSTEM.CNF\n");
            BootError();
        }
    }
    close(fd);

    system_cnf[size] = '\0';
    cnf_end          = &system_cnf[size];

    // Parse SYSTEM.CNF
    cnf_start        = system_cnf;
    while ((pChar = CNFGetToken(cnf_start, "BOOT2")) == NULL)
        cnf_start = CNFAdvanceLine(cnf_start, cnf_end);

    if (pChar == (const char *)-1)
    { // Unexpected EOF
        BootError();
    }

    if ((pChar = CNFGetToken(pChar, "=")) == (const char *)-1)
    { // Unexpected EOF
        BootError();
    }

    CNFGetKey(pChar, line);
    if (CNFCheckBootFile(ps2disc_boot, line) == 0)
    { // Parse error
        BootError();
    }

    args[0] = ps2disc_boot;

    scr_printf("Game ID: %s\n", ps2disc_boot);

    CleanUp();
    UpdatePlayHistory(ps2disc_boot);

    SifExitCmd();
    // LoadExecPS2("rom0:PS2LOGO", 1, args);
    LoadExecPS2(ps2disc_boot, 0, NULL);

    return 0;
}
