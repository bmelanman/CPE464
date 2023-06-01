
/* regex_replace.c
:w | !gcc % -o .%<
:w | !gcc % -o .%< && ./.%<
:w | !gcc % -o .%< && valgrind -v ./.%<
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <printf.h>
#include <regex.h>

int regex_replace(char **str, const char *pattern, const char *replace) {

    // replaces regex in pattern with replacement observing capture groups
    // *str MUST be free-able, i.e. obtained by strdup, malloc, ...
    // back references are indicated by char codes 1-31 and none of those chars can be used in the replacement string such as a tab.
    // will not search for matches within replaced text, this will begin searching for the next match after the end of prev match
    // returns:
    //   -1 if pattern cannot be compiled
    //   -2 if count of back references and capture groups don't match
    //   otherwise returns number of matches that were found and replaced

    regex_t reg;
    unsigned int replacements = 0;

    // if regex can't compile pattern, do nothing
    if (!regcomp(&reg, pattern, REG_EXTENDED)) {

        size_t nMatch = reg.re_nsub;
        regmatch_t m[nMatch + 1];
        const char *rpl, *p;

        // count back references in replace
        int br = 0;
        p = replace;

        while (1) {
            while (*++p > 31);
            if (*p) br++;
            else break;
        }

        // if br is not equal to nMatch, leave
        if (br != nMatch) {
            regfree(&reg);
            return -2;
        }

        // look for matches and replace
        char *new;
        char *search_start = *str;
        int c;

        while (!regexec(&reg, search_start, nMatch + 1, m, REG_NOTBOL)) {

            // make enough room
            new = (char *) malloc(strlen(*str) + strlen(replace));

            if (!new) exit(EXIT_FAILURE);

            *new = '\0';
            strncat(new, *str, search_start - *str);
            p = rpl = replace;

            strncat(new, search_start, m[0].rm_so); // test before pattern

            for (int k = 0; k < nMatch; k++) {

                while (*++p > 31); // skip printable char

                c = (unsigned char) *p;  // back reference (e.g. \1, \2, ...)

                strncat(new, rpl, p - rpl); // add head of rpl

                // concat match
                strncat(new, search_start + m[c].rm_so, m[c].rm_eo - m[c].rm_so);

                rpl = p++; // skip back reference, next match
            }

            strcat(new, p); // trailing of rpl

            unsigned int new_start_offset = strlen(new);
            strcat(new, search_start + m[0].rm_eo); // trailing text in *str
            free(*str);

            *str = (char *) malloc(strlen(new) + 1);
            strcpy(*str, new);

            search_start = *str + new_start_offset;

            free(new);
            replacements++;
        }

        regfree(&reg);

        // adjust size
        *str = (char *) realloc(*str, strlen(*str) + 1);

        return (int) replacements;

    }

    return -1;
}

int main(void) {

    char input[250] = "Lorem ipsum sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor.";
    char output[300] = {0};
    char regexStr[16] = "/(.{0,80}\\S)/g";

    printf("Input: \n%s\n\n", input);

    regex_replace(((char **) &input), regexStr, "\r\n");

    printf("Output: \n%s\n\n", output);

    return 0;
}
