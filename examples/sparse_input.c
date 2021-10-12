#include <stdio.h>
#include <stdlib.h>

char *get_word(char *end) {
    char *word_ptr = NULL;
    char symbol;
    int word_len = 0;
    symbol = getchar();
    /* Пропускаем лишние пробелы и табуляции */
    while (symbol == ' ' || symbol == '\t') {
        symbol = getchar();
    }
    /* Считываем слово до следующего разделительного символа */
    while (symbol != ' ' && symbol != '\t' && symbol != '\n') {
        word_ptr = realloc(word_ptr, (word_len + 1) * sizeof(char));
        word_ptr[word_len] = symbol;
        word_len++;
        symbol = getchar();
    }
    *end = symbol;
    word_ptr = realloc(word_ptr, (word_len + 1) * sizeof(char));
    word_ptr[word_len] = '\0';
    return word_ptr;
}

char **get_list() {
    char **list = NULL;
    char last_symb = '\0';
    int list_len = 0;
    /* Считываем строку до переноса, добавляя слова в динамический массив */
    list = realloc(list, (list_len + 1) * sizeof(char *));
    list[list_len] = get_word(&last_symb);
    while (last_symb != '\n') {
        list = realloc(list, (list_len + 1) * sizeof(char *));
        list[list_len] = get_word(&last_symb);
        list_len++;
    }
    /* Последняя строка в массиве - пустая */
    list = realloc(list, (list_len + 1) * sizeof(char *));
    list[list_len] = NULL;
    return list;
}

char **remove_list(char **list) {
    int index = 0;
    while (list[index] != NULL) {
        free(list[index]);
        index++;
    }
    free(list);
    return NULL;
}

int main() {
    char **test = get_list();
    int index = 0;
    /* Выводим слова без пробелов */
    while (test[index] != NULL) {
        fputs(test[index], stdout);
        index++;
    }
    index = 0;
    putchar('\n');
    /* Выводим слова в столбик*/
    while (test[index] != NULL) {
        fputs(test[index], stdout);
        putchar('\n');
        index++;
    }
    test = remove_list(test);
    return 0;
}
