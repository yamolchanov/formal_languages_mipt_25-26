#define TEST_MODE 
#include "earley.cpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class Tester {
private:
    int Tests = 0;
    int Passed = 0;

    void RunTest(const std::string& TestName, 
                 const std::string& grammar, 
                 const std::string& word, 
                 bool result
        ) {
        Tests++;
        std::stringstream ss(grammar);
        Grammar g = GrammarReader::readFromStream(ss);
        EarleyParser parser;
        parser.Prepare(g);
        bool parser_ans = parser.Predict(word);

        if (parser_ans == result) {
            Passed++;
            std::cout << "\033[1;32m[OK]\033[0m " << TestName << "\n";
        } else {
            std::cout << "\033[1;31m[FAIL]\033[0m " << TestName << "\n\n";
            std::cout << "Грамматика:\n" << grammar << "\n\n";
            std::cout << "Слово: " << word << "\n\n";
            std::cout << "Ожидали: " << (result ? "Yes" : "No") 
                        << "\nПолучили: " << (parser_ans ? "Yes" : "No") << "\n\n";
        }
    }

public:
    int Stats() {
        if (Passed == Tests) {
            std::cout << "\033[1;35mТесты успешно пройдены. (" << Passed << "/" << Tests << ")\033[0m\n";
            return 0;
        } else {
            std::cout << "\035[1;31mТесты завалены.(" << Passed << "/" << Tests << ")\033[0m\n";
            return -1;
        }
        std::cout << "\n";
    }

    void runAll() {
        testDyck();
        testArithmetic();
        testLValue();
        testLeftRecursion();
        testCyclic();
        testAmbiguitySR();
        testAmbiguityRR();
        testHW1();
        testUseless();
        testInherentlyAmbiguous();
        testHW2Stress();
        testPalindromes();
        testLALR();
        testUnusedNT();
        testStartEps();
        testNoRules();
        testDeepRecursion();
        testEpsilonStorm();
        testNonLinear();
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
    std::cout << "\033[1;35mЗапуск тестов\033\n";
    tester.runAll();
    tester.Stats();
    int exit_code = tester.Stats();
    return exit_code;  
}
