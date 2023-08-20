#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>
#include <thread>
#include <chrono>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;


class ReadCSV {
public:
    map<string, vector<string>> CustomerOrder;
    map<string, pair<string, string>> ordUsageRate;
    map<string, string> customerData;
    map<string, string> orderData;

public:
    void readCSV(const string& filename) {
        vector<vector<string>> values;
        vector<string> row;
        string line, word;

        fstream file(filename, ios::in);
        if (file.is_open()) {
            while (getline(file, line)) {
                row.clear();
                stringstream str(line);
                while (getline(str, word, ',')) {
                    row.push_back(word);
                }

                if (row.size() == 4) {
                    string customerID = row[0];
                    string orderID = row[1];
                    string usage = row[2];
                    string rate = row[3];

                    CustomerOrder[customerID].push_back(orderID);
                    ordUsageRate[orderID] = make_pair(usage, rate);
                }
            }
        }
        else {
            cout << "Could not open the file: " << filename << "\n";
        }

        //to read data into customerData and orderData maps
        ifstream dataFile("Customer.csv");
        if (dataFile.is_open()) {
            string dataLine;
            getline(dataFile, dataLine); // Skip the header line
            while (getline(dataFile, dataLine)) {
                stringstream ss(dataLine);
                string sno, customerID, _, orderNo;
                getline(ss, sno, ',');
                getline(ss, customerID, ',');
                getline(ss, _, ','); // Skip the 3rd column
                getline(ss, orderNo, ','); // Read the 4th column as Order no.

                // to remove quotes if present
                //not neccessary
                customerID.erase(remove(customerID.begin(), customerID.end(), '\"'), customerID.end());
                orderNo.erase(remove(orderNo.begin(), orderNo.end(), '\"'), orderNo.end());

                customerData[customerID] = orderNo;
            }
            dataFile.close();
        }
        else {
            cout << "Could not open the file: Customer.csv\n";
        }

        ifstream orderFile("dataAvailable.csv");
        if (orderFile.is_open()) {
            string orderLine;
            getline(orderFile, orderLine); // Skip the header line
            while (getline(orderFile, orderLine)) {
                stringstream ss(orderLine);
                string sno, customerID, orderNo;
                getline(ss, sno, ',');
                getline(ss, customerID, ',');
                getline(ss, orderNo, ',');

                // Quotes removal
                //not necessary 
                customerID.erase(remove(customerID.begin(), customerID.end(), '\"'), customerID.end());
                orderNo.erase(remove(orderNo.begin(), orderNo.end(), '\"'), orderNo.end());

                orderData[customerID] = orderNo;
            }
            orderFile.close();
        }
        else {
            cout << "Could not open the file: dataAvailable.csv\n";
        }
    }

    void displayTheValues() {
        for (const auto& customer : CustomerOrder) {
            cout << "Customer ID: " << customer.first << endl;
            for (const auto& orderID : customer.second) {
                cout << "Order ID: " << orderID << endl;
                const auto& order = ordUsageRate[orderID];
                cout << "Usage: " << order.first << endl;

                float usage = stof(order.first);
                float rate = stof(order.second);

                cout << "Rate: " << order.second << endl;
                float cost = usage * rate;
                cout << "Cost: " << cost << endl;

                float allocation = stof(order.first);
                float balance = allocation - cost;
                cout << "Balance: " << balance << endl;
                cout << endl;
            }
            cout << endl;
        }

        // to display customerData and orderData
        cout << "Data from Customer.csv:" << endl;
        for (const auto& data : customerData) {
            cout << "Customer ID: " << data.first << ", Order no.: " << data.second << endl;
        }
        cout << endl;

        cout << "Data from dataAvailable.csv:" << endl;
        for (const auto& data : orderData) {
            cout << "Customer ID: " << data.first << ", Order no.: " << data.second << endl;
        }
        cout << endl;
    }
};

class DaemonProcess {
private:
    ReadCSV& csvData;
    SOCKET serverSocket;

public:
    DaemonProcess(ReadCSV& data) : csvData(data), serverSocket(INVALID_SOCKET) {}

    void run() {
        // Create and start the TCP server
        if (!startTCPServer()) {
            cout << "Error: Failed to start the TCP server." << endl;
            return;
        }

        while (true) {
            cout << "Daemon process is running..." << endl;
            compareCustomerIDs();

            this_thread::sleep_for(chrono::seconds(10));
        }

        // Close the server socket 
        closeTCPServer();
    }

    // Fn to start the TCP server
    bool startTCPServer() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cout << "Error: WSAStartup failed." << endl;
            return false;
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            cout << "Error: Failed to create socket." << endl;
            WSACleanup();
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(54000); // Change the port number if needed
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cout << "Error: Failed to bind the socket." << endl;
            closesocket(serverSocket);
            WSACleanup();
            return false;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            cout << "Error: Failed to listen on the socket." << endl;
            closesocket(serverSocket);
            WSACleanup();
            return false;
        }

        cout << "Server is listening for incoming connections..." << endl;
        return true;
    }

    // Fn to close the TCP server
    void closeTCPServer() {
        closesocket(serverSocket);
        WSACleanup();
    }

    void compareCustomerIDs() {
        //Same code
    }
};
