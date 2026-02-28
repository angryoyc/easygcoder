int copy_file(const char *source, const char *destination){
    FILE *src, *dst;
    char buffer[4096];
    size_t bytes;
    // Открываем исходный файл для чтения в бинарном режиме
    src = fopen(source, "rb");
    if (src == NULL) {
        //perror("Ошибка открытия исходного файла");
        return -1;
    }
    // Открываем целевой файл для записи в бинарном режиме
    dst = fopen(destination, "wb");
    if (dst == NULL) {
        //perror("Ошибка создания целевого файла");
        fclose(src);
        return -1;
    }
    // Копируем данные блоками
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            //perror("Ошибка записи в целевой файл");
            fclose(src);
            fclose(dst);
            return -1;
        }
    }
    // Проверяем на ошибки чтения
    if (ferror(src)) {
        //perror("Ошибка чтения исходного файла");
        fclose(src);
        fclose(dst);
        return -1;
    }
    fclose(src);
    fclose(dst);
    printf("%s -> %s\n", source, destination);
    return 0;
}

int change_file_permissions(const char *filename, mode_t mode) {
    if(chmod(filename, mode) == 0){
        //printf("Права файла '%s' успешно изменены на %o\n", filename, mode);
        return 0;
    }else{
        fprintf(stderr, "WARNING! Error file access level changing\n");
        return -1;
    }
}
