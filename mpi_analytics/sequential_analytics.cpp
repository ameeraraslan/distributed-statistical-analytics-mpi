#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <iomanip>

using namespace std;

int main(int argc, char* argv[])
{
    // =====================================================
    // Check command-line argument
    // =====================================================
    if (argc < 2)
    {
        cout << "Usage: sequential_analytics.exe <csv_filename>" << endl;
        cout << "Example: sequential_analytics.exe \"C:\\MPIProject\\dataset\\ecommerce_small_1million.csv\"" << endl;
        return 1;
    }

    string filename = argv[1];

    cout << "Sequential E-commerce Data Analytics Program" << endl;
    cout << "Dataset file: " << filename << endl;

    auto total_start = chrono::high_resolution_clock::now();

    // =====================================================
    // Read Dataset from CSV
    // =====================================================
    vector<double> data;
    vector<double> data2;

    ifstream file(filename);

    if (!file.is_open())
    {
        cout << "Error: Cannot open file " << filename << endl;
        return 1;
    }

    string line;

    // Skip CSV header
    getline(file, line);

    while (getline(file, line))
    {
        stringstream ss(line);
        string transaction_str, loyalty_str;

        getline(ss, transaction_str, ',');
        getline(ss, loyalty_str, ',');

        if (!transaction_str.empty() && !loyalty_str.empty())
        {
            double transactionAmount = stod(transaction_str);
            double loyaltyPoints = stod(loyalty_str);

            data.push_back(transactionAmount);
            data2.push_back(loyaltyPoints);
        }
    }

    file.close();

    long long N = data.size();

    if (N <= 0)
    {
        cout << "Error: Dataset is empty or invalid." << endl;
        return 1;
    }

    cout << "Total records loaded: " << N << endl;
    cout << fixed << setprecision(6);

    // =====================================================
    // Task 1: Basic Statistics
    // =====================================================
    auto stat_start = chrono::high_resolution_clock::now();

    double sum = 0.0;
    double min_value = data[0];
    double max_value = data[0];

    for (long long i = 0; i < N; i++)
    {
        sum += data[i];

        if (data[i] < min_value)
        {
            min_value = data[i];
        }

        if (data[i] > max_value)
        {
            max_value = data[i];
        }
    }

    double mean = sum / N;

    double variance_sum = 0.0;

    for (long long i = 0; i < N; i++)
    {
        double difference = data[i] - mean;
        variance_sum += difference * difference;
    }

    double variance = variance_sum / N;
    double standard_deviation = sqrt(variance);

    auto stat_end = chrono::high_resolution_clock::now();

    double stat_time_ms =
        chrono::duration<double, milli>(stat_end - stat_start).count();

    // =====================================================
    // Task 2: Histogram Generation
    // =====================================================
    auto histogram_start = chrono::high_resolution_clock::now();

    int bin_count = 10;
    vector<long long> histogram(bin_count, 0);

    double range_min = 0.0;
    double range_max = 10000.0;
    double bin_width = (range_max - range_min) / bin_count;

    for (long long i = 0; i < N; i++)
    {
        int bin_index = static_cast<int>((data[i] - range_min) / bin_width);

        if (bin_index >= bin_count)
        {
            bin_index = bin_count - 1;
        }

        if (bin_index < 0)
        {
            bin_index = 0;
        }

        histogram[bin_index]++;
    }

    auto histogram_end = chrono::high_resolution_clock::now();

    double histogram_time_ms =
        chrono::duration<double, milli>(histogram_end - histogram_start).count();

    // =====================================================
    // Task 3: Sorting
    // =====================================================
    auto sorting_start = chrono::high_resolution_clock::now();

    vector<double> sorted_data = data;
    sort(sorted_data.begin(), sorted_data.end());

    auto sorting_end = chrono::high_resolution_clock::now();

    double sorting_time_ms =
        chrono::duration<double, milli>(sorting_end - sorting_start).count();

    // =====================================================
    // Task 4: Pearson Correlation
    // =====================================================
    auto correlation_start = chrono::high_resolution_clock::now();

    double sum_x = 0.0;
    double sum_y = 0.0;

    for (long long i = 0; i < N; i++)
    {
        sum_x += data[i];
        sum_y += data2[i];
    }

    double mean_x = sum_x / N;
    double mean_y = sum_y / N;

    double numerator = 0.0;
    double denominator_x = 0.0;
    double denominator_y = 0.0;

    for (long long i = 0; i < N; i++)
    {
        double x_diff = data[i] - mean_x;
        double y_diff = data2[i] - mean_y;

        numerator += x_diff * y_diff;
        denominator_x += x_diff * x_diff;
        denominator_y += y_diff * y_diff;
    }

    double pearson_correlation = numerator / sqrt(denominator_x * denominator_y);

    auto correlation_end = chrono::high_resolution_clock::now();

    double correlation_time_ms =
        chrono::duration<double, milli>(correlation_end - correlation_start).count();

    // =====================================================
    // Task 5: Moving Average
    // =====================================================
    auto moving_average_start = chrono::high_resolution_clock::now();

    int window_size = 5;
    vector<double> moving_average;

    if (N >= window_size)
    {
        moving_average.resize(N - window_size + 1);

        double window_sum = 0.0;

        for (int i = 0; i < window_size; i++)
        {
            window_sum += data[i];
        }

        moving_average[0] = window_sum / window_size;

        for (long long i = window_size; i < N; i++)
        {
            window_sum += data[i];
            window_sum -= data[i - window_size];

            moving_average[i - window_size + 1] = window_sum / window_size;
        }
    }

    auto moving_average_end = chrono::high_resolution_clock::now();

    double moving_average_time_ms =
        chrono::duration<double, milli>(moving_average_end - moving_average_start).count();

    // =====================================================
    // Task 6: Outlier Detection using Z-score
    // =====================================================
    auto outlier_start = chrono::high_resolution_clock::now();

    long long outlier_count = 0;
    double z_threshold = 3.0;

    for (long long i = 0; i < N; i++)
    {
        double z_score = (data[i] - mean) / standard_deviation;

        if (fabs(z_score) > z_threshold)
        {
            outlier_count++;
        }
    }

    auto outlier_end = chrono::high_resolution_clock::now();

    double outlier_time_ms =
        chrono::duration<double, milli>(outlier_end - outlier_start).count();

    // =====================================================
    // Total Execution Time
    // =====================================================
    auto total_end = chrono::high_resolution_clock::now();

    double total_time_ms =
        chrono::duration<double, milli>(total_end - total_start).count();

    // =====================================================
    // Output: Task 1
    // =====================================================
    cout << "\n========== SEQUENTIAL TASK 1: BASIC STATISTICS ==========" << endl;
    cout << "Mean                : " << mean << endl;
    cout << "Variance            : " << variance << endl;
    cout << "Standard Deviation  : " << standard_deviation << endl;
    cout << "Minimum Value       : " << min_value << endl;
    cout << "Maximum Value       : " << max_value << endl;

    // =====================================================
    // Output: Task 2
    // =====================================================
    cout << "\n========== SEQUENTIAL TASK 2: HISTOGRAM GENERATION ==========" << endl;

    for (int i = 0; i < bin_count; i++)
    {
        double lower_bound = range_min + i * bin_width;
        double upper_bound = lower_bound + bin_width;

        cout << "Bin " << i + 1
            << " [" << lower_bound << " - " << upper_bound << "] : "
            << histogram[i] << endl;
    }

    // =====================================================
    // Output: Task 3
    // =====================================================
    cout << "\n========== SEQUENTIAL TASK 3: SORTING ==========" << endl;

    cout << "First 5 sorted values: ";
    for (int i = 0; i < 5 && i < N; i++)
    {
        cout << sorted_data[i] << " ";
    }

    cout << "\nLast 5 sorted values : ";
    for (long long i = max(0LL, N - 5); i < N; i++)
    {
        cout << sorted_data[i] << " ";
    }
    cout << endl;

    // =====================================================
    // Output: Task 4
    // =====================================================
    cout << "\n========== SEQUENTIAL TASK 4: PEARSON CORRELATION ==========" << endl;
    cout << "Pearson Correlation : " << pearson_correlation << endl;

    // =====================================================
    // Output: Task 5
    // =====================================================
    cout << "\n========== SEQUENTIAL TASK 5: MOVING AVERAGE ==========" << endl;

    if (!moving_average.empty())
    {
        cout << "Window Size         : " << window_size << endl;
        cout << "First 5 values      : ";

        for (int i = 0; i < 5 && i < moving_average.size(); i++)
        {
            cout << moving_average[i] << " ";
        }

        cout << endl;
    }
    else
    {
        cout << "Dataset is smaller than moving average window size." << endl;
    }

    // =====================================================
    // Output: Task 6
    // =====================================================
    cout << "\n========== SEQUENTIAL TASK 6: OUTLIER DETECTION ==========" << endl;
    cout << "Z-score Threshold   : " << z_threshold << endl;
    cout << "Outlier Count       : " << outlier_count << endl;

    // =====================================================
    // Output: Execution Time
    // =====================================================
    cout << "\n========== SEQUENTIAL EXECUTION TIME ==========" << endl;
    cout << "Task 1 - Basic Statistics Time : " << stat_time_ms << " ms" << endl;
    cout << "Task 2 - Histogram Time        : " << histogram_time_ms << " ms" << endl;
    cout << "Task 3 - Sorting Time          : " << sorting_time_ms << " ms" << endl;
    cout << "Task 4 - Correlation Time      : " << correlation_time_ms << " ms" << endl;
    cout << "Task 5 - Moving Average Time   : " << moving_average_time_ms << " ms" << endl;
    cout << "Task 6 - Outlier Detection Time: " << outlier_time_ms << " ms" << endl;
    cout << "Total Sequential Execution Time: " << total_time_ms << " ms" << endl;

    // =====================================================
    // Save Sequential results to CSV file for Excel
    // =====================================================
    string resultFileName = "C:\\MPIProject\\mpi_analytics\\sequential_results.csv";

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
                << "Task1_BasicStatistics_ms,Task2_Histogram_ms,Task3_Sorting_ms,"
                << "Task4_Correlation_ms,Task5_MovingAverage_ms,"
                << "Task6_OutlierDetection_ms,TotalExecutionTime_ms\n";
        }

        resultFile << fixed << setprecision(6);

        resultFile << "Sequential,"
            << "\"" << filename << "\"" << ","
            << 1 << ","
            << N << ","
            << mean << ","
            << variance << ","
            << standard_deviation << ","
            << min_value << ","
            << max_value << ","
            << pearson_correlation << ","
            << window_size << ","
            << z_threshold << ","
            << outlier_count << ","
            << stat_time_ms << ","
            << histogram_time_ms << ","
            << sorting_time_ms << ","
            << correlation_time_ms << ","
            << moving_average_time_ms << ","
            << outlier_time_ms << ","
            << total_time_ms << "\n";

        resultFile.close();

        cout << "\nResults saved to: " << resultFileName << endl;
    }
    else
    {
        cout << "\nError: Unable to create sequential_results.csv" << endl;
    }

    return 0;
}