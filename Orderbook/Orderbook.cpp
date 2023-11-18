
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>




int main() {

    std::ifstream file("SCH.log"); // Replace "your_file.csv" with your file name
    if (!file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return 1;
    }

    int linesToRead = 10; // REMOVE: Read only 10 lines for now for testing purposes

    std::string line;
    while (std::getline(file, line) && linesToRead > 0)  // REMOVE: linesToRead > 0 to read the entire file
    { 
        std::vector<std::string> tokens;
        std::istringstream tokenStream(line);
        std::string token;

        while (tokenStream >> token) { // Using space (' ') as the delimiter
            tokens.push_back(token);
        }

        // Accessing individual fields
        if (tokens.size() >= 7) { // Ensure at least 7 fields are present
            std::string epoch = tokens[0];
            std::string orderId = tokens[1];
            std::string symbol = tokens[2];
            std::string orderSide = tokens[3];
            std::string orderCategory = tokens[4];
            std::string price = tokens[5];
            std::string quantity = tokens[6];

            // Process or use the extracted data as needed
            // Example: Print the fields
            std::cout << "Epoch: " << epoch << ", Order ID: " << orderId << ", Symbol: " << symbol
                << ", Order Side: " << orderSide << ", Order Category: " << orderCategory
                << ", Price: " << price << ", Quantity: " << quantity << std::endl;
        }
        else {
            std::cerr << "Invalid number of fields in line: " << line << std::endl;
        }

        linesToRead--;
    }

    file.close();
 
    return 0;
    
}
