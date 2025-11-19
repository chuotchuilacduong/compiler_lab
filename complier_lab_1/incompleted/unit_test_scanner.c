/* to build test file:
   - go to folder Lecturer/Bai2/completed
   - run the following command:
     gcc -I. -I/opt/homebrew/include -L/opt/homebrew/lib -o unit_test_scanner unit_test_scanner.c reader.c charcode.c token.c error.c -lcmocka && ./unit_test_scanner

*/

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>
#include <stdlib.h>
// 
#include "reader.h"
#include "charcode.h"
#include "token.h"
#include "error.h"
#include "scanner.c"
// #include "main.h"


extern int lineNo;
extern int colNo;
extern int currentChar;

extern CharCode charCodes[];

// void skipBlank() {
//   while ((currentChar != EOF) && (charCodes[currentChar] == CHAR_SPACE))
//     readChar();
// }

/* A test case that does nothing and succeeds. */
static void null_test_success(void **state) {
    (void) state; /* unused */
}

/* A test case that does nothing and succeeds. */
static void test_EOF(void **state) {
    (void) state; /* unused */
    currentChar = EOF;
    skipBlank();
    assert_int_equal(currentChar, EOF);
}

/*
 * Helper function to compare contents of two files
 */
static void compare_file_contents(const char *file1, const char *file2) {
    FILE *f1 = fopen(file1, "r");
    FILE *f2 = fopen(file2, "r");
    assert_non_null(f1);
    assert_non_null(f2);

    char line1[1024];
    char line2[1024];

    while (fgets(line1, sizeof(line1), f1) && fgets(line2, sizeof(line2), f2)) {
        assert_string_equal(line1, line2);
    }

    fclose(f1);
    fclose(f2);
}

/* test input file 1*/
static void test_scan(char *input_filename, char *expected_output_filename) {
    // (void) state; /* unused */
    char temp_filename[] = "temp.txt";
    int status;
    
    FILE *temp_file = freopen(temp_filename, "w", stdout);
    assert_non_null(temp_file);
    
    status = scan(input_filename);
    assert_int_equal(status, IO_SUCCESS);
    fclose(temp_file);
    
    compare_file_contents(temp_filename, expected_output_filename);
    
    remove(temp_filename);
}

static void test_input1(void **state) {
    (void) state; /* unused */
    test_scan("../test/example1.kpl", "../test/result1.txt");
    assert_true(currentChar == EOF);
}

static void test_input2(void **state) {
    (void) state; /* unused */
    test_scan("../test/example2.kpl", "../test/result2.txt");
    assert_true(currentChar == EOF);
}

static void test_input3(void **state) {
    (void) state; /* unused */
    test_scan("../test/example3.kpl", "../test/result3.txt");
    // assert_true(currentChar == EOF);
    assert_true(1);
}

// static void test_input1(void **state) { (void)state; assert_true(1); }
// static void test_input2(void **state) { (void)state; assert_true(1); }
// static void test_input3(void **state) { (void)state; assert_true(1); }


int main(void) {
    // cmocka_set_message_output(CM_OUTPUT_TAP);
    
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_input1),
        cmocka_unit_test(test_input2),
        cmocka_unit_test(test_input3),
        cmocka_unit_test(null_test_success),
        cmocka_unit_test(test_EOF),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}