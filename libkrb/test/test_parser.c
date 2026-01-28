/*
 * KRB Parser Tests
 *
 * Unit tests for KRB file parser.
 */

#include <u.h>
#include <libc.h>
#include <stdlib.h>
#include <string.h>
#include "../krb.h"

int test_count = 0;
int pass_count = 0;
int fail_count = 0;

#define TEST_START(name) \
    do { \
        test_count++; \
        print("Test %d: %s...", test_count, name); \
    } while(0)

#define TEST_PASS() \
    do { \
        pass_count++; \
        print(" PASS\n"); \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        fail_count++; \
        print(" FAIL: %s\n", msg); \
    } while(0)

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            TEST_FAIL(msg); \
            return; \
        } \
    } while(0)

/* Create a minimal valid KRB file for testing */
static char*
create_minimal_krb_file(size_t *size_out)
{
    char *data;
    size_t size;
    uint32_t offset;

    /* Allocate enough space for a minimal KRB file */
    size = 4096;
    data = malloc(size);
    if (data == nil)
        return nil;

    memset(data, 0, size);

    /* Write header */
    /* Magic */
    data[0] = 0x4E;  /* "KRYN" little-endian */
    data[1] = 0x59;
    data[2] = 0x52;
    data[3] = 0x4B;

    /* Version */
    data[4] = 1;  /* Major = 1 */
    data[5] = 0;
    data[6] = 0;  /* Minor = 0 */
    data[7] = 0;

    /* String table offset and size (right after header at offset 128) */
    offset = 128;
    memcpy(data + 40, &offset, 4);  /* string_table_offset */

    uint32_t str_size = 256;
    memcpy(data + 72, &str_size, 4);  /* string_table_size */

    /* Write string table */
    char *str_table = data + offset;
    uint32_t str_count = 2;
    memcpy(str_table, &str_count, 4);  /* count */

    /* Strings */
    strcpy(str_table + 4, "Text");     /* String 0 */
    strcpy(str_table + 9, "Column");   /* String 1 */

    *size_out = size;
    return data;
}

void
test_load_valid_krb(void)
{
    KrbFile *file;
    char *data;
    size_t size;

    TEST_START("Load valid KRB file");

    data = create_minimal_krb_file(&size);
    TEST_ASSERT(data != nil, "Failed to create test KRB file");

    file = krb_load_from_memory(data, size);
    TEST_ASSERT(file != nil, "Failed to load KRB file");

    if (file != nil) {
        TEST_ASSERT(file->header.magic == 0x4B52594E, "Magic number mismatch");
        TEST_ASSERT(file->header.version_major == 1, "Version mismatch");
        TEST_ASSERT(file->strings.count == 2, "String count mismatch");

        krb_free(file);
        free(data);
        TEST_PASS();
    }
}

void
test_invalid_magic(void)
{
    KrbFile *file;
    char *data;
    size_t size;

    TEST_START("Reject invalid magic number");

    data = create_minimal_krb_file(&size);
    TEST_ASSERT(data != nil, "Failed to create test KRB file");

    /* Corrupt magic */
    data[0] = 0xFF;

    file = krb_load_from_memory(data, size);
    TEST_ASSERT(file == nil, "Should reject invalid magic");

    free(data);
    TEST_PASS();
}

void
test_string_table(void)
{
    KrbFile *file;
    char *data;
    size_t size;
    char *str;

    TEST_START("String table parsing");

    data = create_minimal_krb_file(&size);
    TEST_ASSERT(data != nil, "Failed to create test KRB file");

    file = krb_load_from_memory(data, size);
    TEST_ASSERT(file != nil, "Failed to load KRB file");

    if (file != nil) {
        str = krb_get_string(file, 4);  /* First string */
        TEST_ASSERT(str != nil, "Failed to get string");
        TEST_ASSERT(strcmp(str, "Text") == 0, "String content mismatch");

        str = krb_get_string(file, 9);  /* Second string */
        TEST_ASSERT(strcmp(str, "Column") == 0, "String content mismatch");

        krb_free(file);
        free(data);
        TEST_PASS();
    }
}

void
print_summary(void)
{
    print("\n");
    print("=== Test Summary ===\n");
    print("Total: %d\n", test_count);
    print("Passed: %d\n", pass_count);
    print("Failed: %d\n", fail_count);
    print("==================\n");
}

void
main(int argc, char *argv[])
{
    print("KRB Parser Tests\n");
    print("================\n\n");

    test_load_valid_krb();
    test_invalid_magic();
    test_string_table();

    print_summary();

    exits((fail_count == 0) ? nil : "tests failed");
}
