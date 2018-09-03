#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <functional>


class Variable {
public:
  enum DataType {
                 INT,
                 BOOL,
                 STRING,
                 UNDEFINED
  };

  Variable() {
    this->dt = UNDEFINED;
  }

private:
  std::string symbol;
  DataType dt;
  int integerValue;
  bool booleanValue;
  std::string stringValue;

public:
  std::string toString() {
    return "";
  }
  bool isDefined() {
    return this->dt != UNDEFINED;
  }
  std::string getSymbol() {
    return this->symbol;
  }
};


class State {
private:
  std::unordered_map<std::string, Variable> variables;
public:
  Variable operator()(std::string var) {
    return this->variables[var];
  }
};

class Operation {
  Variable leftOperand;
  Variable rightOperand;
  std::function<Variable(const Variable,const Variable)> op;
  Variable operator()(){
    return op(leftOperand, rightOperand);
  }
};

class ExpressionStatus {
public:
  enum Status {
               SUCCESS,
               FAILURE
  };
private:
  Status status;
  std::string message;
public:
  ExpressionStatus(ExpressionStatus::Status status, std::string message) {
    this->status = status;
    this->message = message;
  }
  ExpressionStatus::Status getStatus() {
    return status;
  }
  std::string getMessage() {
    return message;
  }
};

class Expression {
public:
  ExpressionStatus operator()();
};

class Statement {
private:
  std::vector<Expression> exps;
public:
  virtual std::string toString(int lineNum) {
    std::string ret = "";
    for(auto& exp : exps) {
      auto status = exp();
      ret += status.getMessage();
      if(status.getStatus() == ExpressionStatus::FAILURE) {
        ret += " LINE " + std::to_string(lineNum) + "\n";
        break;
      }
    }
    return ret;
  };
};

class InvalidStatement : public Statement {
private:
  std::string reason;
public:
  InvalidStatement(std::string reason) {
    this->reason = reason;
  }
  virtual std::string toString(int lineNum){
    return this->reason + std::to_string(lineNum);
  }
};


//increasing a vector by a scalar.
//std::transform(myv1.begin(), myv1.end(), myv1.begin(),
//               std::bind(std::multiplies<T>(), std::placeholder::_1, 3));

Statement parseStatement(const std::vector<std::string>& words, State state) {
  if(words.back() != ";") {
    return InvalidStatement("RUNTIME ERROR: EXPECTED ; ");
  }
  for(auto word = words.begin(); word != words.end(); word++) {
    if(*word == "PRINT") {
      auto operation = [](Variable v) {std::cout << v.toString() << std::endl;};
      auto var = state(*++word);
      if(var.isDefined()) {
        operation(var);
      }
      return InvalidStatement("RUNTIME ERROR: " + var.getSymbol() + " IS UNDEFINED ");
    }
  }
}

/*
  Parses a string into a vector of words using space as a delimiter.
*/
std::vector<std::string> parseString(std::string& str) {
  std::stringstream stream(str);
  std::vector<std::string> words;
  std::string temp;
  while(stream >> temp) {
    words.push_back(temp);
  }
  return words;
}
