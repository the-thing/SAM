#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "debug.c"
#include "debug.h"

#include "SamTabs.h"

#include "reciter.c"
#include "reciter.h"

#include "render.c"
#include "render.h"

#include "sam.h"
#include "sam.c"

int debug;

void WriteWav(char* filename, char* buffer, int bufferlength)
{
    FILE *file = fopen(filename, "wb");
    if (file == NULL) return;
    //RIFF header
    fwrite("RIFF", 4, 1,file);
    unsigned int filesize=bufferlength + 12 + 16 + 8 - 8;
    fwrite(&filesize, 4, 1, file);
    fwrite("WAVE", 4, 1, file);

    //format chunk
    fwrite("fmt ", 4, 1, file);
    unsigned int fmtlength = 16;
    fwrite(&fmtlength, 4, 1, file);
    unsigned short int format=1; //PCM
    fwrite(&format, 2, 1, file);
    unsigned short int channels=1;
    fwrite(&channels, 2, 1, file);
    unsigned int samplerate = 22050;
    fwrite(&samplerate, 4, 1, file);
    fwrite(&samplerate, 4, 1, file); // bytes/second
    unsigned short int blockalign = 1;
    fwrite(&blockalign, 2, 1, file);
    unsigned short int bitspersample=8;
    fwrite(&bitspersample, 2, 1, file);

    //data chunk
    fwrite("data", 4, 1, file);
    fwrite(&bufferlength, 4, 1, file);
    fwrite(buffer, bufferlength, 1, file);

    fclose(file);
}

unsigned char *copy(const unsigned char *src, size_t len) {
    unsigned char *dst = malloc(len + 1);
    
    if (dst == NULL) {
        return NULL;
    }
    
    memcpy(dst, src, len);
    dst[len] = '\0';
    
    return dst;
}

int indexOf(const char* str, char c) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == c) {
            return i;
        }
    }
    
    return -1;
}

void print(char text[]) {
    int index = indexOf(text, 155);
        
    if (index < 0) {
        printf("Failed to find EOL\n");
        return;
    }
         
    const char* data = copy(text, index);
    printf("%s", data);
    free(data);
}

void print_phoneme() {
    debug = 1;
    char text[] = ". what is your name? [";
    
    if (!TextToPhonemes(text)) {
        printf("TextToPhonemes failed\n");
        return;
    }
    
    int index = indexOf(text, 155);
        
    if (index < 0) {
        printf("Failed to find EOL\n");
        return;
    }
         
    const char* data = copy(text, index);
    printf("%s\n", data);
    free(data);
}

void print_flags1(int mask) {
    for (int i = 0; i < 81; i++) {
        if (flags[i] & mask) {
            printf("%c%c\n", signInputTable1[i], signInputTable2[i]);
        }
    }
}

void print_flags2(int mask) {
    for (int i = 0; i < 81; i++) {
        if (flags[i] & mask) {
            printf("%c%c\n", signInputTable1[i], signInputTable2[i]);
        }
    }
}

void test_progress() {
    debug = 0;
    
    char text[] = "mix [";

    if (!TextToPhonemes((unsigned char *)input)) {
        printf("TextToPhonemes failed\n");
        return;
    }

    SetInput(text);

    Init();
    phonemeindex[255] = 32;

    if (!TextToPhonemes((unsigned char *)input)) {
        printf("TextToPhonemes failed\n");
        return;
    }
    
    print(input);
    
    if (!Parser1()) {
        printf("parser failed\n");
        return;
    }

    Parser2();
    CopyStress(); 
    SetPhonemeLength();
    AdjustLengths();
    Code41240();
    Clean();
    InsertBreath();

    // PrintPhonemes(phonemeindex, phonemeLength, stress);
    
    PrepareOutput();
    
    // WriteWav("test.wav", GetBuffer(), GetBufferLength() / 50);
}

void batch_text_to_phonemes(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;

        unsigned char input[256];
        for (size_t i = 0; i < len; i++)
            input[i] = (unsigned char)toupper((unsigned char)line[i]);
        input[len]   = '[';
        input[len+1] = '\0';

        printf("%s=", line);
        
        if (!TextToPhonemes(input)) {
            printf("FAILED");
            return;
        }
        
        int index = indexOf(input, 155);
        
        if (index < 0) {
            printf("Failed to find EOL\n");
        }
         
        const char* data = copy(input, index);
        printf("%s\n", data);
        free(data);
    }

    fclose(f);
}

void process_word_list(const char *filename) {
    debug = 0;
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        return;
    }

    char line[254];
    while (fgets(line, sizeof(line), f)) {
        // Strip trailing newline/carriage return
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;
        
        // printf("line length = %d\n", len);

        // Build input: uppercase word + phonetic end-marker 0x9B (already phonetic, skip reciter)
        for (size_t i = 0; i < len; i++)
            input[i] = (char)toupper((unsigned char)line[i]);
        input[len]   = '\x9b';
        input[len+1] = '\0';

        // printf("--- Word: %s ---\n", line);
        // printf("idx,phoneme,length,stress\n");

        // Initialise SAM state
        Init();
        phonemeindex[255] = 32;

        // Parse phoneme string into phonemeindex[]/stress[]
        if (!Parser1()) {
            printf("Parser1 failed\n");
            continue;
        }

        // Apply phoneme transformation rules
        Parser2();
        CopyStress();
        SetPhonemeLength();
        AdjustLengths();
        Code41240();
        Clean();
        InsertBreath();
        // PrintPhonemes(phonemeindex, phonemeLength, stress);
        // printf("%s=", line);
        
        print(input);
        printf("=");

        int i = 0;
        
        // count
        int count = 0;
        while (phonemeindex[i] != 255 && i < 255) {
            unsigned char idx = phonemeindex[i];

            if (idx < 81 || idx == 254) {
                count++;
            }

            i++;
        }
        
        printf("%d|", count);

        // indexes
        i = 0;
        while (phonemeindex[i] != 255 && i < 255) {
            unsigned char idx = phonemeindex[i];
            if (idx < 81 || idx == 254) {
                if (i != 0) {
                    printf(",");
                }
                printf("%d",idx);
            } else {
                printf("FAILURE %d,??,%d,%d\n", idx, phonemeLength[i], stress[i]);
            }
            i++;
        }
        
        printf("|");
        
        // phonemes
        i = 0;
        while (phonemeindex[i] != 255 && i < 255) {
            unsigned char idx = phonemeindex[i];
            if (idx < 81 || idx == 254) {
                if (i != 0) {
                    printf(",");
                }
                
                if (idx == 254) {
                    printf("??");
                } else {
                    printf("%c%c", signInputTable1[idx], signInputTable2[idx]);    
                }
            } else {
                printf("FAILURE %d,??,%d,%d\n", idx, phonemeLength[i], stress[i]);
            }
            i++;
        }
        
        printf("|");
        
        // length
        i = 0;
        while (phonemeindex[i] != 255 && i < 255) {
            unsigned char idx = phonemeindex[i];
            if (idx < 81 || idx == 254) {
                if (i != 0) {
                    printf(",");
                }
                printf("%d", phonemeLength[i]);
            } else {
                printf("FAILURE %d,??,%d,%d\n", idx, phonemeLength[i], stress[i]);
            }
            i++;
        }
        
        printf("|");
        
        // stress
        i = 0;
        while (phonemeindex[i] != 255 && i < 255) {
            unsigned char idx = phonemeindex[i];
            if (idx < 81 || idx == 254) {
                if (i != 0) {
                    printf(",");
                }
                printf("%d", stress[i]);
            } else {
                printf("FAILURE %d,??,%d,%d\n", idx, phonemeLength[i], stress[i]);
            }
            i++;
        }
        
        printf("\n");
    }

    fclose(f);
}

void render_file(const char *filename) {
    debug = 0;
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        return;
    }

    char line[254];
    while (fgets(line, sizeof(line), f)) {
        // Strip trailing newline/carriage return
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;
        
        // printf("line length = %d\n", len);

        // Build input: uppercase word + phonetic end-marker 0x9B (already phonetic, skip reciter)
        for (size_t i = 0; i < len; i++)
            input[i] = (char)toupper((unsigned char)line[i]);
        input[len]   = '\x9b';
        input[len+1] = '\0';

        // printf("--- Word: %s ---\n", line);
        // printf("idx,phoneme,length,stress\n");

        // Initialise SAM state
        Init();
        phonemeindex[255] = 32;

        // Parse phoneme string into phonemeindex[]/stress[]
        if (!Parser1()) {
            printf("Parser1 failed\n");
            continue;
        }

        // Apply phoneme transformation rules
        Parser2();
        CopyStress();
        SetPhonemeLength();
        AdjustLengths();
        Code41240();
        Clean();
        InsertBreath();
        
        print(input);
        printf("=");
        
        PrepareOutput();
        PrintBuffer(GetBuffer(), GetBufferLength() / 50);
    }

    fclose(f);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // print_phoneme();
    // batch_text_to_phonemes("sentences2.txt");
    
    // render_file("sam-word-list3.txt");
    
    // SetPitch(5);
    // SetMouth(230);
    // SetThroat(240);
    //
    // test_progress();

    
    return 0;
}
