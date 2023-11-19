
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <map>
#include <algorithm>

struct Order {
    int64_t timestamp;
    int64_t orderId;
    std::string symbol; // AAPL, GOOG, MSFT, etc. (SCS, SCH in this case)
    std::string side; // BUY, SELL
    std::string category; // NEW, CANCEL, TRADE (Modify)
    double price; 
    int quantity;
    std::vector<int> samePriceOrders; // Vector of quantity of orders with the same price
};


// Custom comparator to sort the order book on the BID side in descending order
struct CompareBids {
    bool operator()(const double& lhs, const double& rhs) const {
        return lhs > rhs;
    }
};

// Custom comparator to sort the order book on the ASK side in ascending order
struct CompareAsks {
    bool operator()(const double& lhs, const double& rhs) const {
        return lhs < rhs;
    }
};

class OrderBook
{
private:
    // Key: Price, Value: Vector of Orders (same price) <=> Price Level
    std::map<double, Order> bids; // Buy orders
    std::map<double, Order> asks; // Sell orders

public:
    void processOrder(const Order& order) {
        
        if (order.side == "BUY") {
            // NEW, CANCEL, TRADE (Modify) cases
            if (order.category == "NEW") {
                // Case #1: Order at that price doesn't exist in the order book (BID side)
             
                // Case #2: Order at that price already exists in the order book, so add it to the samePriceOrders vector 
            }

            else if (order.category == "CANCEL") {
                // Add code
            }
            else if (order.category == "TRADE") {
                // Add code                                  
            }
        }
        else if (order.side == "SELL") {
            // NEW, CANCEL, TRADE (Modify) cases
            if (order.category == "NEW") {
                // Case #1: Order at that price doesn't exist in the order book (BID side)

                // Case #2: Order at that price already exists in the order book, so add it to the samePriceOrders vector 
            }

            else if (order.category == "CANCEL") {
                // Add code
            }
            else if (order.category == "TRADE") {
                // Add code                                  
            }
           
        }
    }


    void printOrderBook() {
        // Add code
    }
};


void ReadFile(const std::string& filePath) 
{
    std::ifstream file(filePath); // Replace "your_file.csv" with your file name
    if (!file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        //return 1; // uncomment this line if you want to exit the function on error
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
            /*std::cout << "Epoch: " << epoch << ", Order ID: " << orderId << ", Symbol: " << symbol
                << ", Order Side: " << orderSide << ", Order Category: " << orderCategory
                << ", Price: " << price << ", Quantity: " << quantity << std::endl;*/
        }
        else {
            std::cerr << "Invalid number of fields in line: " << line << std::endl;
        }

        linesToRead--;
    }

    file.close();
}

int main() {

    // file path
    std::string filePath = "SCH.log";
    ReadFile(filePath);

	return 0;
}
