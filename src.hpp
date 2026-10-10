#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <sstream>

#include <fstream>
#include <cstdio>
#include <algorithm>
#include <stdexcept>

// Basic exception class
class BasicException {
protected:
    std::string message;
public:
    explicit BasicException(const std::string& msg) : message(msg) {}
    virtual const char* what() const noexcept { return message.c_str(); }
};

class ArgumentException : public BasicException {
public:
    explicit ArgumentException(const std::string& msg) : BasicException(msg) {}
};

class IteratorException : public BasicException {
public:
    explicit IteratorException(const std::string& msg) : BasicException(msg) {}
};

struct Pokemon {
    char name[12]; // as required, max length 10 + null
    int id;
    std::vector<std::string> types;
};

class Pokedex {
private:
    // map ordered by id (ascending)
    std::map<int, Pokemon> data;
    const std::unordered_set<std::string> allowedTypes = {"water","fire","grass","electric","ground","flying","dragon"};
    // effectiveness table: attacker -> defender -> multiplier
    const std::unordered_map<std::string, std::unordered_map<std::string, double>> eff = {
        {"water", {{"fire",2.0},{"grass",0.5},{"water",0.5},{"ground",2.0},{"dragon",1.0},{"electric",1.0},{"flying",1.0}}},
        {"fire", {{"grass",2.0},{"water",0.5},{"fire",0.5},{"ground",0.5},{"dragon",0.5},{"electric",1.0},{"flying",1.0}}},
        {"grass", {{"water",2.0},{"ground",0.5},{"fire",0.5},{"grass",0.5},{"dragon",0.5},{"electric",1.0},{"flying",2.0}}},
        {"electric", {{"water",2.0},{"ground",0.0},{"grass",1.0},{"fire",1.0},{"electric",0.5},{"flying",2.0},{"dragon",1.0}}},
        {"ground", {{"electric",2.0},{"grass",2.0},{"water",1.0},{"fire",2.0},{"ground",0.5},{"flying",0.0},{"dragon",1.0}}},
        {"flying", {{"grass",2.0},{"electric",0.5},{"ground",1.0},{"water",1.0},{"fire",1.0},{"flying",0.5},{"dragon",1.0}}},
        {"dragon", {{"dragon",2.0},{"water",1.0},{"fire",1.0},{"grass",1.0},{"electric",1.0},{"ground",1.0},{"flying",1.0}}}
    };

    std::vector<std::string> splitTypes(const std::string& s) const {
        std::vector<std::string> res;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, '#')) {
            if (!item.empty()) res.push_back(item);
        }
        return res;
    }

    void validateName(const std::string& name) const {
        if (name.empty() || name.size() > 10) {
            throw ArgumentException("Argument Error: PM Name Invalid (" + name + ")");
        }
        // ensure unique
        for (const auto& kv : data) {
            if (std::string(kv.second.name) == name) {
                throw ArgumentException("Argument Error: PM Name Invalid (" + name + ")");
            }
        }
    }

    void validateId(int id) const {
        if (id <= 0) {
            throw ArgumentException("Argument Error: PM ID Invalid (" + std::to_string(id) + ")");
        }
        if (data.find(id) != data.end()) {
            throw ArgumentException("Argument Error: PM ID Invalid (" + std::to_string(id) + ")");
        }
    }

    std::vector<std::string> validateAndParseTypes(const std::string& typesStr) const {
        auto vec = splitTypes(typesStr);
        if (vec.empty() || vec.size() > 7) {
            throw ArgumentException("Argument Error: PM Type Invalid (" + typesStr + ")");
        }
        for (const auto& t : vec) {
            if (allowedTypes.find(t) == allowedTypes.end()) {
                throw ArgumentException("Argument Error: PM Type Invalid (" + t + ")");
            }
        }
        return vec;
    }

    double getSingleMultiplier(const std::string& atk, const std::string& def) const {
        auto itA = eff.find(atk);
        if (itA == eff.end()) return 1.0;
        auto itD = itA->second.find(def);
        if (itD == itA->second.end()) return 1.0;
        return itD->second;
    }

    private:
        std::string fileName;
        void loadFromFile() {
            std::ifstream fin(fileName);
            if (!fin) return;
            std::string line;
            while (std::getline(fin, line)) {
                if (line.empty()) continue;
                std::stringstream ss(line);
                std::string name; int id; std::string typesStr;
                ss >> name >> id >> typesStr;
                if (ss.fail()) continue;
                Pokemon p; std::snprintf(p.name, sizeof(p.name), "%s", name.c_str());
                p.id = id;
                p.types = splitTypes(typesStr);
                data.emplace(id, std::move(p));
            }
        }
        void saveToFile() const {
            std::ofstream fout(fileName, std::ios::trunc);
            for (const auto& kv : data) {
                const auto& p = kv.second;
                std::ostringstream typesOut;
                for (size_t i = 0; i < p.types.size(); ++i) {
                    typesOut << p.types[i];
                    if (i + 1 < p.types.size()) typesOut << '#';
                }
                fout << p.name << ' ' << p.id << ' ' << typesOut.str() << '\n';
            }
        }



public:
    explicit Pokedex(const char* _fileName) : fileName(_fileName ? _fileName : "") {
        if (!fileName.empty()) loadFromFile();
    }
    ~Pokedex() {
        if (!fileName.empty()) saveToFile();
    }

    bool pokeAdd(const char* name_c, int id, const char* types_c) {
        try {
            std::string name(name_c);
            std::string typesStr(types_c);
            validateName(name);
            validateId(id);
            auto typesVec = validateAndParseTypes(typesStr);
            Pokemon p;
            std::snprintf(p.name, sizeof(p.name), "%s", name.c_str());
            p.id = id;
            p.types = std::move(typesVec);
            data.emplace(id, std::move(p));
            return true;
        } catch (const ArgumentException&) {
            throw; // rethrow as required
        }
        return false;
    }

    bool pokeDel(int id) {
        auto it = data.find(id);
        if (it != data.end()) {
            data.erase(it);
            return true;
        }
        return false;
    }

    std::string pokeFind(int id) const {
        auto it = data.find(id);
        if (it != data.end()) {
            return std::string(it->second.name);
        }
        return "None";
    }

    std::string typeFind(const char* types_c) const {
        try {
            std::string typesStr(types_c);
            auto queryTypes = validateAndParseTypes(typesStr);
            std::vector<const Pokemon*> matches;
            for (const auto& kv : data) {
                const auto& p = kv.second;
                bool ok = true;
                for (const auto& qt : queryTypes) {
                    if (std::find(p.types.begin(), p.types.end(), qt) == p.types.end()) {
                        ok = false; break;
                    }
                }
                if (ok) matches.push_back(&p);
            }
            if (matches.empty()) return "None";
            std::ostringstream oss;
            oss << matches.size() << "\n";
            for (size_t i = 0; i < matches.size(); ++i) {
                oss << matches[i]->name;
                if (i + 1 < matches.size()) oss << "\n";
            }
            return oss.str();
        } catch (const ArgumentException&) {
            throw;
        }
    }

    float attack(const char* type_c, int id) const {
        std::string atk(type_c);
        if (allowedTypes.find(atk) == allowedTypes.end()) {
            throw ArgumentException("Argument Error: PM Type Invalid (" + atk + ")");
        }
        auto it = data.find(id);
        if (it == data.end()) return -1.0f;
        const auto& target = it->second;
        double mult = 1.0;
        for (const auto& def : target.types) {
            mult *= getSingleMultiplier(atk, def);
        }
        return static_cast<float>(mult);
    }

    int catchTry() const {
        if (data.empty()) return 0;
        // start with smallest id
        std::unordered_set<int> owned;
        auto itStart = data.begin();
        owned.insert(itStart->first);
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& kv : data) {
                int pid = kv.first;
                if (owned.count(pid)) continue;
                const Pokemon& target = kv.second;
                bool capturable = false;
                for (int oid : owned) {
                    const Pokemon& owner = data.at(oid);
                    for (const auto& atkType : owner.types) {
                        double mult = 1.0;
                        for (const auto& defType : target.types) {
                            mult *= getSingleMultiplier(atkType, defType);
                        }
                        if (mult >= 2.0 - 1e-9) { // at least 2x
                            capturable = true; break;
                        }
                    }
                    if (capturable) break;
                }
                if (capturable) {
                    owned.insert(pid);
                    changed = true;
                }
            }
        }
        return static_cast<int>(owned.size());
    }

    // iterator wrapper
    class iterator {
    private:
        std::map<int, Pokemon>::iterator it;
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Pokemon;
        using difference_type = std::ptrdiff_t;
        using pointer = Pokemon*;
        using reference = Pokemon&;
        iterator() = default;
        explicit iterator(std::map<int, Pokemon>::iterator i) : it(i) {}
        iterator& operator++() { ++it; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++it; return tmp; }
        iterator& operator--() { --it; return *this; }
        iterator operator--(int) { iterator tmp = *this; --it; return tmp; }
        iterator& operator=(const iterator& rhs) { it = rhs.it; return *this; }
        bool operator==(const iterator& rhs) const { return it == rhs.it; }
        bool operator!=(const iterator& rhs) const { return it != rhs.it; }
        reference operator*() const { return const_cast<reference>(it->second); }
        pointer operator->() const { return const_cast<pointer>(&it->second); }
    };

    iterator begin() { return iterator(data.begin()); }
    iterator end() { return iterator(data.end()); }
};
