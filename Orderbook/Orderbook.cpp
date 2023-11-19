
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
    std::vector<Order> samePriceOrders; // Vector of quantity of orders with the same price
    
};

struct PriceLevel { // orders at the same specific price
    int quantity;
    int orderId;
    //std::vector<Order> order;
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
    // Key: Price, Value: Orders that also contain orders of the same price (unprocessed) <=> Price Level
    std::map<double, Order, CompareBids> bids; // Buy orders
    std::map<double, Order, CompareAsks> asks; // Sell orders

public:
    void processOrder(const Order& order) {
        if (order.side == "BUY") {
            // NEW, CANCEL, TRADE (Modify) cases
            if (order.category == "NEW") {
                // Case #1: Order at that price doesn't exist in the order book (BID side)
                if (bids.find(order.price) == bids.end()) {
                    // Add the order price as key, and the quantity to the samePriceOrders vector too
                    bids[order.price] = order;
                    bids[order.price].samePriceOrders.push_back(order);
                }
                // Case #2: Order at that price already exists in the order book, so add it to the samePriceOrders vector
                else {
                    // Update the quantity of the order too
                    bids[order.price].quantity += order.quantity;
                    bids[order.price].samePriceOrders.push_back(order);
                }
            }

            else if (order.category == "CANCEL") {
                // Cancel the order at that price by checking orderId with the samePriceOrders vector orderIds, if after cancelling the order (deleting it), the size of the vector is 0, then delete the order at that price from the order book
                // If the order doesn't exist in the order book, then throw an error
                if (bids.find(order.price) != bids.end()) {
					// Check if the orderId exists in the samePriceOrders vector
					auto it = std::find_if(bids[order.price].samePriceOrders.begin(), bids[order.price].samePriceOrders.end(), [&order](const Order& o) {return o.orderId == order.orderId; });
					if (it != bids[order.price].samePriceOrders.end()) {
                        // Update the price by substracting the quantity of the order that will be deleted from the quantity of the order at that price
                        bids[order.price].quantity -= it->quantity;
						// Remove the order from the samePriceOrders vector
						bids[order.price].samePriceOrders.erase(it);
						// Check if the size of the samePriceOrders vector is 0, if it is, then remove the order at that price from the order book
						if (bids[order.price].samePriceOrders.size() == 0) {
							bids.erase(order.price);
						}
					}
					else {
						std::cout << "Order doesn't exist in the order book" << std::endl;
					}
				}
				else {
					std::cout << "Order doesn't exist in the order book" << std::endl;
				}
            }

            else if (order.category == "TRADE") {
                // Check if the order exists in the order book by checking the order price (key of the map)
                // if it does, and there are enough quantity at that price, then update the quantity (subtract the traded quantity)
                // and go through the samePriceOrders vector and subtract the traded quantity from the first element of the vector to the last accordingly
                // if the quantity at that price in samePriceOrders vector reaches 0, then remove the price level from the order book.
                // if the quantity at that price in samePriceOrders vector is less than 0, then throw an error

                if (bids.find(order.price) != bids.end()) {
                    // Check if there are enough quantity at that price
                    if (bids[order.price].quantity >= order.quantity) {
                        // Update the quantity
                        bids[order.price].quantity -= order.quantity;

                        int removeQuantity = order.quantity;

                        for (auto it = bids[order.price].samePriceOrders.begin(); it != bids[order.price].samePriceOrders.end();) {
                            if (it->quantity >= removeQuantity) {
                                it->quantity -= removeQuantity;
                                // Remove the order from the order book if the quantity reaches 0 <=> the order has been traded/processed
                                if (it->quantity == 0) {
                                    bids[order.price].samePriceOrders.erase(it);
                                }
                                if (bids[order.price].samePriceOrders.size() == 0) {
                                    bids.erase(order.price);
                                }
                                break; // found enough quantity of orders to subtract from (ie: to trade)
                            }
                            else {
                                removeQuantity -= it->quantity; // Update the removeQuantity to subtract from the next element in the vector
                                it = bids[order.price].samePriceOrders.erase(it); // Remove the element from the vector
                                // if size of vector is 0, then remove the order at that price from the order book
                                if (bids[order.price].samePriceOrders.size() == 0) {
									bids.erase(order.price);
								}
                            }
                        }

                    }
                    else {
                        std::cout << "Not enough quantity at that price" << std::endl;
                    }
                }
                else {
                    std::cout << "Order doesn't exist in the order book" << std::endl;
                };
            }
        }
        else if (order.side == "SELL") {
            // NEW, CANCEL, TRADE (Modify) cases
            if (order.category == "NEW") {
                // Case #1: Order at that price doesn't exist in the order book (BID side)
                if (asks.find(order.price) == asks.end()) {
                    // Add the order price as key, and the quantity to the samePriceOrders vector too
                    asks[order.price] = order;
                    asks[order.price].samePriceOrders.push_back(order);
                }

                // Case #2: Order at that price already exists in the order book, so add it to the samePriceOrders vector
                else {
                    // Update the quantity of the order too
                    asks[order.price].quantity += order.quantity;
                    asks[order.price].samePriceOrders.push_back(order);
                }
            }

            else if (order.category == "CANCEL") {
                // Cancel the order at that price by checking orderId with the samePriceOrders vector orderIds, if after cancelling the order (deleting it), the size of the vector is 0, then delete the order at that price from the order book
                // If the order doesn't exist in the order book, then throw an error
                if (asks.find(order.price) != asks.end()) {
                    // Check if the orderId exists in the samePriceOrders vector
                    auto it = std::find_if(asks[order.price].samePriceOrders.begin(), asks[order.price].samePriceOrders.end(), [&order](const Order& o) {return o.orderId == order.orderId; });
                    if (it != asks[order.price].samePriceOrders.end()) {
         
                        // Update the price by substracting the quantity of the order that will be deleted from the quantity of the order at that price
                        asks[order.price].quantity -= it->quantity;
                        // Remove the order from the samePriceOrders vector
                        asks[order.price].samePriceOrders.erase(it);
                        // Check if the size of the samePriceOrders vector is 0, if it is, then remove the order at that price from the order book
                        if (asks[order.price].samePriceOrders.size() == 0) {
                            asks.erase(order.price);
                        }
                    }
                    else {
                        std::cout << "Order doesn't exist in the order book" << std::endl;
                    }
                }
                else {
                    std::cout << "Order doesn't exist in the order book" << std::endl;
                }
            }
            else if (order.category == "TRADE") {
                // Check if the order exists in the order book by checking the order price (key of the map)
                // if it does, and there are enough quantity at that price, then update the quantity (subtract the traded quantity)
                // and go through the samePriceOrders vector and subtract the traded quantity from the first element of the vector to the last accordingly
                // if the quantity at that price in samePriceOrders vector reaches 0, then remove the price level from the order book.
                // if the quantity at that price in samePriceOrders vector is less than 0, then throw an error
                if (asks.find(order.price) != asks.end()) {
                    // Check if there are enough quantity at that price
                    if (asks[order.price].quantity >= order.quantity) {
                        // Update the quantity
                        asks[order.price].quantity -= order.quantity;

                        int removeQuantity = order.quantity;

                        for (auto it = asks[order.price].samePriceOrders.begin(); it != asks[order.price].samePriceOrders.end();) {
                            if (it->quantity >= removeQuantity) {
                                it->quantity -= removeQuantity;
                                // Remove the order from the order book if the quantity reaches 0 <=> the order has been traded/processed
                                if (it->quantity == 0) {
                                    asks[order.price].samePriceOrders.erase(it);
                                }
                                // if size of vector is 0, then remove the order at that price from the order book
                                if (asks[order.price].samePriceOrders.size() == 0) {
                                    asks.erase(order.price);
                                }
                                break; // found enough quantity of orders to subtract from (ie: to trade)
                            }
                            else {
                                removeQuantity -= it->quantity; // Update the removeQuantity to subtract from the next element in the vector
                                it = asks[order.price].samePriceOrders.erase(it); // Remove the element from the vector
                                // if size of vector is 0, then remove the order at that price from the order book
                                if (asks[order.price].samePriceOrders.size() == 0) {
                                    asks.erase(order.price);
                                }
                            }
                        }
                    }
                    else {
                        std::cout << "Not enough quantity at that price" << std::endl;
                    }
                }
                else {
                    std::cout << "Order doesn't exist in the order book" << std::endl;
                };
            }
           
        }
    }

    void printOrderBook() {
        // Print the order book of each side, and their samePriceOrders vectors
        std::cout << "BID SIDE" << std::endl;
        for (auto& bid : bids) {
            std::cout << "Price: " << bid.first << ", Quantity: " << bid.second.quantity;
            std::cout << " Same Price Orders: ";
            std::cout << "[";
            for (auto& samePriceOrder : bid.second.samePriceOrders) {
                std::cout << samePriceOrder.quantity << " (" << "id:" << samePriceOrder.orderId << "), ";
            }
            std::cout << " ]";
            std::cout << std::endl;
        }

        std::cout << "\n";

        std::cout << "ASK SIDE" << std::endl;
        for (auto& ask : asks) {
            std::cout << "Price: " << ask.first << ", Quantity: " << ask.second.quantity;
            std::cout << " Same Price Orders: ";
            std::cout << "[";
            for (auto& samePriceOrder : ask.second.samePriceOrders) {
                std::cout << samePriceOrder.quantity << " (" << "id:" << samePriceOrder.orderId << "), ";
            }
            std::cout << " ]";
            std::cout << std::endl;
        }
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
}

int main() {

    // file path
    std::string filePath = "SCH.log";
    ReadFile(filePath); // Uncomment this line to read the file

    // Code below is for testing purposes:
    //OrderBook orderBook;

    //Order order1 = { 1, 1, "SCH", "BUY", "NEW", 9.6, 4 };
    //Order order2 = { 2, 2, "SCH", "BUY", "NEW", 9.5, 6 };
    //Order order3 = { 4, 8, "SCH", "SELL", "NEW", 9.7, 5 };
    //Order order4 = { 5, 2, "SCH", "SELL", "NEW", 9.7, 10 };
    //// 

  
    //orderBook.processOrder(order1);
    //orderBook.processOrder(order2);
    //orderBook.processOrder(order3);
    //orderBook.processOrder(order4);

    //
    //orderBook.printOrderBook();


    //std::cout << "\n\n\n\n";
    //// Trade an order
    //Order order5 = { 6, 2, "SCH", "SELL", "CANCEL", 9.7, 4 };
    //// 
    ////Order order5 = { 6, 8, "SCH", "SELL", "TRADE", 9.7, 4 };
    //Order order6 = { 7, 1, "SCH", "BUY", "TRADE", 9.6, 4 };
   
    ///* Order order7 = { 8, 5, "SCH", "SELL", "TRADE", 9.7, 10 };
    //Order order8 = { 9, 5, "SCH", "SELL", "TRADE", 9.7, 25 };*/

    //orderBook.processOrder(order5);
    //orderBook.processOrder(order6);
    //orderBook.printOrderBook();

	return 0;
}
