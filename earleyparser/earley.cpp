#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <memory>
#include <stdexcept>

struct Rule {
    char left;
    std::string right;
    std::string toString() const {
        return std::string(1, left) + " -> " + (right.empty() ? "epsilon" : right);
    }
};

struct Grammar {
    char StartSymbol;
    std::unordered_set<char> NonTerminals;
    std::unordered_set<char> Terminals;
    std::vector<Rule> Rules;
    std::unordered_map<char, std::vector<size_t>> RulesIndex;

    void Index() {
        RulesIndex.clear();
        for (size_t i = 0; i < Rules.size(); ++i) {
            RulesIndex[Rules[i].left].push_back(i);
        };
    }

    bool isNonTerminal(char c) const {
        return NonTerminals.count(c);
    }
};

struct EarleyItem {
    size_t RuleIndex;
    size_t CurrentPoint;
    size_t StartPoint;

    bool operator==(const EarleyItem& other) const {
        return RuleIndex == other.RuleIndex && 
               CurrentPoint == other.CurrentPoint && 
               StartPoint == other.StartPoint;
    }
};

namespace std {
    template<>
    struct hash<EarleyItem> {
        size_t operator()(const EarleyItem& e) const {
            size_t h1 = std::hash<size_t>{}(e.RuleIndex);
            size_t h2 = std::hash<size_t>{}(e.CurrentPoint);
            size_t h3 = std::hash<size_t>{}(e.StartPoint);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}


class EarleyParser {
private:
    Grammar grammar;
    const char start_of_parse = '@'; 
public:
    EarleyParser() {};

    void Prepare(Grammar G) {
        if (G.Rules.empty() && G.NonTerminals.empty()) {
             throw std::runtime_error("Грамматика пустая.");
        }
        Rule StartRule;
        StartRule.left = start_of_parse;
        StartRule.right = std::string(1, G.StartSymbol);
        G.Rules.push_back(StartRule);
        G.NonTerminals.insert(start_of_parse);
        G.Index();
        this->grammar = G;
    }

    bool Predict(const std::string& word) {
        size_t n = word.length();
        std::vector<std::vector<EarleyItem>> Table(n + 1);
        std::vector<std::unordered_set<EarleyItem>> UniqueSets(n+1);
        size_t StartRuleIndex = grammar.Rules.size() - 1;
        AddState(0, {StartRuleIndex, 0, 0}, Table, UniqueSets);
        
        for (size_t i = 0; i <= n; ++i) {
            for (size_t j = 0; j < Table[i].size(); ++j) {

                const EarleyItem item = Table[i][j];
                const Rule& rule = grammar.Rules[item.RuleIndex];

                if (item.CurrentPoint < rule.right.size()) {
                    char NextSymbol = rule.right[item.CurrentPoint];

                    if (grammar.isNonTerminal(NextSymbol)) {
                        Predictor(NextSymbol, i, Table, UniqueSets);
                    } else {
                        if (i < n && word[i] == NextSymbol) {
                            Scan(item, i, Table, UniqueSets);
                        }
                    }

                } else {
                    Complete(item, i, Table, UniqueSets);
                }
            }
        }
        for (const auto& item : Table[n]) {
            const Rule& r = grammar.Rules[item.RuleIndex];
            if (r.left == start_of_parse && item.CurrentPoint == r.right.size() && item.StartPoint == 0) {
                return true;
            }
        }
        return false;
    }

private:
    void AddState(
        size_t index,
        EarleyItem item,
        std::vector<std::vector<EarleyItem>>& Table,
        std::vector<std::unordered_set<EarleyItem>>& UniqueSets
    ) {
        if (UniqueSets[index].find(item) == UniqueSets[index].end()) {
            UniqueSets[index].insert(item);
            Table[index].push_back(item);
        }
    }

    void Predictor(
        char NonTerm,
        size_t index, 
        std::vector<std::vector<EarleyItem>>& Table, 
        std::vector<std::unordered_set<EarleyItem>>& uniqueSets
    ) {
        if (grammar.RulesIndex.find(NonTerm) != grammar.RulesIndex.end()) {
            const auto& indices = grammar.RulesIndex[NonTerm];
            for (size_t rIdx : indices) {
                AddState(index, {rIdx, 0, index}, Table, uniqueSets);
            }
        }
    }

    void Scan(const EarleyItem& item, size_t index, 
                 std::vector<std::vector<EarleyItem>>& Table, 
                 std::vector<std::unordered_set<EarleyItem>>& UniqueSets) {
        
        AddState(index + 1, {item.RuleIndex, item.CurrentPoint + 1, item.StartPoint}, Table, UniqueSets);
    }

    void Complete(const EarleyItem& item, size_t index, 
                   std::vector<std::vector<EarleyItem>>& Table, 
                   std::vector<std::unordered_set<EarleyItem>>& UniqueSets) {
        
        size_t origin = item.StartPoint;
        char finishedNonTerm = grammar.Rules[item.RuleIndex].left;

        for (const auto& prevItem : Table[origin]) {
            const Rule& r = grammar.Rules[prevItem.RuleIndex];
            if (prevItem.CurrentPoint < r.right.size() && r.right[prevItem.CurrentPoint] == finishedNonTerm) {
                AddState(index, {prevItem.RuleIndex, prevItem.CurrentPoint + 1, prevItem.StartPoint}, Table, UniqueSets);
            }
        }
    }
};

class GrammarReader {
public:
    static Grammar readFromStream(std::istream& in) {
        Grammar g;
        int n;
        int sigma;
        int p;
        
        if (!(in >> n >> sigma >> p)) {
            throw std::runtime_error("Ошибка ввода");
        }

        for (int i = 0; i < n; ++i) {
            std::string nt;
            in >> nt;
            g.NonTerminals.insert(nt[0]);
        }

        for (int i = 0; i < sigma; ++i) {
            char t;
            in >> t; 
            g.Terminals.insert(t); 
        }
        
        in >> std::ws;
        for (int i = 0; i < p; ++i) {
            std::string line;
            std::getline(in, line);
            if (!line.empty() && line.back() == '\r') {line.pop_back();}
            if (line.empty()) { i--; continue; }
            g.Rules.push_back(parseRule(line));
        }

        std::string StartS; in >> StartS;
        g.StartSymbol = StartS[0];
        return g;
    }

private:
    static Rule parseRule(const std::string& line) {
        auto pos = line.find("->");
        std::string left;
        std::string right;
        std::stringstream(line.substr(0, pos)) >> left;
        std::stringstream(line.substr(pos + 2)) >> right;
        return Rule{left[0], right};
    }
};

#ifndef TEST_MODE 
int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    std::cout << "Введите входные данные.\n";
    try {
        Grammar grammar = GrammarReader::readFromStream(std::cin);
        EarleyParser parser;
        parser.Prepare(grammar);
        std::cout << "\n";
        int m;
        if (std::cin >> m) {
            std::cin >> std::ws;
            for (int i = 0; i < m; ++i) {
                std::string word;
                std::getline(std::cin, word);
                if (!word.empty() && word.back() == '\r') {word.pop_back();}
                std::cout << "" << (parser.Predict(word) ? "YES\n" : "NO\n");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
    }
}
#endif
