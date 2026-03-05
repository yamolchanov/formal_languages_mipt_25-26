#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <algorithm>
#include <stdexcept>
#include <functional>



// техника
enum SymbolType { TERMINAL, NON_TERMINAL };

struct Symbol {
    std::string value;
    SymbolType type;
    bool operator==(const Symbol& other) const {
        return value == other.value && type == other.type;
    }
};

struct SymbolHasher {
    std::size_t operator()(const Symbol& s) const {
        return std::hash<std::string>{}(s.value) ^ (std::hash<int>{}((int)s.type) << 1);
    }
};

using SymbolSet = std::unordered_set<Symbol, SymbolHasher>;

struct prod {
    int id;
    Symbol left;
    std::vector<Symbol> right;

    bool operator==(const prod& other) const {
        return left == other.left && right == other.right;
    }
};

struct ProdHasher {
    std::size_t operator()(const prod& p) const {
        std::size_t seed = SymbolHasher{}(p.left);
        for (const auto& s : p.right) {
            seed = seed * 31 + SymbolHasher{}(s);
        }
        return seed;
    }
};


// грамматика
struct Grammar {
    SymbolSet term;
    SymbolSet non_term;
    std::vector<prod> productions;
    Symbol pr_start_symbol;

    std::vector<prod> getProductionsFor(const Symbol& left) const {
        std::vector<prod> result;
        for (const auto& prod : productions) {
            if (prod.left == left) {
                result.push_back(prod);
            }
        }
        return result;
    }
};

// lr(1)-ситуация.
struct LR1Item {
    prod prod;
    size_t dot;
    Symbol lookahead;
    bool operator==(const LR1Item& other) const {
        return prod == other.prod && 
               dot == other.dot && 
               lookahead == other.lookahead;
    }
};

// опять техника aka hash'ик
struct LR1ItemHasher {
    std::size_t operator()(const LR1Item& item) const {
        std::size_t h1 = ProdHasher{}(item.prod);
        std::size_t h2 = std::hash<std::size_t>{}(item.dot);
        std::size_t h3 = SymbolHasher{}(item.lookahead);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

using ItemSet = std::unordered_set<LR1Item, LR1ItemHasher>;
struct ItemSetHasher {
    std::size_t operator()(const ItemSet& s) const {
        std::size_t seed = 0;
        for (const auto& item : s) {
            seed += LR1ItemHasher{}(item);
        }
        return seed;
    }
};

enum ActionType { SHIFT, REDUCE, ACCEPT, ERROR };

struct Action {
    ActionType type;
    int stateId; 
    int prodId;
};

// сам алгоритм
class LR1Parser {
private:
    Grammar gram;
    Grammar start_gram;
    std::unordered_map<int, std::unordered_map<Symbol, Action, SymbolHasher>> actionTable;
    std::unordered_map<int, std::unordered_map<Symbol, int, SymbolHasher>> gotoTable;
    std::unordered_map<Symbol, SymbolSet, SymbolHasher> firstSets;

    SymbolSet computeFirst(const Symbol& s) {
        if (s.type == TERMINAL) {
            return {s};
        } else { return firstSets[s];}
    }

    SymbolSet comp_first(const std::vector<Symbol>& seq) {
        SymbolSet result;
        bool if_all_eps = true;
        for (const auto& sym : seq) {
            SymbolSet firstS = computeFirst(sym);
            bool if_eps = false;
            for (const auto& f : firstS) {
                if (f.value == "") {
                    if_eps = true;
                } else {
                    result.insert(f);
                }
            }
            if (!if_eps) {
                if_all_eps = false;
                break;
            }
        }
        if (if_all_eps) {
            result.insert({"", TERMINAL});
        }
        return result;
    }

    void buildFirstSets() {
        for (const auto& t : start_gram.term) {
            firstSets[t] = {t};
        }

        firstSets[{"$", TERMINAL}] = { {"$", TERMINAL} };
        
        for (const auto& nt : start_gram.non_term) {
            firstSets[nt] = {};
        }

        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& prod : start_gram.productions) {
                SymbolSet currentFirst = comp_first(prod.right);
                size_t oldSize = firstSets[prod.left].size();
                firstSets[prod.left].insert(currentFirst.begin(), currentFirst.end());
                if (firstSets[prod.left].size() > oldSize) {
                    changed = true;
                }
            }
        }
    }

    ItemSet closure(ItemSet items) {
        bool changed = true;
        while (changed) {
            changed = false;
            ItemSet newItems = items;
            
            for (const auto& item : items) {
                if (item.dot < item.prod.right.size()) {
                    Symbol B = item.prod.right[item.dot];
                    if (B.type == NON_TERMINAL) {
                        std::vector<Symbol> beta;
                        for (size_t i = item.dot + 1; i < item.prod.right.size(); ++i) {
                            beta.push_back(item.prod.right[i]);
                        }
                        beta.push_back(item.lookahead);

                        SymbolSet firstBetaA = comp_first(beta);

                        for (const auto& prod : start_gram.getProductionsFor(B)) {
                            for (const auto& term : firstBetaA) {
                                if (term.value == "") {
                                    continue;
                                }
                                LR1Item newItem = {prod, 0, term};
                                if (newItems.find(newItem) == newItems.end()) {
                                    newItems.insert(newItem);
                                    changed = true;
                                }
                            }
                        }
                    }
                }
            }
            items = newItems;
        }
        return items;
    }

    ItemSet go_to(const ItemSet& items, const Symbol& X) {
        ItemSet next_items;
        for (const auto& item : items) {
            if (item.dot < item.prod.right.size() && 
                item.prod.right[item.dot] == X) {
                LR1Item new_item = item;
                new_item.dot++;
                next_items.insert(new_item);
            }
        }
        return closure(next_items);
    }

public:
    LR1Parser() = default;

    void fit(const Grammar& g) {
        gram = g;
        start_gram = gram;
        
        Symbol start_symbol = { "S'", NON_TERMINAL };
        start_gram.non_term.insert(start_symbol);
        
        prod Start_prod;
        Start_prod.id = -1;
        Start_prod.left = start_symbol;
        Start_prod.right = { gram.pr_start_symbol };
        start_gram.productions.insert(start_gram.productions.begin(), Start_prod);

        for(size_t i=0; i<start_gram.productions.size(); ++i) {
            start_gram.productions[i].id = i;
        }

        buildFirstSets();
        std::vector<ItemSet> states;
        std::unordered_map<ItemSet, int, ItemSetHasher> stateToIndex;
        LR1Item startItem = {start_gram.productions[0], 0, {"$", TERMINAL}};
        ItemSet state0 = closure({startItem});
        states.push_back(state0);
        stateToIndex[state0] = 0;
        int processed = 0;
        while (processed < states.size()) {
            ItemSet current = states[processed];
            int currentStateId = processed;
            processed++;
            SymbolSet symbolsAfterDot;
            for (const auto& item : current) {
                if (item.dot < item.prod.right.size()) {
                    symbolsAfterDot.insert(item.prod.right[item.dot]);
                }
            }

            for (const auto& sym : symbolsAfterDot) {
                ItemSet nextState = go_to(current, sym);
                if (nextState.empty()) {continue;}

                int next_state_index;
                auto it = stateToIndex.find(nextState);
                if (it == stateToIndex.end()) {
                    next_state_index = states.size();
                    stateToIndex[nextState] = next_state_index;
                    states.push_back(nextState);
                } else {
                    next_state_index = it->second;
                }

                if (sym.type == TERMINAL) {
                    if (actionTable[currentStateId].count(sym)) {
                         Action existing = actionTable[currentStateId][sym];
                         if (existing.type != SHIFT || existing.stateId != next_state_index) {
                             throw std::runtime_error("НЕ LR(1)");
                         }
                    }
                    actionTable[currentStateId][sym] = {SHIFT, next_state_index, 0};
                } else if (sym.type == NON_TERMINAL) {
                    gotoTable[currentStateId][sym] = next_state_index;
                }
            }
        }

        for (int i = 0; i < states.size(); ++i) {
            for (const auto& item : states[i]) {
                if (item.dot == item.prod.right.size()) {
                    if (item.prod.left.value == "S'") {
                        if (item.lookahead.value == "$") {
                            actionTable[i][{"$", TERMINAL}] = {ACCEPT, 0, 0};
                        }
                    } else {
                        if (actionTable[i].count(item.lookahead)) {
                             throw std::runtime_error("НЕ LR(1)");
                        }
                        actionTable[i][item.lookahead] = {REDUCE, 0, item.prod.id};
                    }
                }
            }
        }
    }

    bool predict(const std::string& wordStr) {
        std::vector<Symbol> input;
        for (char c : wordStr) {
            std::string val(1, c);
            bool is_term = false;
            for (const auto& t : gram.term) {
                if (t.value == val) is_term = true;
            }
            if (!is_term) {
                return false;
            }
            input.push_back({val, TERMINAL});
        }
        input.push_back({"$", TERMINAL});

        std::stack<int> stateStack;
        stateStack.push(0);

        int ip = 0;

        while (1 == 1) {
            int s = stateStack.top();
            if (ip > input.size()) {
                return false;
            }
            
            Symbol a;
            if (ip < input.size()) {
                a = input[ip];
            } else {
                return false;
            }

            if (actionTable[s].find(a) == actionTable[s].end()) {
                return false;
            }
            Action action = actionTable[s][a];
            if (action.type == SHIFT) {
                stateStack.push(action.stateId);
                ip++;
            } else if (action.type == REDUCE) {
                const prod& prod = start_gram.productions[action.prodId];
                size_t popCount = prod.right.size();
                for (size_t k = 0; k < popCount; ++k) {
                    stateStack.pop();
                }
                if (stateStack.empty()) {return false;}
                int t = stateStack.top();
                if (gotoTable[t].find(prod.left) == gotoTable[t].end()) {
                     return false;
                }
                stateStack.push(gotoTable[t][prod.left]);
            } else if (action.type == ACCEPT) {
                return true;
            } else {
                return false;
            }
        }
    }
};

void solve() {
    int non_term;
    int symbs;
    int rules;
    if (!(std::cin >> non_term >> symbs >> rules)) {return;}
    Grammar g;
    
    for (int i = 0; i < non_term; ++i) {
        char c;
        std::cin >> c;
        g.non_term.insert({std::string(1, c), NON_TERMINAL});
    }

    for (int i = 0; i < symbs; ++i) {
        char c;
        std::cin >> c;
        g.term.insert({std::string(1, c), TERMINAL});
    }
    std::cin >> std::ws;

    for (int i = 0; i < rules; ++i) {
        std::string line;
        std::getline(std::cin, line);
        if (!line.empty() && line.back() == '\r') {line.pop_back();}

        size_t arrowPos = line.find("->");
        if (arrowPos == std::string::npos) {continue;}

        std::string left_string = line.substr(0, arrowPos);
        left_string.erase(std::remove(left_string.begin(), left_string.end(), ' '), left_string.end());
        
        std::string right_str = line.substr(arrowPos + 2);
        right_str.erase(std::remove(right_str.begin(), right_str.end(), ' '), right_str.end());

        prod prod;
        prod.left = {left_string, NON_TERMINAL};
        
        if (!right_str.empty()) {
            for (char c : right_str) {
                std::string val(1, c);
                bool is_not_terminal = false;
                for(const auto& nt : g.non_term) {
                    if(nt.value == val) {
                        is_not_terminal = true;
                    }
                }
                if (is_not_terminal) {
                    prod.right.push_back({val, NON_TERMINAL});
                } else {
                    prod.right.push_back({val, TERMINAL});
                }
            }
        }
        g.productions.push_back(prod);
    }

    std::string start_symbol;
    std::cin >> start_symbol;
    g.pr_start_symbol = {start_symbol, NON_TERMINAL};

    LR1Parser parser;
    try {
        parser.fit(g);
    } catch (...) {
        return; 
    }

    int m;
    std::cin >> m;
    std::vector<std::string> results;
    for (int i = 0; i < m; ++i) {
        std::string word;
        std::cin >> word;
        bool res = parser.predict(word);
        results.push_back(res ? "Yes" : "No");
    }

    for (const auto& res : results) {
        std::cout << res << std::endl;
    }
}

#ifndef TEST_MODE
int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    solve();
    return 0;
}
#endif
