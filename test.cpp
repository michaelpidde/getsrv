#include <string.h>

#include "server.h"


/*******************************************************************************
 * 
 * CRUFT TO MAKE TESTING WORK
 * 
 ******************************************************************************/
static int TestsRun;
static int TrueAssertions = 0;
static int TotalAssertions = 0;

#define TEST(name)\
{\
    start(name);\
    ++TestsRun;\
}

inline void start(const char *name)
{
    printf("%s...\n", name);
}

inline void ASSERT_TRUE(bool exp, const char *msg)
{
    if(exp) {
        ++TrueAssertions;
        printf(" .\n");
    } else {
        printf(" . ERR: %s\n", msg);
    }
    ++TotalAssertions;
}

inline void ASSERT_FALSE(bool exp, const char *msg)
{
    ASSERT_TRUE(!exp, msg);
}


/*******************************************************************************
 * 
 * TESTS
 * 
 ******************************************************************************/
void runTests() {
    printf(
        "\n"
        "--------------------------------------------------------------------------------\n"
        "- TESTS\n"
        "--------------------------------------------------------------------------------\n"
    );

    TEST("isValidHttpVersion") {
        ASSERT_TRUE(isValidHttpVersion("GET / HTTP/1.1"), "Valid HTTP version should pass");
        ASSERT_FALSE(isValidHttpVersion("GET / HTTP/2.0"), "Invalid HTTP version should not pass");
        ASSERT_FALSE(isValidHttpVersion(""), "Should not be valid if buffer is empty");
    }

    TEST("isValidRequestType") {
        ASSERT_TRUE(isValidRequestType("GET / HTTP/1.1"), "Valid GET type should pass");
        ASSERT_FALSE(isValidRequestType(" GET / HTTP/1.1"), "Request should not be valid with leading space");
        ASSERT_FALSE(isValidRequestType("POST / HTTP/1.1"), "Request should not be valid unless type is GET");
        ASSERT_FALSE(isValidRequestType(""), "Should not be valid if buffer is empty");
    }

    TEST("getStringBetween") {
        Find_Result* find = getStringBetween("ABC/DEF.", 'B', 'E');
        ASSERT_TRUE(find->found, "Result should be found");
        ASSERT_TRUE(strcmp(find->string, "C/D") == 0, "Result string should match expected");
        free(find);

        Find_Result* find2 = getStringBetween("ABC/DEF.", '#', '.');
        ASSERT_FALSE(find2->found, "Starting token should not be found so success should be false");
        ASSERT_TRUE(strlen(find2->string) == 0, "Result string should be zero length");
        free(find2);

        Find_Result* find3 = getStringBetween("ABC/DEF.", 'B', '#');
        ASSERT_FALSE(find3->found, "Ending token should not be found so success should be false");
        ASSERT_TRUE(strlen(find3->string) == 0, "Result string should be zero length");
        free(find3);

        Find_Result* find4 = getStringBetween("B", 'B', 'B');
        ASSERT_FALSE(find4->found, "Ending token should not be found since buffer is only one character which was already found so success should be false");
        ASSERT_TRUE(strlen(find4->string) == 0, "Result string should be zero length");
        free(find4);

        Find_Result* find5 = getStringBetween("AZ", 'A', 'Z');
        ASSERT_TRUE(find5->found, "Result should be found");
        ASSERT_TRUE(strlen(find5->string) == 0, "Result string should be zero length");
    }

    TEST("dictionaryFind") {
        Page page0;
        page0.key = "key0";
        page0.value = "value0";
        Page page1;
        page1.key = "key1";
        page1.value = "value1";
        Dictionary dict;
        dict.pages = (Page*)malloc(sizeof(Page) * 2);
        dict.pages[0] = page0;
        dict.pages[1] = page1;
        dict.entries = 2;

        ASSERT_TRUE(dictionaryFind(&dict, "DoesNotExist") == NULL, "Find on key that does not exist should return NULL");
        const char* find = dictionaryFind(&dict, "key1");
        ASSERT_TRUE(find != NULL && strncmp(find, page1.value, strlen(page1.value)) == 0, "Find on existing key should return Page");

        free(dict.pages);
    }

    TEST("dictionaryAdd") {
        Dictionary dict = {};
        const char* key0 = "key0";
        const char* value0 = "value0";
        dictionaryAdd(&dict, key0, value0);
        ASSERT_TRUE(dict.entries == 1, "Dictionary should have 1 entry");
        ASSERT_TRUE(strncmp(dict.pages[0].key, key0, strlen(key0)) == 0, "First key should be expected value");
        ASSERT_TRUE(strncmp(dict.pages[0].value, value0, strlen(value0)) == 0, "First value should be expected value");

        const char* key1 = "key1";
        const char* value1 = "value1";
        dictionaryAdd(&dict, key1, value1);
        ASSERT_TRUE(dict.entries == 2, "Dictionary should have 2 entries");
        ASSERT_TRUE(strncmp(dict.pages[1].key, key1, strlen(key1)) == 0, "Second key should be expected value");
        ASSERT_TRUE(strncmp(dict.pages[1].value, value1, strlen(value1)) == 0, "Second value should be expected value");
    }

    printf(
        "\n"
        "--------------------------------------------------------------------------------\n"
        "     Tests: %d\n"
        "Assertions: %d/%d\n"
        "--------------------------------------------------------------------------------\n\n",
        TestsRun, TrueAssertions, TotalAssertions
    );
}