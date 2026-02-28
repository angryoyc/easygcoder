#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define EPSILON 0.00001
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define RESET   "\033[0m"
#define DARK    "\033[2m"
#define ITALIC  "\033[3m"

// Для подсчета общего числа проваленных утверждений (ассертов)
static int total_failures = 0;
static int total = 0;

// Для отслеживания провалилось ли текущее 'IT' утверждение
// Используется для вывода "OK" или "FAIL" в макросе IT
#define TEST_FAILED_FLAG_NAME __test_failed_in_current_it

// --- базовые assert'ы ---
// Удален return;
#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        printf("\n" RED "✗" RESET " FAIL at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        total_failures++; \
        TEST_FAILED_FLAG_NAME = 1; \
    }else{ \
        total++; \
    }

// Удален return;
#define ASSERT_EQ_INT(expected, actual) \
    if ((expected) != (actual)) { \
        printf("\n" RED "✗" RESET " FAIL at %s:%d: expected %d, got %d\n", \
               __FILE__, __LINE__, (expected), (actual)); \
        total_failures++; \
        TEST_FAILED_FLAG_NAME = 1; \
    }else{ \
        total++; \
    }

// Исправлен формат для double (%f вместо %d для __LINE__)
// Удален return;
#define ASSERT_EQ_DOUBLE(expected, actual) \
    if (fabs((expected) - (actual)) > EPSILON) { \
        printf("\n" RED "✗" RESET " FAIL at %s:%d: expected %f, got %f\n", \
               __FILE__, __LINE__, (double)(expected), (double)(actual)); \
        total_failures++; \
        TEST_FAILED_FLAG_NAME = 1; \
    }else{ \
        total++; \
    }

// --- BDD синтаксис ---

#define DESCRIBE(name) \
    void describe_##name(void); \
    int main(void) { \
        printf("Тесты: %s\n", #name); \
        describe_##name(); \
        printf("\n\n" ITALIC "Успешных: " GREEN "%d" RESET ITALIC "  Проваленных: %s%d" RESET "\n", total, (total_failures==0)?GREEN:RED, total_failures); \
        return total_failures > 0 ? 1 : 0; \
    } \
    void describe_##name(void)

#define CONTEXT(title) printf(ITALIC "\n  %s\n\n" RESET, title);

#define IT(title, block) \
    do { \
        int TEST_FAILED_FLAG_NAME = 0; /* Флаг для текущего теста */ \
        printf(DARK "    • %s ... " RESET, title); \
        block; /* Выполняем тестовый блок */ \
        if (TEST_FAILED_FLAG_NAME == 0) { \
            printf("[" GREEN " ✔ " RESET "]\n"); \
        } else { \
            /* Вывод FAIL уже сделан в макросе ASSERT, здесь просто переход на новую строку */ \
            printf("\n"); \
        } \
    } while(0)
