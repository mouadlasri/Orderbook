﻿
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <map>
#include <algorithm>
#include <iomanip>

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

struct Snapshot {
    int64_t timestamp;
    std::string symbol;
    std::vector<std::string> bidSnapshots;
    std::vector<std::string> askSnapshots;
};

class OrderBook
{
private:
    // Key: Price, Value: Orders that also contain orders of the same price (unprocessed) <=> Price Level
    std::map<double, Order, CompareBids> bids; // Buy orders
    std::map<double, Order, CompareAsks> asks; // Sell orders

    // Create a vector of snapshots that we will output to a file later after processing the whole order book
    std::vector<Snapshot> snapshots;
    
 public:
    // Process the order and update the order book accordingly
    void processOrder(const Order& order, std::string symbol, int64_t snapshotStartTime = 0, int64_t snapshotEndTime = 0, int numberOfFields = 5) {
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

        // Add snapshot of the order book to a vector of snapshots
        // Check if snapshot is between startTime and endTime
        if (order.timestamp >= snapshotStartTime && order.timestamp <= snapshotEndTime) {
            // Create a snapshot of the current timestamp, and symbol
            Snapshot newSnapshot;
            newSnapshot.timestamp = order.timestamp;
            newSnapshot.symbol = symbol;

            // Get top n bids and asks if they exist (ie: get the first N elements of the bids and asks if they exist)
            getTopBids(newSnapshot, numberOfFields);
            getTopAsks(newSnapshot, numberOfFields);

            // Add the snapshot to the vector of snapshots
            snapshots.push_back(newSnapshot);
        }
    }

    void printOrderBook() {
        // Print the order book of each side, and their samePriceOrders vectors
        std::cout << "BID SIDE" << std::endl;
        for (auto& bid : bids) {
            std::cout << "Price: " << bid.first << ", Quantity: " << bid.second.quantity;
            std::cout << ", Orders: ";
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
            std::cout << ", Orders: ";
            std::cout << "[";
            for (auto& samePriceOrder : ask.second.samePriceOrders) {
                std::cout << samePriceOrder.quantity << " (" << "id:" << samePriceOrder.orderId << "), ";
            }
            std::cout << " ]";
            std::cout << std::endl;
        }
    }

    void printSnapshots() {
        // Print the snapshots
        for (auto& snapshot : snapshots) {
            std::cout << snapshot.symbol << ", " << snapshot.timestamp << ", ";
            for (auto& bid : snapshot.bidSnapshots) {
                std::cout << bid << " ";
            }
            std::cout << "X ";
            
            for (auto& ask : snapshot.askSnapshots) {
                std::cout << ask << " ";
            }
            std::cout << std::endl;
        }
    }

    // Get the top n bids from the order book. Used for snapshots
    void getTopBids(Snapshot& snapshot, int numberOfFields) {
        // Get the top n bids from the bids map (first n elements of the map if they exist, otherwise the minimum number of elements that exist)
        int numElements = std::min(static_cast<int>(bids.size()), numberOfFields);

        // Get the first n elements of the map and add them to the snapshot
        for (auto it = bids.begin(); it != bids.end(); ++it) {
            std::ostringstream oss;
            oss <<  std::setprecision(5) << it->first;
            std::string priceString = oss.str();
            // Add the snapshot to the vector of bid snapshots
			snapshot.bidSnapshots.push_back(std::to_string(it->second.quantity) + "@" + priceString);
            // break if we reach numElements
            if (--numElements == 0) {
				break;
			}
		}
        std::reverse(snapshot.bidSnapshots.begin(), snapshot.bidSnapshots.end());
    }

    // Get the top n asks from the order book. Used for snapshots
    void getTopAsks(Snapshot& snapshot, int numberOfFields) {
        // Get the top n bids from the bids map (first n elements of the map if they exist, otherwise the minimum number of elements that exist)
        int numElements = std::min(static_cast<int>(asks.size()), numberOfFields);

        // Get the first n elements of the map and add them to the snapshot
        for (auto it = asks.begin(); it != asks.end(); ++it) {
            std::ostringstream oss;
            oss << std::setprecision(5) << it->first;
            std::string priceString = oss.str();
            // Add the snapshot to the vector of ask snapshots
            snapshot.askSnapshots.push_back(std::to_string(it->second.quantity) + "@" + priceString);
            // break if we reach numElements
            if (--numElements == 0) {
                break;
            }
        }
    }

    void saveSnapshotsTimeToFile(bool singleSnapshot) {
        // Save the snapshots to a file
        std::ofstream snapshotsFile("snapshots.txt");
        if (!snapshotsFile.is_open()) {
            std::cerr << "Unable to open file for writing!" << std::endl;
            return;
        }

            // If we only want to output the last snapshot at snapshotEndTime (only one time is given which is the end time)
        if (singleSnapshot) {
			// Get the last snapshot
			Snapshot snapshot = snapshots.back();
			snapshotsFile << snapshot.symbol << ", " << snapshot.timestamp << ", ";
			for (auto& bid : snapshot.bidSnapshots) {
				snapshotsFile << bid << " ";
			}
			snapshotsFile << "X ";

			for (auto& ask : snapshot.askSnapshots) {
				snapshotsFile << ask << "  ";
			}
			snapshotsFile << std::endl;
		}
		else { // if we want to output a range of snapshots from snapshotStartTime to snapshotEndTime
			for (const auto& snapshot : snapshots) {
				snapshotsFile << snapshot.symbol << ", " << snapshot.timestamp << ", ";
				for (auto& bid : snapshot.bidSnapshots) {
					snapshotsFile << bid << " ";
				}
				snapshotsFile << "X ";

				for (auto& ask : snapshot.askSnapshots) {
					snapshotsFile << ask << "  ";
				}
				snapshotsFile << std::endl;
			}
		}

        snapshotsFile.close();
    }
};


void ReadFile(const std::string& filePath, const std::string& symbol, int64_t snapshotStartTime=0, int64_t snapshotEndTime=0)
{
    OrderBook orderbook;
    int numberOfFields = 5; // can be modified to get more or less fields for the snapshots
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Unable to open file!" << std::endl;
        //return 1;
    }

    std::vector<Order> orders;
    std::string line;

    //int linesRead = 0; // testing purposes

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        Order order;

        if (!(iss >> order.timestamp >> order.orderId >> order.symbol >> order.side >> order.category >> order.price >> order.quantity)) {
            std::cerr << "Error reading line!" << std::endl;
            continue;
        }

        // check the timestamp of the order,if it's smaller than end time, then process the order.
        // Inside the process order, it will check the start time too , and if it's between start and end time, then it will add it to the snapshots vector
        if (order.timestamp <= snapshotEndTime) {
			orderbook.processOrder(order, symbol, snapshotStartTime, snapshotEndTime);
		}

        //orders.push_back(order);

        //linesRead++;
    }

    file.close();

    for (const auto& order : orders) {
        std::cout << "Epoch: " << order.timestamp << ", Order ID: " << order.orderId << ", Symbol: " << order.symbol
            << ", Side: " << order.side << ", Category: " << order.category << ", Price: " << order.price
            << ", Quantity: " << order.quantity << std::endl;
    }

    orderbook.printOrderBook();
    std::cout << "\n\n";
    orderbook.printSnapshots();
    if (snapshotStartTime == 0) { // if we want to output the last snapshot at snapshotEndTime (only one time is given which is the end time)
        orderbook.saveSnapshotsTimeToFile(true); 
    }
    else { // if we want to output a range of snapshots from snapshotStartTime to snapshotEndTime
        orderbook.saveSnapshotsTimeToFile(false);
    }
}

// function to save orders to a binary file (for faster reading)

void saveOrdersToBinary(const std::string& txtFilename, const std::string& binFilename) {
    std::ifstream file(txtFilename);
    if (!file.is_open()) {
        std::cerr << "Unable to open file!" << std::endl;
        return;
    }


    std::ofstream binaryFile(binFilename, std::ios::binary);
    if (!binaryFile.is_open()) {
        std::cerr << "Unable to open binary file for writing!" << std::endl;
        return;
    }

    std::string line;
    int ordersRead = 0; // testing purposes
    int limitRead = 10; // testing purposes
    while (std::getline(file, line) && ordersRead < limitRead) {
        std::istringstream iss(line);
        Order order;

        if (!(iss >> order.timestamp >> order.orderId >> order.symbol >> order.side >> order.category >> order.price >> order.quantity)) {
            std::cerr << "Error reading line!" << std::endl;
            continue;
        }

        binaryFile.write(reinterpret_cast<const char*>(&order), sizeof(Order));
        ordersRead++;
    }

    file.close();
    binaryFile.close();
}

// function to read orders from a binary file (for faster reading) and print them
// not working properly
void readOrdersFromBinary(const std::string& binFilename) { 
   std::ifstream binaryFile(binFilename, std::ios::binary);
    if (!binaryFile.is_open()) {
        std::cerr << "Unable to open binary file for reading!" << std::endl;
        return;
    }

    Order order;
    while (binaryFile.read(reinterpret_cast<char*>(&order), sizeof(Order))) {
        std::cout << "Epoch: " << order.timestamp << ", Order ID: " << order.orderId
            << ", Symbol: " << order.symbol << ", Side: " << order.side
            << ", Category: " << order.category << ", Price: " << order.price
            << ", Quantity: " << order.quantity << std::endl;
    }

    binaryFile.close();
}

void getSnapshotInTimeRange(const std::string& filePath, const std::string& symbol, int64_t startSnapshotTime=0, int64_t endSnapshotTime=0) {
	// Read the file and process the orders, then print and store the snapshots in file
    // Then we'll print the snapshots vector
    ReadFile(filePath, symbol, startSnapshotTime, endSnapshotTime);
}


int main() {
    OrderBook orderbook;

    // file path, modify it to read the file
    std::string filePathTxt = "SCH.log";
    std::string symbol = "SCS"; // symbol of the order book (SCS, SCH, etc.)
    //orderbook.printOrderBook();
    
    // Enter the start and end time of the snapshot
    int64_t startSnapshotTime = 1609723805976270988;
    int64_t endSnapshotTime = 1609723806144461785;

    // Get snapshot in time range
    // In the case of not giving a startSnapshotTime (ie: 0), then it will output the last snapshot at endSnapshotTime
    // because we only want the top N bids and asks at one specific time, instead of a range of time
    getSnapshotInTimeRange(filePathTxt, symbol, startSnapshotTime, endSnapshotTime);

    /*int64_t startSnapshotTime = 0;
    int64_t endSnapshotTime = 1609722900119980000;*/
    //getSnapshotInTimeRange(filePathTxt, symbol, startSnapshotTime, endSnapshotTime);


    // std::string binaryFilePath = "SCH.bin";
    // Save orders to binary file
    // saveOrdersToBinary(filePathTxt, binaryFilePath);
    // Read orders from binary file and process the data to update the order book
    // readOrdersFromBinary(binaryFilePath);
    
    //Code below is for testing purposes only:
   /* OrderBook orderBook;

    Order order1 = { 1, 1, "SCH", "BUY", "NEW", 9.6, 4 };
    Order order2 = { 2, 2, "SCH", "BUY", "NEW", 9.5, 6 };
    Order order3 = { 4, 8, "SCH", "SELL", "NEW", 9.7, 5 };
    Order order4 = { 6, 2, "SCH", "SELL", "NEW", 9.7, 10 };
    

  
    orderBook.processOrder(order1, "SC", 1, 5);
    orderBook.processOrder(order2, "SC", 1, 5);
    orderBook.processOrder(order3, "SC", 1, 5);
    orderBook.processOrder(order4, "SC", 1, 5);
    
    orderBook.printOrderBook();
    std::cout << "\n\n";
    orderBook.printSnapshots();*/


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
