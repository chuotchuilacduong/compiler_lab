#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // 1. Kiểm tra tham số đầu vào
    if (argc != 3) {
        printf("Usage: %s <file1> <file2>\n", argv[0]);
        printf("Example: %s example2 tests/example2\n", argv[0]);
        return 1;
    }

    // 2. Mở file
    FILE *file1 = fopen(argv[1], "rb");
    if (file1 == NULL) {
        perror("Error opening file 1");
        return 1;
    }

    FILE *file2 = fopen(argv[2], "rb");
    if (file2 == NULL) {
        perror("Error opening file 2");
        fclose(file1);
        return 1;
    }

    // 3. So sánh từng byte
    long byte_count = 0;
    int ch1, ch2; // Dùng int để chứa được giá trị EOF (-1)

    printf("Comparing '%s' and '%s'...\n", argv[1], argv[2]);

    while (1) {
        ch1 = fgetc(file1);
        ch2 = fgetc(file2);

        // Kiểm tra xem có file nào kết thúc chưa
        if (ch1 == EOF && ch2 == EOF) {
            // Cả 2 cùng kết thúc -> Giống nhau hoàn toàn
            printf("SUCCESS: Files are identical. Total bytes: %ld\n", byte_count);
            break; 
        }

        // Nếu một file kết thúc trước file kia -> Khác nhau về kích thước
        if (ch1 == EOF || ch2 == EOF) {
            printf("FAILURE: Files differ in size at offset %ld (0x%lX).\n", byte_count, byte_count);
            if (ch1 == EOF) printf("File 1 ended sooner.\n");
            else printf("File 2 ended sooner.\n");
            return 1;
        }

        // So sánh nội dung byte
        if (ch1 != ch2) {
            printf("FAILURE: Mismatch at offset %ld (0x%lX)\n", byte_count, byte_count);
            printf("File 1: 0x%02X\n", ch1);
            printf("File 2: 0x%02X\n", ch2);
            fclose(file1);
            fclose(file2);
            return 1;
        }

        byte_count++;
    }

    // 4. Dọn dẹp
    fclose(file1);
    fclose(file2);
    return 0;
}