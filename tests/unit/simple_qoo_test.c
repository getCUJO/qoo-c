#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <cmocka.h>

#include "../../src/simple-qoo.h"

#define UNUSED(x) (void)(x)

char *testfilepath = NULL;

static void test_sqa_stats_single_sample(void **state)
{
	UNUSED(state);
	struct sqa_stats *stats = sqa_stats_create();
	struct timespec measured_delay;
	measured_delay.tv_sec = 0;
	measured_delay.tv_nsec = 200 * 1000 * 1000; // 200ms
	sqa_stats_add_sample(stats, &measured_delay);
	assert_int_equal(sqa_stats_get_number_of_samples(stats), 1);
	struct timespec *tmp = sqa_stats_get_max(stats);
	assert_true((tmp->tv_sec == 0) && (tmp->tv_nsec == 200 * 1000 * 1000));
	tmp = sqa_stats_get_min(stats);
	assert_true((tmp->tv_sec == 0) && (tmp->tv_nsec == 200 * 1000 * 1000));
	double tmp2 = sqa_stats_get_sum(stats);
	assert_in_range(tmp2, 0, 99999999999);
	assert_float_equal(tmp2, 0.2, 0.000001);
	assert_true(sqa_stats_get_number_of_lost_packets(stats) == 0);
	assert_true(sqa_stats_get_delay_eq_loss_threshold(stats)->tv_sec == 15);
	assert_true(sqa_stats_get_delay_eq_loss_threshold(stats)->tv_nsec == 0);
	tmp2 = sqa_stats_get_mean(stats);
	assert_in_range(tmp2, 0, 99999999999);
	assert_float_equal(tmp2, 0.2, 0.000001);
	assert_true(sqa_stats_get_variance(stats) == 0.0);
	assert_true(sqa_stats_get_standard_deviation(stats) == 0.0);
	assert_true(sqa_stats_get_median(stats) == 0.2);
	assert_true(sqa_stats_get_percentile(stats, 0.5) == 0.2);
	assert_true(sqa_stats_get_percentile(stats, 0.9) == 0.2);
	assert_true(sqa_stats_get_percentile(stats, 0.99) == 0.2);
	sqa_stats_destroy(stats);
}

static void test_sqa_stats_multiple_samples(void **state)
{
	UNUSED(state);
	struct sqa_stats *stats = sqa_stats_create();
	struct timespec measured_delay;
	measured_delay.tv_sec = 0;
	measured_delay.tv_nsec = 200 * 1000 * 1000; // 200ms
	sqa_stats_add_sample(stats, &measured_delay);
	measured_delay.tv_sec = 0;
	measured_delay.tv_nsec = 100 * 1000 * 1000; // 100ms
	sqa_stats_add_sample(stats, &measured_delay);
	assert_int_equal(sqa_stats_get_number_of_samples(stats), 2);
	struct timespec *tmp = sqa_stats_get_max(stats);
	assert_true((tmp->tv_sec == 0) && (tmp->tv_nsec == 200 * 1000 * 1000));
	tmp = sqa_stats_get_min(stats);
	assert_true((tmp->tv_sec == 0) && (tmp->tv_nsec == 100 * 1000 * 1000));
	assert_in_range(sqa_stats_get_sum(stats), 0, 9999999);
	assert_float_equal(sqa_stats_get_sum(stats), 0.3, 0.0000001);
	assert_true(sqa_stats_get_number_of_lost_packets(stats) == 0);
	assert_float_equal(sqa_stats_get_mean(stats), 0.15, 0.0000001);
	assert_float_equal(
		sqa_stats_get_standard_deviation(stats), 0.05, 0.0000001);
	assert_float_equal(sqa_stats_get_variance(stats), 0.0025, 0.0000001);
	assert_float_equal(sqa_stats_get_median(stats), 0.2, 0.0000001);
	assert_float_equal(sqa_stats_get_percentile(stats, 50), 0.2, 0.0000001);
	assert_float_equal(sqa_stats_get_percentile(stats, 99), 0.2, 0.0000001);
	assert_float_equal(
		sqa_stats_get_percentile(stats, 99.9), 0.2, 0.0000001);
	sqa_stats_destroy(stats);
}

static void sqa_stats_test_loss(void **state)
{
	UNUSED(state);
	struct sqa_stats *stats = sqa_stats_create();
	struct timespec measured_delay;
	measured_delay.tv_sec = 15;
	measured_delay.tv_nsec = 200 * 1000 * 1000; // 200ms
	sqa_stats_add_sample(stats, &measured_delay);
	assert_int_equal(sqa_stats_get_number_of_samples(stats), 1);
	assert_int_equal(sqa_stats_get_number_of_lost_packets(stats), 1);
	assert_in_range(sqa_stats_get_loss_percentage(stats), 0, 100);
	assert_float_equal(
		sqa_stats_get_loss_percentage(stats), 100, 0.0000001);
	sqa_stats_destroy(stats);
}

static void sqa_stats_test_1M_samples(void **state)
{
	UNUSED(state);
	struct sqa_stats *stats = sqa_stats_create();
	struct timespec measured_delay;
	measured_delay.tv_sec = 0;
	measured_delay.tv_nsec = 200 * 1000 * 1000; // 200ms
	for (int i = 0; i < 1000000; i++) {
		sqa_stats_add_sample(stats, &measured_delay);
	}
	assert_int_equal(sqa_stats_get_number_of_samples(stats), 1000000);
	assert_int_equal(sqa_stats_get_number_of_lost_packets(stats), 0);
	assert_float_equal(sqa_stats_get_loss_percentage(stats), 0, 0.0000001);
	assert_float_equal(sqa_stats_get_mean(stats), 0.2, 0.0000001);
	assert_float_equal(sqa_stats_get_standard_deviation(stats), 0, 0.0001);
	sqa_stats_destroy(stats);
}

struct simple_NR_list *create_network_requirement()
{
	struct simple_NR_list *nr = malloc(sizeof(struct simple_NR_list));

	nr->nr_perf.percentiles[0] = 50;
	nr->nr_perf.percentiles[1] = 90;
	nr->nr_perf.percentiles[2] = 99;
	nr->nr_perf.latencies[0] = 0.02;
	nr->nr_perf.latencies[1] = 0.02;
	nr->nr_perf.latencies[2] = 0.02;
	nr->nr_perf.num_percentiles = 3;
	nr->nr_perf.num_latencies = 3;

	nr->nr_useless.percentiles[0] = 50;
	nr->nr_useless.percentiles[1] = 90;
	nr->nr_useless.percentiles[2] = 99;
	nr->nr_useless.latencies[0] = 0.25;
	nr->nr_useless.latencies[1] = 0.3;
	nr->nr_useless.latencies[2] = 0.4;
	nr->nr_useless.num_percentiles = 3;
	nr->nr_useless.num_latencies = 3;

	return nr;
}

static void test_example_from_readme(void **state)
{
	UNUSED(state);
	struct sqa_stats *stats = sqa_stats_create();

	// Add sample(s)
	struct timespec measured_delay;
	measured_delay.tv_sec = 0;
	measured_delay.tv_nsec = 200 * 1000 * 1000; // 200ms
	sqa_stats_add_sample(stats, &measured_delay);

	// Compute statistics
	// QoO
	struct simple_NR_list *nr = create_network_requirement();
	double qoo = sqa_stats_get_qoo(stats, nr);
	assert_float_equal(qoo, 21.739, 0.001);
	// RPM
	double rpm = sqa_stats_get_rpm(stats);
	assert_float_equal(rpm, 300, 0.000001);
	// Clean up
	free(nr);
	sqa_stats_destroy(stats);
}

int main(int argc, char *argv[])
{
	testfilepath = "./testfiles";
	if (argc == 1) {
		printf("no testfile path given as argument\n");
		testfilepath = "./testfiles";
		// char cwd[1024];
		// getcwd(cwd, sizeof(cwd));
		// printf("Current working dir: %s\n", cwd);
		// exit(1);
	} else if (argc == 2) {
		testfilepath = argv[1];
	}
	const struct CMUnitTest qed_standard_format_tests[] = {
		cmocka_unit_test(test_sqa_stats_single_sample),
		cmocka_unit_test(test_sqa_stats_multiple_samples),
		cmocka_unit_test(sqa_stats_test_loss),
		cmocka_unit_test(sqa_stats_test_1M_samples),
		cmocka_unit_test(test_example_from_readme),
	};
	return cmocka_run_group_tests(
		qed_standard_format_tests,
		NULL,
		NULL); // move list init to setup and teardown
}