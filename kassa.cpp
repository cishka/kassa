#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iomanip>//для вывода
#include <algorithm>

enum class Category {
	Vegetables,//овощи
	Dairy, //молочные продукты
	Fruits, // фрукты
	Other
};

class Product {
private:
	std::string barcode; //штрихкод
	std::string name; // название
	Category category; //категория
	double pricePerkg; // цена за кг товара
	double discount; // скидка на товар
	double quantity; // кол-во товара

public:
	Product(const std::string& barcode, const std::string& name, Category category, double pricePerkg, double discount, double quantity):
		barcode(barcode),name(name),category(category),pricePerkg(pricePerkg),discount(discount), quantity(quantity){}

	Product(const Product& other) = default;

	bool operator==(const Product& other) const {
		return barcode == other.barcode && name == other.name && category == other.category && pricePerkg == other.pricePerkg && discount == other.discount;
	}

	bool operator <(const Product& other) const {
		return calculateCost() < other.calculateCost();
	}
	
	Product operator +(const Product& other) const {
		if (barcode != other.barcode) throw std::invalid_argument("Diff products");
		return Product(barcode, name, category, pricePerkg, discount, quantity + other.quantity);
	}

	Product operator -(const Product& other) const {
		if (barcode != other.barcode) throw std::invalid_argument("Diff products");
		if (quantity < other.quantity) throw std::invalid_argument("not enough quantity to subtract");
		return Product(barcode, name, category, pricePerkg, discount, quantity - other.quantity);
	}

	double calculateCost() const {
		return pricePerkg * discount * quantity;
	}

	std::string getDescription() const {
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(2);
		oss << name << "...." << quantity << calculateCost() << "*" << pricePerkg * discount << "....";
		return oss.str();
	}
	
	std::string getName() const {
		return name;
	}
		
};

class Store {
private:
	std::vector<Product> allGoods;

public:
	Store() {
		allGoods.emplace_back("001", "Банан", Category::Fruits, 105.0, 1.0, 100);
		allGoods.emplace_back("002", "Яблоко", Category::Fruits, 150.0, 1.0, 50);
		allGoods.emplace_back("003", "Огурец", Category::Vegetables, 200.0, 1.0, 30);
		allGoods.emplace_back("004", "Помидор", Category::Vegetables, 300.0, 1.0, 20);
		allGoods.emplace_back("005", "Картошка", Category::Vegetables, 50.0, 1.0, 100);
	}

	Product findProduct(const std::string& name, double quantity) {
		auto it = std::find_if(allGoods.begin(), allGoods.end(), [&](const Product& product) {
			return product.getName().find(name) != std::string::npos;
		});

		if (it == allGoods.end()) {
			throw std::runtime_error("Product not find" + name);
		}

		if (it->calculateCost() < quantity) {
			throw std::runtime_error("Недостаточно товара" + name);
		}

		return *it;
	}
};

class Check {
private:
	std::vector<Product> purchasedGoods;//купленные
	std::vector<std::string> unrecognizedProducts;//нераспознанные 

	double calculatedTotal() const {
		double total = 0.0;
		for (const auto& product : purchasedGoods) {
			total += product.calculateCost();
		}
		return total;
	}
public:
	void addProduct(const Product& product) {
		auto it = std::find_if(purchasedGoods.begin(), purchasedGoods.end(), [&](const Product& p) {
			return p == product;
			});
		if (it != purchasedGoods.end()) {
			*it = *it + product;
		}
		else {
			purchasedGoods.push_back(product);
		}
	}

	void removeProduct(const std::string& name) {
		auto it = std::remove_if(purchasedGoods.begin(), purchasedGoods.end(), [&](const Product& p) {
			return p.getName() == name;
			});
		purchasedGoods.erase(it, purchasedGoods.end());
	}

	void addUnrecognizedProduct(const std::string& order) {
		// Логика добавления нераспознанного заказа
		std::cout << "Нераспознанный товар: " << order << std::endl;
	}

	void printCheck(std::ostream& os)const {
		os << "ЧекP:\n";
		for (const auto& product : purchasedGoods) {
			os << product.getDescription() << "\n";
			os << "Сумма: " << calculatedTotal() << "\n";
			if (!unrecognizedProducts.empty()) {
				os << "Не найдено:\n";
				for (const auto& product : unrecognizedProducts) {
					os << product << "\n";
				}
			}
		}
	}
};

class Parser {
public:
	void parseFile(const std::string& filename, Store& store, Check& receipt) {
		std::ifstream file(filename);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open the file!");
		}

		std::string line;
		while (std::getline(file, line)) {
			bool shouldRemove = false;
			if (line.find("remove") != std::string::npos || line.find("delete") != std::string::npos) {
				shouldRemove = true;
			}

			double quantity = 0.0;
			for (char ch : line) {
				if (isdigit(ch)) {
					quantity = ch - '0';
					break;
				}
			}

			std::string name;
			size_t numPos = std::string::npos;
			for (size_t i = 0; i < line.length(); i++) {
				if (isdigit(line[i])) {
					numPos = i;
					break;
				}
			}

			if (numPos != std::string::npos && numPos + 2 < line.length()) {
				name = line.substr(numPos + 2);
			}
			else {
				name = "unknown";
			}

			if (!name.empty() && !std::isalnum(name.back())) {
				name.pop_back();
			}

			try {
				Product product = store.findProduct(name, quantity);
				if (shouldRemove) {
					receipt.removeProduct(name);
				}
				else {
					receipt.addProduct(product);
				}
			}
			catch (...) {
				receipt.addUnrecognizedProduct(name);
			}
		}
	}
};

int main() {
	try {
		Store supermarket;

		Check myReceipt;

		Parser parser;
		parser.parseFile("orders.txt", supermarket, myReceipt);

		std::ofstream output("check.txt");
		myReceipt.printCheck(output);

		std::cout << "Receipt successfully generated in output.txt!" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}

	return 0;
}



