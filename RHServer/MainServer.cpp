#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <fstream>

using namespace boost::asio;
using ip::tcp;

class PersistentAuthenticator {
private:
    std::map<std::string, std::string> logins; 
    std::string filename;

public:
    PersistentAuthenticator(const std::string& fname) : filename(fname) {}

    ~PersistentAuthenticator() {
        save();
    }

    void load() {
        std::ifstream fin(filename);
        if (!fin.is_open()) {
            std::cerr << "Not found error\n";
            return;
        }

        std::string line;
        while (std::getline(fin, line)) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                std::string username = line.substr(0, pos);
                std::string password = line.substr(pos + 1);
                logins[username] = password;
            }
        }
        fin.close();
    }

    void save() {
        std::ofstream fout(filename);
        if (!fout.is_open()) {
            std::cerr << "Open error\n";
            return;
        }

        for (const auto& pair : logins) {
            fout << pair.first << ":" << pair.second << "\n";
        }
        fout.close();
    }

    void addUser(const std::string& username, const std::string& password) {
        logins[username] = password;
        save(); 
    }

    bool authenticate(const std::string& username, const std::string& password) {
        auto it = logins.find(username);
        if (it != logins.end()) {
            return it->second == password;
        }
        return false;
    }
    bool authenticateAdmin(const std::string& username) {
        return logins.count(username) > 0;
    }
    void removeUser(const std::string& username) {
        if (logins.erase(username) > 0) {
            save();
        }
        else {
            std::cout << "Not found this login\n";
        }
    }
};

std::string getHash(const std::string& s) {
    std::size_t hashValue = std::hash<std::string>{}(s);
    std::ostringstream oss;
    oss << std::hex << hashValue;
    return oss.str();
}

int main() {
    try {
		PersistentAuthenticator auth("logins.txt");
		auth.load();
        if (!auth.authenticateAdmin("admin")) {
            auth.addUser("admin", getHash("admin"));
        }
        std::cout << "Entr login. Them enter password" << std::endl;
        std::string servLogin, servPassword;
        std::getline(std::cin, servLogin);
        std::getline(std::cin, servPassword);
        if (!auth.authenticate(servLogin, getHash(servPassword))) {
            std::cout << "Authentication faild.\n";
            return 0;
        }
        if (servPassword == "admin") {
            std::cout << "Entr new login. Them enter new password" << std::endl;
            std::getline(std::cin, servLogin);
            std::getline(std::cin, servPassword);
			auth.removeUser("admin");
            auth.addUser(servLogin, getHash(servPassword));
        }
        io_context ioContext;
        tcp::acceptor acceptor(ioContext, tcp::endpoint(tcp::v4(), 8080));
        while (true) {
			std::cout << "Do you want to add or remove user? (add/remove/continue): ";
			std::string command;
			std::getline(std::cin, command);
            
            if (command == "add") {
                std::string newLogin, newPassword;
                std::cout << "Enter new login: ";
                std::getline(std::cin, newLogin);
                std::cout << "Enter new password: ";
                std::getline(std::cin, newPassword);
                auth.addUser(newLogin, getHash(newPassword));
                std::cout << "User added successfully.\n";
            } else if (command == "remove") {
                std::string removeLogin;
                std::cout << "Enter login to remove: ";
                std::getline(std::cin, removeLogin);
                auth.removeUser(removeLogin);
            } else if (command == "continue") {
                break;
            }
            else {
                std::cout << "Unknown command.\n";
            }
            
        }

        std::cout << "Server listening on port 8080\n";

        
        while (true) {
            tcp::socket socket(ioContext);
            acceptor.accept(socket);

            std::array<char, 1024> buf;
            size_t length = socket.read_some(buffer(buf));
            std::string request(buf.data(), length);
            std::cout << "Received from client: \"" << request << "\"\n";
            length = socket.read_some(buffer(buf));
            std::string request2(buf.data(), length);
            std::cout << "Received from client: \"" << request2 << "\"\n";
            if (!auth.authenticate(request, getHash(request2))) {
                std::string response = "Authentication faild.";
                write(socket, buffer(response));
                return 0;
            }
            std::string response = "Information from server";
            write(socket, buffer(response));
        }
    }
    catch (const std::system_error& err) {
        std::cerr << "Error occurred: " << err.what() << std::endl;
    }

    return 0;
}