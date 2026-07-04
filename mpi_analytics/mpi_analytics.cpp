#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <limits>

using namespace std;

// Count CSV rows excluding header
long long countCsvRows(const string& filename)
{
    ifstream file(filename);

    if (!file.is_open())
    {
        return -1;
    }

    string line;
    long long count = 0;

    getline(file, line); // skip header

    while (getline(file, line))
    {
        if (!line.empty())
        {
            count++;
        }
    }

    file.close();
    return count;
}

// Read selected CSV row range only
bool readCsvRange(
    const string& filename,
    long long startRow,
    long long rowCount,
    vector<double>& transaction,
    vector<double>& loyalty,
    bool readLoyalty)
{
    transaction.clear();
    loyalty.clear();

    if (rowCount <= 0)
    {
        return true;
    }

    ifstream file(filename);

    if (!file.is_open())
    {
        return false;
    }

    string line;

    getline(file, line); // skip header

    long long currentRow = 0;

    while (currentRow < startRow && getline(file, line))
    {
        currentRow++;
    }

    transaction.reserve((size_t)rowCount);

    if (readLoyalty)
    {
        loyalty.reserve((size_t)rowCount);
    }

    long long rowsRead = 0;

    while (rowsRead < rowCount && getline(file, line))
    {
        stringstream ss(line);
        string transaction_str, loyalty_str;

        getline(ss, transaction_str, ',');
        getline(ss, loyalty_str, ',');

        if (!transaction_str.empty())
        {
            double transactionAmount = stod(transaction_str);
            transaction.push_back(transactionAmount);

            if (readLoyalty)
            {
                double loyaltyPoints = 0.0;

                if (!loyalty_str.empty())
                {
                    loyaltyPoints = stod(loyalty_str);
                }

                loyalty.push_back(loyaltyPoints);
            }

            rowsRead++;
        }

        currentRow++;
    }

    file.close();
    return true;
}

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double total_start = MPI_Wtime();

    // =====================================================
    // EXP-4 TIMERS
    // =====================================================
    double communication_time_ms = 0.0;
    double computation_time_ms = 0.0;
    double timer_start = 0.0;
    double timer_end = 0.0;

    if (argc < 2)
    {
        if (rank == 0)
        {
            cout << "Usage: mpi_analytics.exe <csv_filename>" << endl;
            cout << "Example: mpi_analytics.exe \"C:\\MPIProject\\dataset\\ecommerce_small_1million.csv\"" << endl;
        }

        MPI_Finalize();
        return 1;
    }

    string filename = argv[1];

    long long N = 0;

    // =====================================================
    // Rank 0 counts total records
    // =====================================================
    if (rank == 0)
    {
        cout << "MPI E-commerce Data Analytics Program - EXP-4 Version" << endl;
        cout << "Dataset file: " << filename << endl;
        cout << "MPI processes: " << size << endl;

        N = countCsvRows(filename);

        if (N <= 0)
        {
            cout << "Error: Cannot open file or dataset is empty: " << filename << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        cout << "Total records counted: " << N << endl;
    }

    // =====================================================
    // COMMUNICATION: Broadcast dataset size
    // =====================================================
    timer_start = MPI_Wtime();
    MPI_Bcast(&N, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    if (N <= 0)
    {
        MPI_Finalize();
        return 1;
    }

    // =====================================================
    // Divide rows equally among all ranks including Master
    // =====================================================
    vector<long long> counts(size);
    vector<long long> starts(size);

    long long base_count = N / size;
    long long remainder = N % size;

    for (int i = 0; i < size; i++)
    {
        counts[i] = base_count;

        if (i < remainder)
        {
            counts[i]++;
        }
    }

    starts[0] = 0;

    for (int i = 1; i < size; i++)
    {
        starts[i] = starts[i - 1] + counts[i - 1];
    }

    long long local_count = counts[rank];
    long long local_start_row = starts[rank];

    vector<double> local_transaction;
    vector<double> local_loyalty;

    double read_start = MPI_Wtime();

    bool readOk = readCsvRange(
        filename,
        local_start_row,
        local_count,
        local_transaction,
        local_loyalty,
        true
    );

    if (!readOk)
    {
        cout << "Rank " << rank << " Error: Cannot open file " << filename << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    double read_end = MPI_Wtime();
    double read_time_ms = (read_end - read_start) * 1000.0;

    long long actual_local_count = (long long)local_transaction.size();

    char processorName[MPI_MAX_PROCESSOR_NAME];
    int nameLength = 0;
    MPI_Get_processor_name(processorName, &nameLength);

    cout << "Rank " << rank
        << " running on " << processorName
        << " read rows " << local_start_row
        << " to " << (local_start_row + actual_local_count - 1)
        << " (" << actual_local_count << " records)." << endl;

    // =====================================================
    // COMMUNICATION: Barrier
    // =====================================================
    timer_start = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    // =====================================================
    // TASK 1: Basic Statistics
    // =====================================================
    double stat_start = MPI_Wtime();

    timer_start = MPI_Wtime();

    double local_sum = 0.0;
    double local_min = numeric_limits<double>::max();
    double local_max = numeric_limits<double>::lowest();

    for (long long i = 0; i < actual_local_count; i++)
    {
        double value = local_transaction[(size_t)i];

        local_sum += value;

        if (value < local_min)
        {
            local_min = value;
        }

        if (value > local_max)
        {
            local_max = value;
        }
    }

    timer_end = MPI_Wtime();
    computation_time_ms += (timer_end - timer_start) * 1000.0;

    double global_sum = 0.0;
    double global_min = 0.0;
    double global_max = 0.0;

    // COMMUNICATION: Reduce sum
    timer_start = MPI_Wtime();
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    // COMMUNICATION: Reduce minimum
    timer_start = MPI_Wtime();
    MPI_Reduce(&local_min, &global_min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    // COMMUNICATION: Reduce maximum
    timer_start = MPI_Wtime();
    MPI_Reduce(&local_max, &global_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    double mean = 0.0;

    if (rank == 0)
    {
        timer_start = MPI_Wtime();
        mean = global_sum / N;
        timer_end = MPI_Wtime();
        computation_time_ms += (timer_end - timer_start) * 1000.0;
    }

    // COMMUNICATION: Broadcast mean
    timer_start = MPI_Wtime();
    MPI_Bcast(&mean, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    timer_start = MPI_Wtime();

    double local_variance_sum = 0.0;

    for (long long i = 0; i < actual_local_count; i++)
    {
        double difference = local_transaction[(size_t)i] - mean;
        local_variance_sum += difference * difference;
    }

    timer_end = MPI_Wtime();
    computation_time_ms += (timer_end - timer_start) * 1000.0;

    double global_variance_sum = 0.0;

    // COMMUNICATION: Reduce variance
    timer_start = MPI_Wtime();
    MPI_Reduce(
        &local_variance_sum,
        &global_variance_sum,
        1,
        MPI_DOUBLE,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    double variance = 0.0;
    double standard_deviation = 0.0;

    if (rank == 0)
    {
        timer_start = MPI_Wtime();
        variance = global_variance_sum / N;
        standard_deviation = sqrt(variance);
        timer_end = MPI_Wtime();
        computation_time_ms += (timer_end - timer_start) * 1000.0;
    }

    // COMMUNICATION: Broadcast standard deviation
    timer_start = MPI_Wtime();
    MPI_Bcast(&standard_deviation, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    double stat_end = MPI_Wtime();
    double stat_time_ms = (stat_end - stat_start) * 1000.0;

    // =====================================================
    // TASK 2: Histogram Generation
    // =====================================================
    double histogram_start = MPI_Wtime();

    int bin_count = 10;
    vector<long long> local_histogram(bin_count, 0);
    vector<long long> global_histogram(bin_count, 0);

    double range_min = 0.0;
    double range_max = 10000.0;
    double bin_width = (range_max - range_min) / bin_count;

    timer_start = MPI_Wtime();

    for (long long i = 0; i < actual_local_count; i++)
    {
        double value = local_transaction[(size_t)i];

        int bin_index = static_cast<int>((value - range_min) / bin_width);

        if (bin_index >= bin_count)
        {
            bin_index = bin_count - 1;
        }

        if (bin_index < 0)
        {
            bin_index = 0;
        }

        local_histogram[bin_index]++;
    }

    timer_end = MPI_Wtime();
    computation_time_ms += (timer_end - timer_start) * 1000.0;

    // COMMUNICATION: Reduce histogram
    timer_start = MPI_Wtime();
    MPI_Reduce(
        local_histogram.data(),
        global_histogram.data(),
        bin_count,
        MPI_LONG_LONG,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    double histogram_end = MPI_Wtime();
    double histogram_time_ms = (histogram_end - histogram_start) * 1000.0;

    // =====================================================
    // TASK 3: Sorting
    // =====================================================
    double sorting_start = MPI_Wtime();

    timer_start = MPI_Wtime();

    vector<double> local_sorted_transaction = local_transaction;
    sort(local_sorted_transaction.begin(), local_sorted_transaction.end());

    timer_end = MPI_Wtime();
    computation_time_ms += (timer_end - timer_start) * 1000.0;

    const int TOP_K = 5;

    vector<double> local_first5(TOP_K, numeric_limits<double>::max());
    vector<double> local_last5(TOP_K, numeric_limits<double>::lowest());

    timer_start = MPI_Wtime();

    for (int i = 0; i < TOP_K && i < (int)local_sorted_transaction.size(); i++)
    {
        local_first5[i] = local_sorted_transaction[(size_t)i];
        local_last5[i] = local_sorted_transaction[local_sorted_transaction.size() - 1 - i];
    }

    timer_end = MPI_Wtime();
    computation_time_ms += (timer_end - timer_start) * 1000.0;

    vector<double> gathered_first5;
    vector<double> gathered_last5;

    if (rank == 0)
    {
        gathered_first5.resize(size * TOP_K);
        gathered_last5.resize(size * TOP_K);
    }

    // COMMUNICATION: Gather first 5 candidates
    timer_start = MPI_Wtime();
    MPI_Gather(
        local_first5.data(),
        TOP_K,
        MPI_DOUBLE,
        rank == 0 ? gathered_first5.data() : nullptr,
        TOP_K,
        MPI_DOUBLE,
        0,
        MPI_COMM_WORLD
    );
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    // COMMUNICATION: Gather last 5 candidates
    timer_start = MPI_Wtime();
    MPI_Gather(
        local_last5.data(),
        TOP_K,
        MPI_DOUBLE,
        rank == 0 ? gathered_last5.data() : nullptr,
        TOP_K,
        MPI_DOUBLE,
        0,
        MPI_COMM_WORLD
    );
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    vector<double> global_first5;
    vector<double> global_last5;

    if (rank == 0)
    {
        timer_start = MPI_Wtime();

        sort(gathered_first5.begin(), gathered_first5.end());
        sort(gathered_last5.begin(), gathered_last5.end(), greater<double>());

        for (double value : gathered_first5)
        {
            if (value != numeric_limits<double>::max() && global_first5.size() < TOP_K)
            {
                global_first5.push_back(value);
            }
        }

        for (double value : gathered_last5)
        {
            if (value != numeric_limits<double>::lowest() && global_last5.size() < TOP_K)
            {
                global_last5.push_back(value);
            }
        }

        timer_end = MPI_Wtime();
        computation_time_ms += (timer_end - timer_start) * 1000.0;
    }

    double sorting_end = MPI_Wtime();
    double sorting_time_ms = (sorting_end - sorting_start) * 1000.0;

    // =====================================================
    // TASK 4: Pearson Correlation
    // =====================================================
    double correlation_start = MPI_Wtime();

    timer_start = MPI_Wtime();

    double local_sum_x = 0.0;
    double local_sum_y = 0.0;
    double local_sum_xy = 0.0;
    double local_sum_x2 = 0.0;
    double local_sum_y2 = 0.0;

    for (long long i = 0; i < actual_local_count; i++)
    {
        double x = local_transaction[(size_t)i];
        double y = local_loyalty[(size_t)i];

        local_sum_x += x;
        local_sum_y += y;
        local_sum_xy += x * y;
        local_sum_x2 += x * x;
        local_sum_y2 += y * y;
    }

    timer_end = MPI_Wtime();
    computation_time_ms += (timer_end - timer_start) * 1000.0;

    double global_sum_x = 0.0;
    double global_sum_y = 0.0;
    double global_sum_xy = 0.0;
    double global_sum_x2 = 0.0;
    double global_sum_y2 = 0.0;

    // COMMUNICATION: Reduce correlation values
    timer_start = MPI_Wtime();
    MPI_Reduce(&local_sum_x, &global_sum_x, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    timer_start = MPI_Wtime();
    MPI_Reduce(&local_sum_y, &global_sum_y, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    timer_start = MPI_Wtime();
    MPI_Reduce(&local_sum_xy, &global_sum_xy, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    timer_start = MPI_Wtime();
    MPI_Reduce(&local_sum_x2, &global_sum_x2, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    timer_start = MPI_Wtime();
    MPI_Reduce(&local_sum_y2, &global_sum_y2, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    double pearson_correlation = 0.0;

    if (rank == 0)
    {
        timer_start = MPI_Wtime();

        double numerator = (N * global_sum_xy) - (global_sum_x * global_sum_y);

        double denominator = sqrt(
            ((N * global_sum_x2) - (global_sum_x * global_sum_x)) *
            ((N * global_sum_y2) - (global_sum_y * global_sum_y))
        );

        if (denominator != 0.0)
        {
            pearson_correlation = numerator / denominator;
        }

        timer_end = MPI_Wtime();
        computation_time_ms += (timer_end - timer_start) * 1000.0;
    }

    double correlation_end = MPI_Wtime();
    double correlation_time_ms = (correlation_end - correlation_start) * 1000.0;

    // =====================================================
    // TASK 5: Moving Average
    // =====================================================
    double moving_average_start = MPI_Wtime();

    int window_size = 5;
    long long total_ma_count = 0;

    if (N >= window_size)
    {
        total_ma_count = N - window_size + 1;

        long long ma_base = total_ma_count / size;
        long long ma_remainder = total_ma_count % size;

        long long local_ma_count = ma_base;

        if (rank < ma_remainder)
        {
            local_ma_count++;
        }

        long long ma_start_index = 0;

        for (int i = 0; i < rank; i++)
        {
            ma_start_index += ma_base;

            if (i < ma_remainder)
            {
                ma_start_index++;
            }
        }

        long long rows_needed = local_ma_count + window_size - 1;

        vector<double> ma_transaction;
        vector<double> dummy_loyalty;

        // This is file reading, not computation or communication
        readCsvRange(
            filename,
            ma_start_index,
            rows_needed,
            ma_transaction,
            dummy_loyalty,
            false
        );

        long long local_ma_actual = 0;

        timer_start = MPI_Wtime();

        if ((long long)ma_transaction.size() >= window_size && local_ma_count > 0)
        {
            double window_sum = 0.0;

            for (int i = 0; i < window_size; i++)
            {
                window_sum += ma_transaction[(size_t)i];
            }

            for (long long i = 0; i < local_ma_count; i++)
            {
                double ma_value = window_sum / window_size;
                local_ma_actual++;

                if (i + window_size < (long long)ma_transaction.size())
                {
                    window_sum += ma_transaction[(size_t)(i + window_size)];
                    window_sum -= ma_transaction[(size_t)i];
                }
            }
        }

        timer_end = MPI_Wtime();
        computation_time_ms += (timer_end - timer_start) * 1000.0;

        long long global_ma_actual = 0;

        // COMMUNICATION: Reduce moving average count
        timer_start = MPI_Wtime();
        MPI_Reduce(
            &local_ma_actual,
            &global_ma_actual,
            1,
            MPI_LONG_LONG,
            MPI_SUM,
            0,
            MPI_COMM_WORLD
        );
        timer_end = MPI_Wtime();
        communication_time_ms += (timer_end - timer_start) * 1000.0;

        if (rank == 0)
        {
            total_ma_count = global_ma_actual;
        }
    }

    double moving_average_end = MPI_Wtime();
    double moving_average_time_ms = (moving_average_end - moving_average_start) * 1000.0;

    // For display only: first 5 moving average values calculated by Rank 0
    vector<double> display_moving_average;

    if (rank == 0 && N >= window_size)
    {
        vector<double> first_ma_rows;
        vector<double> dummy_loyalty;

        readCsvRange(
            filename,
            0,
            window_size + TOP_K - 1,
            first_ma_rows,
            dummy_loyalty,
            false
        );

        if ((long long)first_ma_rows.size() >= window_size)
        {
            timer_start = MPI_Wtime();

            double window_sum = 0.0;

            for (int i = 0; i < window_size; i++)
            {
                window_sum += first_ma_rows[(size_t)i];
            }

            for (int i = 0; i < TOP_K && i + window_size <= (int)first_ma_rows.size(); i++)
            {
                display_moving_average.push_back(window_sum / window_size);

                if (i + window_size < (int)first_ma_rows.size())
                {
                    window_sum += first_ma_rows[(size_t)(i + window_size)];
                    window_sum -= first_ma_rows[(size_t)i];
                }
            }

            timer_end = MPI_Wtime();
            computation_time_ms += (timer_end - timer_start) * 1000.0;
        }
    }

    // =====================================================
    // TASK 6: Outlier Detection using Z-score
    // =====================================================
    double outlier_start = MPI_Wtime();

    double z_threshold = 3.0;
    long long local_outlier_count = 0;

    timer_start = MPI_Wtime();

    for (long long i = 0; i < actual_local_count; i++)
    {
        double z_score = (local_transaction[(size_t)i] - mean) / standard_deviation;

        if (fabs(z_score) > z_threshold)
        {
            local_outlier_count++;
        }
    }

    timer_end = MPI_Wtime();
    computation_time_ms += (timer_end - timer_start) * 1000.0;

    long long global_outlier_count = 0;

    // COMMUNICATION: Reduce outlier count
    timer_start = MPI_Wtime();
    MPI_Reduce(
        &local_outlier_count,
        &global_outlier_count,
        1,
        MPI_LONG_LONG,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );
    timer_end = MPI_Wtime();
    communication_time_ms += (timer_end - timer_start) * 1000.0;

    double outlier_end = MPI_Wtime();
    double outlier_time_ms = (outlier_end - outlier_start) * 1000.0;

    double total_end = MPI_Wtime();
    double total_time_ms = (total_end - total_start) * 1000.0;

    // =====================================================
    // Output only from Rank 0
    // =====================================================
    if (rank == 0)
    {
        cout << fixed << setprecision(6);

        cout << "\n========== MPI TASK 1: BASIC STATISTICS ==========" << endl;
        cout << "Mean                : " << mean << endl;
        cout << "Variance            : " << variance << endl;
        cout << "Standard Deviation  : " << standard_deviation << endl;
        cout << "Minimum Value       : " << global_min << endl;
        cout << "Maximum Value       : " << global_max << endl;

        cout << "\n========== MPI TASK 2: HISTOGRAM GENERATION ==========" << endl;

        for (int i = 0; i < bin_count; i++)
        {
            double lower_bound = range_min + i * bin_width;
            double upper_bound = lower_bound + bin_width;

            cout << "Bin " << i + 1
                << " [" << lower_bound << " - " << upper_bound << "] : "
                << global_histogram[i] << endl;
        }

        cout << "\n========== MPI TASK 3: SORTING ==========" << endl;
        cout << "Each process sorted its own assigned data chunk." << endl;

        cout << "First 5 sorted values: ";
        for (double value : global_first5)
        {
            cout << value << " ";
        }

        cout << "\nLast 5 sorted values : ";
        for (double value : global_last5)
        {
            cout << value << " ";
        }
        cout << endl;

        cout << "\n========== MPI TASK 4: PEARSON CORRELATION ==========" << endl;
        cout << "Pearson Correlation : " << pearson_correlation << endl;

        cout << "\n========== MPI TASK 5: MOVING AVERAGE ==========" << endl;
        cout << "Window Size         : " << window_size << endl;
        cout << "Total MA Values     : " << total_ma_count << endl;
        cout << "First 5 values      : ";

        for (double value : display_moving_average)
        {
            cout << value << " ";
        }

        cout << endl;

        cout << "\n========== MPI TASK 6: OUTLIER DETECTION ==========" << endl;
        cout << "Z-score Threshold   : " << z_threshold << endl;
        cout << "Outlier Count       : " << global_outlier_count << endl;

        cout << "\n========== MPI EXECUTION TIME ==========" << endl;
        cout << "Read Local Data Time          : " << read_time_ms << " ms" << endl;
        cout << "Task 1 - Basic Statistics Time: " << stat_time_ms << " ms" << endl;
        cout << "Task 2 - Histogram Time       : " << histogram_time_ms << " ms" << endl;
        cout << "Task 3 - Sorting Time         : " << sorting_time_ms << " ms" << endl;
        cout << "Task 4 - Correlation Time     : " << correlation_time_ms << " ms" << endl;
        cout << "Task 5 - Moving Average Time  : " << moving_average_time_ms << " ms" << endl;
        cout << "Task 6 - Outlier Detection    : " << outlier_time_ms << " ms" << endl;

        cout << "\n========== EXP-4: COMMUNICATION VS COMPUTATION ==========" << endl;
        cout << "Communication Time            : " << communication_time_ms << " ms" << endl;
        cout << "Computation Time              : " << computation_time_ms << " ms" << endl;

        cout << "\nTotal MPI Execution Time      : " << total_time_ms << " ms" << endl;

        string resultFileName = "C:\\MPIProject\\mpi_analytics\\mpi_results.csv";

        ifstream checkFile(resultFileName);
        bool writeHeader = !checkFile.good() || checkFile.peek() == ifstream::traits_type::eof();
        checkFile.close();

        ofstream resultFile(resultFileName, ios::app);

        if (resultFile.is_open())
        {
            if (writeHeader)
            {
                resultFile << "RunType,DatasetFile,Processes,Records,"
                    << "Mean,Variance,StandardDeviation,Minimum,Maximum,"
                    << "PearsonCorrelation,MovingAverageWindow,OutlierThreshold,OutlierCount,"
                    << "ReadLocalData_ms,"
                    << "Task1_BasicStatistics_ms,Task2_Histogram_ms,Task3_Sorting_ms,"
                    << "Task4_Correlation_ms,Task5_MovingAverage_ms,"
                    << "Task6_OutlierDetection_ms,"
                    << "CommunicationTime_ms,ComputationTime_ms,TotalExecutionTime_ms\n";
            }

            resultFile << fixed << setprecision(6);

            resultFile << "MPI-EXP4,"
                << "\"" << filename << "\"" << ","
                << size << ","
                << N << ","
                << mean << ","
                << variance << ","
                << standard_deviation << ","
                << global_min << ","
                << global_max << ","
                << pearson_correlation << ","
                << window_size << ","
                << z_threshold << ","
                << global_outlier_count << ","
                << read_time_ms << ","
                << stat_time_ms << ","
                << histogram_time_ms << ","
                << sorting_time_ms << ","
                << correlation_time_ms << ","
                << moving_average_time_ms << ","
                << outlier_time_ms << ","
                << communication_time_ms << ","
                << computation_time_ms << ","
                << total_time_ms << "\n";

            resultFile.close();

            cout << "\nResults saved to: " << resultFileName << endl;
        }
        else
        {
            cout << "\nError: Unable to create mpi_results.csv. Close Excel if the file is open." << endl;
        }
    }

    MPI_Finalize();
    return 0;
}