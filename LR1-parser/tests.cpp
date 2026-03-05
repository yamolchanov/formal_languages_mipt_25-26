#define TEST_MODE 
#include "lr1.cpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>

class GrammarReader {
public:
    static Grammar readFromStream(std::istream& in) {
        int non_term_cnt;
        int term_cnt;
        int rules_cnt;
        if (!(in >> non_term_cnt >> term_cnt >> rules_cnt)) {
            throw std::runtime_error("Invalid grammar format: headers");
        }

        Grammar g;
        for (int i = 0; i < non_term_cnt; ++i) {
            std::string val;
            in >> val;
            g.non_term.insert({val, NON_TERMINAL});
        }
        for (int i = 0; i < term_cnt; ++i) {
            std::string val;
            in >> val;
            g.term.insert({val, TERMINAL});
        }
        in >> std::ws;
        for (int i = 0; i < rules_cnt; ++i) {
            std::string line;
            std::getline(in, line);
            if (!line.empty() && line.back() == '\r') {line.pop_back();}
            if (line.empty()) {continue;}

            size_t arrowPos = line.find("->");
            if (arrowPos == std::string::npos) {continue;}
            std::string left_str = line.substr(0, arrowPos);
            left_str.erase(std::remove(left_str.begin(), left_str.end(), ' '), left_str.end());
            std::string rhsStr = line.substr(arrowPos + 2);
            rhsStr.erase(std::remove(rhsStr.begin(), rhsStr.end(), ' '), rhsStr.end());

            prod prod;
            prod.left = {left_str, NON_TERMINAL};
            
            if (!rhsStr.empty()) {
                for (char c : rhsStr) {
                    std::string val(1, c);
                    bool is_non_term = false;
                    for(const auto& nt : g.non_term) {
                        if(nt.value == val) {
                            is_non_term = true;
                            break;
                        }
                    }
                    
                    if (is_non_term) prod.right.push_back({val, NON_TERMINAL});
                    else prod.right.push_back({val, TERMINAL});
                }
            }
            g.productions.push_back(prod);
        }

        std::string start_symbol;
        in >> start_symbol;
        g.pr_start_symbol = {start_symbol, NON_TERMINAL};

        return g;
    }
};

class Tester {
private:
    int Tests = 0;
    int Passed = 0;

    void RunTest(const std::string& TestName, 
                 const std::string& grammar, 
                 const std::string& word, 
                 bool expectedResult
        ) {
        Tests++;
        std::stringstream ss(grammar);
        bool expectAmbiguity = (TestName.find("Amb") != std::string::npos) || 
                               (TestName.find("HW1") != std::string::npos) ||
                               (TestName.find("HW2") != std::string::npos) ||
                               (TestName.find("Palin") != std::string::npos);

        try {
            Grammar g = GrammarReader::readFromStream(ss);
            LR1Parser parser;
            parser.fit(g);
            bool parser_ans = parser.predict(word);
            if (parser_ans == expectedResult) {
                Passed++;
                std::cout << "\033[1;32m[OK]\033[0m " << TestName << "\n";
            } else {
                std::cout << "\033[1;31m[FAIL]\033[0m " << TestName << "\n";
                std::cout << "Ожидали: " << (expectedResult ? "Yes" : "No") 
                          << ", Получили: " << (parser_ans ? "Yes" : "No") << "\n\n";
            }

        } catch (const std::exception& e) {
            std::string err = e.what();
            bool isLr1Error = (err.find("Not LR(1)") != std::string::npos) || 
                              (err.find("НЕ LR(1)") != std::string::npos);
            if (expectAmbiguity && isLr1Error) {
                Passed++;
                std::cout << "\033[1;32m[OK]\033[0m " << TestName << " (Правильно найденная не LR(1) грамматика)\n";
            } 
            else {
                std::cout << "\033[1;31m[FAIL (Exception)]\033[0m " << TestName << "\n";
                std::cout << "Error: " << err << "\n";
                std::cout << "Грамматика:\n" << grammar << "\n\n";
            }
        }
    }

public:
    int Stats() {
        if (Passed == Tests) {
            std::cout << "\n\033[1;35mТесты успешно пройдены. (" << Passed << "/" << Tests << ")\033[0m\n";
            return 0;
        } else {
            std::cout << "\n\033[1;31mТесты завалены (" << Passed << "/" << Tests << ")\033[0m\n";
            return -1;
        }
    }

    void runAll() {
        testDyck();
        testArithmetic();
        testLValue();
        testLeftRecursion();
        testCyclic();
        testHW1();
        testUseless();
        testHW2Stress();
        testPalindromes();
        testLALR();
        testUnusedNT();
        testStartEps();
        testNoRules();
        testDeepRecursion();
        testEpsilonStorm();
        testNonLinear();
        testAmbiguitySR();
        testAmbiguityRR();
        testInherentlyAmbiguous();
    }
    void testDyck() {
        std::string grammar = "1 2 2\nS\n( )\nS ->\nS -> (S)S\nS";
        RunTest("Dyck empty", grammar, "", true);
        RunTest("Dyck ()", grammar, "()", true);
        RunTest("Dyck (())", grammar, "(())", true);
        RunTest("Dyck (()", grammar, "(()", false);
    }
    void testArithmetic() {
        std::string grammar = "3 5 6\nE T F\ni + * ( )\nE -> E+T\nE -> T\nT -> T*F\nT -> F\nF -> (E)\nF -> i\nE";
        RunTest("Arith i+i", grammar, "i+i", true);
        RunTest("Arith (i+i)*i", grammar, "(i+i)*i", true);
        RunTest("Arith i+", grammar, "i+", false);
    }
    void testLValue() {
        std::string grammar = "3 3 5\nS L R\n= * i\nS -> L=R\nS -> R\nL -> *R\nL -> i\nR -> L\nS";
        RunTest("LValue *i=i", grammar, "*i=i", true);
        RunTest("LValue =", grammar, "=", false);
    }
    void testLeftRecursion() {
        std::string grammar = "1 2 2\nS\na b\nS -> Sa\nS -> b\nS";
        RunTest("LeftRec baaa", grammar, "baaa", true);
        RunTest("LeftRec a", grammar, "a", false);
    }
    void testCyclic() {
        std::string grammar = "1 1 2\nS\na\nS -> S\nS -> a\nS";
        RunTest("Cyclic a", grammar, "a", true);
        RunTest("Cyclic aa", grammar, "aa", false);
    }
    void testAmbiguitySR() {
        std::string grammar = "1 2 2\nS\na b\nS -> SaS\nS -> b\nS";
        RunTest("AmbSR bab", grammar, "bab", true);
    }
    void testAmbiguityRR() {
        std::string grammar = "3 1 4\nS A B\nx\nS -> A\nS -> B\nA -> x\nB -> x\nS";
        RunTest("AmbRR x", grammar, "x", true);
    }
    void testHW1() {
        std::string gFull = "7 2 12\nS T Q A B C D\na b\nS -> T\nS -> Q\nT -> CD\nQ -> AB\nA -> aA\nA -> aa\nB -> aBb\nB -> \nC -> aCb\nC -> \nD -> bD\nD -> \nS";
        RunTest("HW1 ab", gFull, "ab", true);
        RunTest("HW1 bab", gFull, "bab", false);
    }
    void testUseless() {
        std::string grammar = "1 1 2\nS\na\nS -> S\nS -> a\nS";
        RunTest("Useless a", grammar, "a", true);
    }
    void testInherentlyAmbiguous() {
        std::string grammar = "1 3 3\nE\nn + *\nE -> E+E\nE -> E*E\nE -> n\nE";
        RunTest("InhAmb n+n+n", grammar, "n+n+n", true);
    }
    void testHW2Stress() {
         std::string grammar = "1 2 7\nS\na b\nS -> \nS -> SS\nS -> aSb\nS -> bSa\nS -> aSaSb\nS -> aSbSa\nS -> bSaSa\nS";
        RunTest("HW2 aab", grammar, "aab", true);
        RunTest("HW2 aaaab", grammar, "aaaab", false);
    }
    void testPalindromes() {
        std::string grammar = "1 2 5\nS\na b\nS -> aSa\nS -> bSb\nS -> \nS -> a\nS -> b\nS";
        RunTest("Palin abba", grammar, "abba", true);
        RunTest("Palin ab", grammar, "ab", false);
    }
    void testLALR() {
        std::string grammar = "3 5 6\nS A B\na b c d e\nS -> aAd\nS -> aBe\nS -> bAe\nS -> bBd\nA -> c\nB -> c\nS";
        RunTest("LALR acd", grammar, "acd", true);
        RunTest("LALR ac", grammar, "ac", false);
    }
    void testUnusedNT() {
        std::string grammar = "2 2 2\nS A\na b\nS -> a\nA -> b\nS";
        RunTest("Unused a", grammar, "a", true);
        RunTest("Unused b", grammar, "b", false);
    }
    void testStartEps() {
        std::string grammar = "1 0 1\nS\n\nS -> \nS";
        RunTest("StartEps empty", grammar, "", true);
        RunTest("StartEps a", grammar, "a", false);
    }
    void testNoRules() {
        std::string grammar = "1 1 0\nS\na\n\nS";
        RunTest("NoRules a", grammar, "a", false);
    }
    void testDeepRecursion() {
        std::string grammar = "1 2 2\nS\na b\nS -> aSb\nS -> \nS";
        std::string passWord(300, 'a'); passWord += std::string(300, 'b');
        std::string failWord = passWord + "b";
        RunTest("DeepRec 300", grammar, passWord, true);
        RunTest("DeepRec 300fail", grammar, failWord, false);
    }
    void testEpsilonStorm() {
        std::string grammar = "7 3 7\nS A B C D E F\na b c\nS -> ABCDEF\nA -> \nB -> \nC -> \nD -> \nE -> \nF -> \nS";
        RunTest("EpsStorm empty", grammar, "", true);
        RunTest("EpsStorm a", grammar, "a", false);
    }
    void testNonLinear() {
        std::string grammar = "1 2 2\nS\n{ }\nS -> {S}S\nS -> \nS";
        RunTest("NonLin {{}}{}", grammar, "{{}}{}", true);
        RunTest("NonLin {", grammar, "{", false);
    }
};

int main() {
    Tester tester;
    std::cout << "\033[1;35mЗапуск тестов\033[0m\n";
    tester.runAll();
    int exit_code = tester.Stats();
    return exit_code;  
}
