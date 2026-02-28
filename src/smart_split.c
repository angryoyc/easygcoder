
/**
 * Проверяет, является ли строка началом числа
 * Поддерживает: целые, десятичные дроби, отрицательные
 */
static bool is_number_start(const char* str) {
    if (!str) return false;
    // Обработка отрицательных чисел
    if (*str == '-') {
        return isdigit(*(str + 1)) || *(str + 1) == '.';
    }
    return isdigit(*str) || *str == '.';
}

/**
 * Получает длину числа в начале строки
 */
static size_t get_number_length(const char* str) {
    if (!str || !is_number_start(str)) return 0;
    const char* p = str;
    // Пропускаем знак минуса
    if (*p == '-') p++;
    // Парсим целую часть
    while (isdigit(*p)) p++;
    // Парсим дробную часть
    if (*p == '.') {
        p++;
        while (isdigit(*p)) p++;
    }
    return p - str;
}

/**
 * Внутренняя функция для очистки при ошибках
 */
static void split_result_cleanup(SplitResult* result) {
    if (result) {
        free(result->buffer);
        free(result->tokens);
        result->buffer = NULL;
        result->tokens = NULL;
        result->count = -1;
    }
}

/**
 * Подсчитывает количество токенов в числовом режиме
 */
static int count_numeric_tokens(const char* str) {
    if (!str || *str == '\0') return 1;
    int count = 0;
    const char* p = str;
    while (*p) {
        // Пропускаем разделители (не числа)
        while (*p && !is_number_start(p)) p++;
        // Нашли число
        if (*p && is_number_start(p)) {
            count++;
            size_t num_len = get_number_length(p);
            p += num_len;
        }
    }
    return count > 0 ? count : 1; // Минимум 1 токен
}

/**
 * Разбивает строку в числовом режиме
 */
static SplitResult split_numeric(const char* str) {
    SplitResult result = {NULL, NULL, -1, SPLIT_MODE_NUMERIC};
    size_t str_len = strlen(str);
    // Подсчитываем количество токенов
    int token_count = count_numeric_tokens(str);
    // Выделяем память
    result.buffer = (char*)malloc(str_len + token_count + 1);
    if (!result.buffer) return result;
    result.tokens = (char**)malloc(token_count * sizeof(char*));
    if (!result.tokens) {
        free(result.buffer);
        return result;
    }
    // Заполняем буфер
    char* write_ptr = result.buffer;
    const char* read_ptr = str;
    int token_idx = 0;
    while (*read_ptr && token_idx < token_count) {
        // Пропускаем разделители
        while (*read_ptr && !is_number_start(read_ptr)) {
            read_ptr++;
        }
        if (!*read_ptr) break;
        // Запоминаем начало токена
        result.tokens[token_idx] = write_ptr;
        // Копируем число
        size_t num_len = get_number_length(read_ptr);
        memcpy(write_ptr, read_ptr, num_len);
        write_ptr += num_len;
        *write_ptr++ = '\0';
        read_ptr += num_len;
        token_idx++;
    }
    // Если не нашли ни одного числа (например, строка "abc"), возвращаем всю строку как один токен
    if (token_idx == 0) {
        free(result.buffer);
        free(result.tokens);
        result.buffer = (char*)malloc(str_len + 2);
        result.tokens = (char**)malloc(sizeof(char*));
        if (!result.buffer || !result.tokens) {
            free(result.buffer);
            free(result.tokens);
            result.buffer = NULL;
            result.tokens = NULL;
            return result;
        }
        strcpy(result.buffer, str);
        result.tokens[0] = result.buffer;
        result.count = 1;
        return result;
    }
    result.count = token_idx;
    return result;
}

/**
 * Разбивает строку по заданному разделителю (явный режим)
 */
static SplitResult split_explicit(const char* str, const char* delim) {
    SplitResult result = {NULL, NULL, -1, SPLIT_MODE_EXPLICIT};
    size_t str_len = strlen(str);
    size_t delim_len = strlen(delim);
    // Если разделитель длиннее строки или строка пуста
    if (delim_len > str_len || str_len == 0) {
        result.buffer = (char*)malloc(str_len + 2);
        if (!result.buffer) return result;
        result.tokens = (char**)malloc(sizeof(char*));
        if (!result.tokens) {
            free(result.buffer);
            return result;
        }
        if (str_len > 0) {
            strcpy(result.buffer, str);
        } else {
            result.buffer[0] = '\0';
        }
        result.tokens[0] = result.buffer;
        result.count = 1;
        return result;
    }
    // Подсчитываем количество токенов
    int token_count = 1;
    const char* search_ptr = str;
    while ((search_ptr = strstr(search_ptr, delim)) != NULL) {
        token_count++;
        search_ptr += delim_len;
    }
    // Выделяем память
    result.buffer = (char*)malloc(str_len + token_count + 1);
    if (!result.buffer) return result;
    result.tokens = (char**)malloc(token_count * sizeof(char*));
    if (!result.tokens) {
        free(result.buffer);
        return result;
    }
    // Заполняем буфер
    char* write_ptr = result.buffer;
    int token_idx = 0;
    const char* read_ptr = str;
    const char* next_delim;
    while (token_idx < token_count - 1) {
        next_delim = strstr(read_ptr, delim);
        if (!next_delim) {
            split_result_cleanup(&result);
            return result;
        }
        result.tokens[token_idx] = write_ptr;
        size_t segment_len = next_delim - read_ptr;
        memcpy(write_ptr, read_ptr, segment_len);
        write_ptr += segment_len;
        *write_ptr++ = '\0';
        read_ptr = next_delim + delim_len;
        token_idx++;
    }
    // Последний токен
    result.tokens[token_idx] = write_ptr;
    size_t remaining_len = strlen(read_ptr);
    memcpy(write_ptr, read_ptr, remaining_len + 1);
    result.count = token_count;
    return result;
}

/**
 * Универсальная функция разбиения строки.
 * @param str Исходная строка
 * @param delim Разделитель (если NULL - включается числовой режим)
 * @return SplitResult структура с результатами
 */
SplitResult smart_split(const char* str, const char* delim) {
    // Проверка входных параметров
    if (!str) {
        SplitResult result = {NULL, NULL, -1, SPLIT_MODE_EXPLICIT};
        return result;
    }
    if (strlen(str) >= MAX_INPUT_SIZE) {
        SplitResult result = {NULL, NULL, -1, SPLIT_MODE_EXPLICIT};
        return result;
    }
    // Выбор режима
    if (delim == NULL) {
        // Числовой режим
        return split_numeric(str);
    } else if (strlen(delim) == 0) {
        // Пустой разделитель - особый случай
        SplitResult result = {NULL, NULL, -1, SPLIT_MODE_EXPLICIT};
        return result;
    } else {
        // Явный режим
        return split_explicit(str, delim);
    }
}

