////
//// Created by yuval on 11/6/19.
////

#ifndef UNTITLED_INTERPRETER_H
#define UNTITLED_INTERPRETER_H

#include <string>
#include <map>
#include <algorithm>
#include "Expression.h"

class Interpreter {
private:
    map< string, string > variables;
public:
    void tokenToMap(string* token);
    Expression* interpret(string exp);
    void setVariables(string var);
    ~Interpreter();
};

#endif //UNTITLED_INTERPRETER_H