#include "munit.h"
#include "parser.h"

static MunitResult test_correct_http_response(const MunitParameter params[], void *user_data) {
    (void)params;
    (void)user_data;

    char http_response[] = "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
    http_request_t result;
    parse_request(http_response, &result);

    munit_assert_string_equal(result.method, "GET");
    munit_assert_string_equal(result.request_target, "/");
    munit_assert_string_equal(result.protocol, "HTTP/1.1");
    munit_assert_string_equal(result.host, "localhost:8080");

    return MUNIT_OK;
}

static MunitResult test_wrong_http_response(const MunitParameter params[], void *user_data) {
    (void)params;
    (void)user_data;

    char bad_http_response[] = "Clearly not a real request";
    http_request_t result;
    int status = parse_request(bad_http_response, &result);

    munit_assert_int(status, ==, -1);

    return MUNIT_OK;
}

// {name, test_func, setup, teardown, options, parameters}
static MunitTest tests[] = {
    {"/test_correct_response", test_correct_http_response, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {"/test_wrong_response", test_wrong_http_response, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

// {name, arr_of_tests, suites(sub suites), iterations, options }
static const MunitSuite suite = {
    "/parser_tests", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char *argv[]) { return munit_suite_main(&suite, NULL, argc, argv); }
