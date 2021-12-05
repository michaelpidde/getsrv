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

void test_isValidRequestType() {
    std::string buffer = "GET / HTTP/1.1";
    
}

void runTests() {
    printf(
        "\n"
        "--------------------------------------------------------------------------------\n"
        "- TESTS\n"
        "--------------------------------------------------------------------------------\n"
    );

    TEST("isValidRequestType") {
        ASSERT_TRUE(isValidRequestType("GET / HTTP/1.1"), "Valid GET type should pass");
        ASSERT_FALSE(isValidRequestType(" GET / HTTP/1.1"), "Request should not be valid with leading space");
        ASSERT_FALSE(isValidRequestType("POST / HTTP/1.1"), "Request should not be valid unless type is GET");
        ASSERT_FALSE(isValidRequestType(""), "Request should not be valid if empty");
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

    printf(
        "\n"
        "--------------------------------------------------------------------------------\n"
        "     Tests: %d\n"
        "Assertions: %d/%d\n"
        "--------------------------------------------------------------------------------\n\n",
        TestsRun, TrueAssertions, TotalAssertions
    );
}