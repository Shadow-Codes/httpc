#include "munit.h"
#include "parser.h"

static MunitResult test_correct_http_response(const MunitParameter params[], void *user_data) {
    (void)params;
    (void)user_data;

    char http_request[] = "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
    http_request_t result;
    int status = parse_request(http_request, &result);

    munit_assert_string_equal(result.method, "GET");
    munit_assert_string_equal(result.request_target, "/");
    munit_assert_string_equal(result.protocol, "HTTP/1.1");
    munit_assert_string_equal(result.host, "localhost:8080");
    munit_assert_int(status, ==, 0);
    return MUNIT_OK;
}

static MunitResult test_host_without_space(const MunitParameter params[], void *user_data) {
    (void)params;
    (void)user_data;

    char http_request[] = "GET / HTTP/1.1\r\nHost:foo\r\n\r\n";
    http_request_t result;
    int status = parse_request(http_request, &result);

    munit_assert_string_equal(result.method, "GET");
    munit_assert_string_equal(result.request_target, "/");
    munit_assert_string_equal(result.protocol, "HTTP/1.1");
    munit_assert_string_equal(result.host, "foo");
    munit_assert_int(status, ==, 0);

    return MUNIT_OK;
}

static MunitResult test_wrong_http_response(const MunitParameter params[], void *user_data) {
    (void)params;
    (void)user_data;

    char bad_http_request[] = "Clearly not a real request";
    http_request_t result;
    int status = parse_request(bad_http_request, &result);

    munit_assert_int(status, ==, -1);

    return MUNIT_OK;
}

static MunitResult test_bad_protocol(const MunitParameter params[], void *user_data) {
    (void)params;
    (void)user_data;

    char bad_protocol_request[] = "GET / HTTP/9.9\r\nHost: localhost:8080\r\n\r\n";
    http_request_t result;
    int status = parse_request(bad_protocol_request, &result);

    munit_assert_int(status, ==, 505);

    return MUNIT_OK;
}

static MunitResult test_missing_protocol(const MunitParameter params[], void *user_data) {
    (void)params;
    (void)user_data;

    char missing_protocol_request[] = "GET /\r\nHost: localhost:8080\r\n\r\n";
    http_request_t result;
    int status = parse_request(missing_protocol_request, &result);

    munit_assert_int(status, ==, -1);

    return MUNIT_OK;
}

// {name, test_func, setup, teardown, options, parameters}
static MunitTest tests[] = {
    {"/test_correct_response", test_correct_http_response, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {"/test_host_without_space", test_host_without_space, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_wrong_response", test_wrong_http_response, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_bad_protocol", test_bad_protocol, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_missing_protocol", test_missing_protocol, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

// {name, arr_of_tests, suites(sub suites), iterations, options }
static const MunitSuite suite = {
    "/parser_tests", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char *argv[]) { return munit_suite_main(&suite, NULL, argc, argv); }
