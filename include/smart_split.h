#define MAX_INPUT_SIZE 1024

/**
 * Режимы работы функции разбиения
 */
typedef enum {
    SPLIT_MODE_EXPLICIT,    // Явно заданный разделитель
    SPLIT_MODE_NUMERIC       // Всё, что не число, как разделитель
} SplitMode;

/**
 * Структура для хранения результата разбиения строки.
 */
typedef struct {
    char* buffer;           // Единый буфер со скопированной строкой
    char** tokens;          // Массив указателей на начала строк в буфере
    int count;              // Количество найденных токенов
    SplitMode mode;         // Режим, в котором был сделан split
} SplitResult;

/**
 * Освобождает память, занятую структурой SplitResult.
 */
void split_result_free(SplitResult* result) {
    if (result) {
        free(result->buffer);
        free(result->tokens);
        result->buffer = NULL;
        result->tokens = NULL;
        result->count = 0;
    }
}

SplitResult smart_split(const char* str, const char* delim);
