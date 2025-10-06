#include <math.h>
#include <stdio.h>
#include "simple-qoo.h"

#include <stdint.h>
struct sqa_stats *sqa_stats_create()
{
    struct sqa_stats *stats = malloc(sizeof(struct sqa_stats));
    stats->number_of_samples = 0;
    stats->number_of_lost_packets = 0;
    stats->min.tv_sec = 0;
    stats->min.tv_nsec = 0;
    stats->max.tv_sec = 0;
    stats->max.tv_nsec = 0;
    stats->delay_eq_loss_threshold.tv_sec = 15; // 15 seconds as default
    stats->delay_eq_loss_threshold.tv_nsec = 0;
    stats->shifted_sum = 0;
    stats->shifted_sum_of_squares = 0;
    stats->offset = 0.1; // 0.1 seconds as default. Just need something in the expected range of the samples.
    stats->empirical_distribution = td_new(50.0);
    return stats;
}

void sqa_stats_destroy(struct sqa_stats *statistics)
{
    td_free(statistics->empirical_distribution);
    free(statistics);
}

void sqa_stats_add_sample_nsec(struct sqa_stats *stats, long delay_nsec)
{
    struct timespec delay;
    delay.tv_sec = delay_nsec / 1000000000;
    delay.tv_nsec = delay_nsec % 1000000000;
    sqa_stats_add_sample(stats, &delay);
}

void sqa_stats_add_sample(struct sqa_stats *stats, struct timespec *delay)
{
    stats->number_of_samples++;
    // A sample is either a lost packet or a packet with a delay
    if (delay->tv_sec > stats->delay_eq_loss_threshold.tv_sec ||
        (delay->tv_sec == stats->delay_eq_loss_threshold.tv_sec &&
         delay->tv_nsec > stats->delay_eq_loss_threshold.tv_nsec)) {
        // If loss
        stats->number_of_lost_packets++;
    } else {
        // If delay
        if (stats->max.tv_sec == 0 && stats->max.tv_nsec == 0) {
            stats->min = *delay;
            stats->max = *delay;
        } else {
            // New min?
            if (delay->tv_sec < stats->min.tv_sec ||
                (delay->tv_sec == stats->min.tv_sec && delay->tv_nsec < stats->min.tv_nsec)) {
                stats->min = *delay;
            }
            // New max?
            if (delay->tv_sec > stats->max.tv_sec ||
                (delay->tv_sec == stats->max.tv_sec && delay->tv_nsec > stats->max.tv_nsec)) {
                stats->max = *delay;
            }
        }
        // Use double precision for the approximations:
        double delay_in_seconds = tspecusec(delay) / 1000000.0;

        stats->shifted_sum += delay_in_seconds - stats->offset;
        stats->shifted_sum_of_squares += pow(delay_in_seconds - stats->offset, 2.0);
        td_add(stats->empirical_distribution, delay_in_seconds, 1);
    }
}

void sqa_stats_count_loss(struct sqa_stats *stats)
{
    stats->number_of_samples += 1;
    stats->number_of_lost_packets += 1;
}

// Get functions for various statistics
int sqa_stats_get_number_of_samples(struct sqa_stats *statistics)
{
    return statistics->number_of_samples;
}

int sqa_stats_get_number_of_lost_packets(struct sqa_stats *statistics)
{
    return statistics->number_of_lost_packets;
}

double sqa_stats_get_loss_percentage(struct sqa_stats *statistics)
{
    return (double) 100.0 * statistics->number_of_lost_packets / statistics->number_of_samples;
}

struct timespec *sqa_stats_get_min(struct sqa_stats *statistics)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return NULL; // No samples (only lost packets)
    return &statistics->min;
}

struct timespec *sqa_stats_get_max(struct sqa_stats *statistics)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return NULL; // No samples (only lost packets)
    return &statistics->max;
}

double sqa_stats_get_min_as_seconds(struct sqa_stats *statistics)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return 0.0; // No samples (only lost packets)
    return tspecusec(&statistics->min) / 1000000.0;
}

double sqa_stats_get_max_as_seconds(struct sqa_stats *statistics)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return 0.0; // No samples (only lost packets)
    return tspecusec(&statistics->max) / 1000000.0;
}

double sqa_stats_get_sum(struct sqa_stats *statistics)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return 0.0; // No samples (only lost packets)
    return statistics->shifted_sum + statistics->offset * statistics->number_of_samples;
}

struct timespec *sqa_stats_get_delay_eq_loss_threshold(struct sqa_stats *statistics)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return NULL; // No samples (only lost packets)
    return &statistics->delay_eq_loss_threshold;
}

double sqa_stats_get_mean(struct sqa_stats *statistics)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return 0.0; // No samples (only lost packets)
    return statistics->offset +
           (statistics->shifted_sum / (statistics->number_of_samples - statistics->number_of_lost_packets));
}

double sqa_stats_get_trimmed_mean(struct sqa_stats *statistics, double lower_cutoff, double upper_cutoff)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return 0.0; // No samples (only lost packets)
    return td_trimmed_mean(statistics->empirical_distribution, lower_cutoff / 100.0, upper_cutoff / 100.0);
}

double sqa_stats_get_variance(struct sqa_stats *statistics)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return 0.0; // No samples (only lost packets)
    // Use the offset values of sum and sum_of_squares for numerical stability
    int number_of_latency_samples = statistics->number_of_samples - statistics->number_of_lost_packets;
    return (statistics->shifted_sum_of_squares - pow(statistics->shifted_sum, 2.0) / number_of_latency_samples) /
           (number_of_latency_samples);
}

double sqa_stats_get_standard_deviation(struct sqa_stats *statistics)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return 0.0; // No samples (only lost packets)
    return sqrt(sqa_stats_get_variance(statistics));
}

double sqa_stats_get_median(struct sqa_stats *statistics)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return 0.0; // No samples (only lost packets)
    return td_quantile(statistics->empirical_distribution, 0.5);
}

double sqa_stats_get_percentile(struct sqa_stats *statistics, double percentile)
{
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return 0.0; // No samples (only lost packets)
    return td_quantile(statistics->empirical_distribution, percentile / 100.0);
}

float domosm_linear_interpolation_between_percentiles(float perc2, float lat1, float lat3, float perc1, float perc3)
{
    return ((perc2 - perc1) * (lat3 - lat1) / (perc3 - perc1)) + lat1;
}

double sqa_stats_get_qoo(struct sqa_stats *statistics, struct simple_NR_list *nr)
{
    double qoo = 100;
    for (int i = 0; i < nr->nr_perf.num_percentiles; i++) {
        double perc = nr->nr_perf.percentiles[i];
        double mes_lat = td_quantile(statistics->empirical_distribution, (double) perc / 100);
        double nrp_lat = nr->nr_perf.latencies[i];
        double nrpou_lat = nr->nr_useless.latencies[i];
        double qoo_part = (1 - (((mes_lat) -nrp_lat) / (nrpou_lat - nrp_lat))) * 100;
        if (qoo_part < 0) {
            qoo_part = 0;
        }
        if (qoo_part < qoo) {
            qoo = qoo_part;
        }
    }
    return qoo;
}

double sqa_stats_get_rpm(struct sqa_stats *statistics)
{
    // This is a simplified RPM metric. The full version needs latency measurements for TCP, TLS and HTTP for "foreign" flows
    // and samples taken on a saturated flow. See: https://datatracker.ietf.org/doc/draft-ietf-ippm-responsiveness/
    if (statistics->number_of_samples == statistics->number_of_lost_packets)
        return 0.0; // No samples (only lost packets)
    return 60.0 / sqa_stats_get_mean(statistics);
}