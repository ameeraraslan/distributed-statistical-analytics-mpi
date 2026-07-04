// EcommerceDatasetGenerator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <random>
#include <iomanip>
#include <string>
#include <chrono>

void generateDataset(const std::string& filename, size_t N)
{
    // Fixed seed for reproducibility
    std::mt19937_64 rng(42);

    // Random transaction amount between RM0.00 and RM10,000.00
    std::uniform_real_distribution<double> amountDist(0.0, 10000.0);

    // Random noise for loyalty points
    std::uniform_real_distribution<double> noiseDist(0.0, 100.0);

    std::ofstream file(filename);

    if (!file.is_open())
    {
        std::cout << "Error: Cannot create file.\n";
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // CSV header
    file << "transactionAmount,loyaltyPoints\n";

    std::cout << "\nGenerating dataset...\n";
    std::cout << "File name: " << filename << "\n";
    std::cout << "Total records: " << N << "\n\n";

    size_t progressStep = N / 10;

    for (size_t i = 0; i < N; i++)
    {
        double transactionAmount = amountDist(rng);

        // Loyalty points are generated based on transaction amount
        // This makes the second column suitable for Pearson correlation
        double loyaltyPoints = (transactionAmount * 0.1) + noiseDist(rng);

        file << std::fixed << std::setprecision(2)
            << transactionAmount << ","
            << loyaltyPoints << "\n";

        // Show first five records on screen
        if (i < 5)
        {
            std::cout << "Record " << (i + 1)
                << " -> Transaction Amount: RM " << std::fixed << std::setprecision(2)
                << transactionAmount
                << ", Loyalty Points: " << loyaltyPoints << "\n";
        }

        // Show progress every 10%
        if (progressStep != 0 && (i + 1) % progressStep == 0)
        {
            int percent = static_cast<int>(((i + 1) * 100) / N);
            std::cout << "Progress: " << percent << "% completed\n";
        }
    }

    file.close();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "\nDataset generated successfully.\n";
    std::cout << "Saved as: " << filename << "\n";
    std::cout << "Total records generated: " << N << "\n";
    std::cout << "Generation time: " << elapsed.count() << " seconds\n";
}

int main()
{
    int choice;

    std::cout << "=============================================\n";
    std::cout << " Synthetic E-commerce Dataset Generator\n";
    std::cout << "=============================================\n";
    std::cout << "Each record contains:\n";
    std::cout << "1. transactionAmount  - simulated e-commerce transaction value\n";
    std::cout << "2. loyaltyPoints      - simulated customer loyalty points\n\n";

    std::cout << "Choose dataset size:\n";
    std::cout << "1. Small  - 1,000,000 records\n";
    std::cout << "2. Medium - 10,000,000 records\n";
    std::cout << "3. Large  - 100,000,000 records\n";
    std::cout << "4. Generate all three datasets\n";
    std::cout << "Enter your choice: ";
    std::cin >> choice;

    if (choice == 1)
    {
        generateDataset("ecommerce_small_1million.csv", 1000000);
    }
    else if (choice == 2)
    {
        generateDataset("ecommerce_medium_10million.csv", 10000000);
    }
    else if (choice == 3)
    {
        generateDataset("ecommerce_large_100million.csv", 100000000);
    }
    else if (choice == 4)
    {
        generateDataset("ecommerce_small_1million.csv", 1000000);
        generateDataset("ecommerce_medium_10million.csv", 10000000);
        generateDataset("ecommerce_large_100million.csv", 100000000);
    }
    else
    {
        std::cout << "Invalid choice. Please run the program again.\n";
    }

    std::cout << "\nProgram finished.\n";

    return 0;
}