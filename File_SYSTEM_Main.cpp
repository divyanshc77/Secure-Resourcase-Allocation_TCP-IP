#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>
#include <Winsock2.h>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

class CustomerIDComparator {
public:
    static bool compare(const string& customerID1, const string& customerID2) {
        return customerID1 == customerID2;
    }
};

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

        // Additional logic to read data into customerData and orderData maps
        ifstream dataFile("Customer.csv");
        if (dataFile.is_open()) {
            string dataLine;
            getline(dataFile, dataLine); // Skip the header line
            while (getline(dataFile, dataLine)) {
                stringstream ss(dataLine);
                string sno, customerID, _, orderNo, allocated;
                getline(ss, sno, ',');
                getline(ss, customerID, ',');
                getline(ss, _, ','); // Skip the 3rd column
                getline(ss, orderNo, ','); // Read the 4th column as Order no.
                getline(ss, _, ','); // Skip the 5th column
                getline(ss, _, ','); // Skip the 6th column
                getline(ss, allocated, ','); // Read the 7th column as Allocated

                // Remove quotes if present
                customerID.erase(remove(customerID.begin(), customerID.end(), '\"'), customerID.end());
                orderNo.erase(remove(orderNo.begin(), orderNo.end(), '\"'), orderNo.end());
                allocated.erase(remove(allocated.begin(), allocated.end(), '\"'), allocated.end());

                customerData[customerID] = allocated;
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

                // Remove quotes if present
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
                float allocation = stof(customerData[customer.first]);
                float balance = allocation - cost;
                cout << "Balance: " << balance << endl;
                cout << endl;
            }
            cout << endl;
        }
    }
};

class AuthenticatedDataWriter {
public:
    void writeAuthenticatedData(const string& data) {
        ofstream outputFile("authenticated_data_received.csv");
        if (outputFile.is_open()) {
            outputFile << data;
            outputFile.close();
            cout << "Authenticated data has been written to authenticated_data_received.csv" << endl;
        } else {
            cerr << "Error: Could not open the file authenticated_data_received.csv for writing." << endl;
        }
    }
};

class BalanceCalculator {
private:
    map<string, string> customerData;
    map<string, string> usageData;

public:
    BalanceCalculator(map<string, string>& customerData, map<string, string>& usageData)
        : customerData(customerData), usageData(usageData) {}

    void calculateBalance() {
        for (const auto& customer : customerData) {
            const string& customerID = customer.first;
            const string& allocationValue = customer.second;
            const string& usageValue = usageData[customerID];

            if (!usageValue.empty()) {
                float allocation = stof(allocationValue);
                float usage = stof(usageValue);
                float balance = allocation - usage;
                cout << "Customer ID: " << customerID << ", Allocation: " << allocation
                     << ", Usage: " << usage << ", Balance: " << balance << endl;
            } else {
                cout << "Customer ID: " << customerID << ", Allocation: " << allocationValue
                     << ", No usage data found. Cannot calculate balance." << endl;
            }
        }
    }
};

class DaemonProcess {
private:
    ReadCSV& csvData;
    AuthenticatedDataWriter& dataWriter;

public:
    DaemonProcess(ReadCSV& data, AuthenticatedDataWriter& writer) : csvData(data), dataWriter(writer) {}

    void run() {
        while (true) {
            cout << "Daemon process is running..." << endl;
            compareCustomerIDs();

            this_thread::sleep_for(chrono::seconds(10));
        }
    }

    void compareCustomerIDs() {
        cout << "Comparison of Customer IDs:" << endl;
        for (const auto& data : csvData.customerData) {
            const string& customerID = data.first;
            const string& orderNoCustomer = data.second;
            const string& orderNoDataAvailable = csvData.orderData[customerID];

            bool match = CustomerIDComparator::compare(orderNoCustomer, orderNoDataAvailable);

            cout << "Customer ID: " << customerID << ", Order no. (Customer.csv): " << orderNoCustomer
                 << ", Order no. (dataAvailable.csv): " << orderNoDataAvailable << ", Match: " << (match ? "Authenticate" : "Non Authenticate")
                 << endl;
        }

        BalanceCalculator balanceCalc(csvData.customerData, csvData.orderData);
        balanceCalc.calculateBalance();

        cout << endl;
    }
};

int main() {
    // Initialize Winsock
    WSADATA wsData;
    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
        cerr << "Error: Failed to initialize Winsock." << endl;
        return 1;
    }

    ReadCSV open;
    open.readCSV("Customer.csv");
    open.readCSV("Usage.csv");
    open.readCSV("Rate.csv");

    AuthenticatedDataWriter dataWriter;
    DaemonProcess daemon(open, dataWriter);

    thread daemonThread(&DaemonProcess::run, &daemon);
    daemonThread.detach();

    // Producer Client Code
    string dataForAuthentication =
        "CUST12345,ORD12345\n"
        "CUST67890,ORD67890\n"
        "CUST23456,ORD23456\n"
        "CUST78901,ORD78901\n"
        "CUST34567,ORD34567\n"
        "CUST89012,ORD89012\n"
        "CUST45678,ORD45678\n"
        "CUST90123,ORD90123\n"
        "CUST56789,ORD56789\n"
        "CUST01234,ORD01234\n";

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Error: Could not create the client socket." << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Error: Could not connect to the server." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if (send(clientSocket, dataForAuthentication.c_str(), dataForAuthentication.size(), 0) == SOCKET_ERROR) {
        cerr << "Error: Failed to send data to the server." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    cout << "Data sent to the server for authentication:" << endl;
    cout << dataForAuthentication << endl;

    char buffer[1024] = {0};
    if (recv(clientSocket, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
        cerr << "Error: Failed to receive authenticated data from the server." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    cout << "Authenticated data received from the server:" << endl;
    cout << buffer << endl;

    closesocket(clientSocket);
    WSACleanup();

    ofstream outputFile("authenticated_data_received.csv");
    if (outputFile.is_open()) {
        outputFile << buffer;
        outputFile.close();
        cout << "Authenticated data has been written to authenticated_data_received.csv" << endl;
    } else {
        cerr << "Error: Could not open the file authenticated_data_received.csv for writing." << endl;
        return 1;
    }

    return 0;
}
