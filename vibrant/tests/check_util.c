#include <check.h>
#include <stdlib.h>

#include "../src/util.c"

/**
 * this tolerance is used for comparing rational numbers
 */
#define TOLERANCE 0.00001

START_TEST(test_s2c_1) {
  double input = 1.0;
  double expected[9] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};

  double ctm_coeffs[9];
  vibrant_saturation_to_coeffs(input, ctm_coeffs);

  for (int i = 0; i < 9; i++) {
    ck_assert_double_eq(ctm_coeffs[i], expected[i]);
  }
}

END_TEST

START_TEST(test_s2c_2) {
  double input = 2.0;
  double expected[9] = {1.66667,  -0.33333, -0.33333, -0.33333, 1.66667,
                        -0.33333, -0.33333, -0.33333, 1.66667};

  double ctm_coeffs[9];
  vibrant_saturation_to_coeffs(input, ctm_coeffs);

  for (int i = 0; i < 9; i++) {
    ck_assert_double_eq_tol(ctm_coeffs[i], expected[i], TOLERANCE);
  }
}

END_TEST

START_TEST(test_s2c_15) {
  double input = 1.5;
  double expected[9] = {1.33333,  -0.16667, -0.16667, -0.16667, 1.33333,
                        -0.16667, -0.16667, -0.16667, 1.33333};

  double ctm_coeffs[9];
  vibrant_saturation_to_coeffs(input, ctm_coeffs);

  for (int i = 0; i < 9; i++) {
    ck_assert_double_eq_tol(ctm_coeffs[i], expected[i], TOLERANCE);
  }
}

END_TEST

START_TEST(test_c2s_1) {
  double input[9] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
  double expected = 1.0;

  double result = vibrant_coeffs_to_saturation(input);
  ck_assert_double_eq_tol(result, expected, TOLERANCE);
}

END_TEST

START_TEST(test_c2s_2) {
  double input[9] = {1.66667,  -0.33333, -0.33333, -0.33333, 1.66667,
                     -0.33333, -0.33333, -0.33333, 1.66667};
  double expected = 2.0;

  double result = vibrant_coeffs_to_saturation(input);
  ck_assert_double_eq_tol(result, expected, TOLERANCE);
}

END_TEST

START_TEST(test_c2s_15) {
  double input[9] = {1.33333,  -0.16667, -0.16667, -0.16667, 1.33333,
                     -0.16667, -0.16667, -0.16667, 1.33333};
  double expected = 1.5;

  double result = vibrant_coeffs_to_saturation(input);
  ck_assert_double_eq_tol(result, expected, TOLERANCE);
}

END_TEST

Suite *ctm_coeffs_suite(void) {
  Suite *suite = suite_create("ctm_coeffs");

  TCase *tcase = tcase_create("saturation_to_coeffs");
  tcase_add_test(tcase, test_s2c_1);
  tcase_add_test(tcase, test_s2c_2);
  tcase_add_test(tcase, test_s2c_15);
  suite_add_tcase(suite, tcase);

  tcase = tcase_create("coeffs_to_saturation");
  tcase_add_test(tcase, test_c2s_1);
  tcase_add_test(tcase, test_c2s_2);
  tcase_add_test(tcase, test_c2s_15);
  suite_add_tcase(suite, tcase);

  return suite;
}

int main(void) {
  int number_failed;
  Suite *suite;
  SRunner *runner;

  suite = ctm_coeffs_suite();
  runner = srunner_create(suite);

  srunner_run_all(runner, CK_VERBOSE);
  number_failed = srunner_ntests_failed(runner);
  srunner_free(runner);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
