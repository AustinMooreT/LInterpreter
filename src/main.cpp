#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cctype>
#include <algorithm>
#include <memory>


class Variable {
public:
  enum DataType {
                 INT,
                 BOOL,
                 STRING,
                 UNDEFINED
  };
private:
  std::string symbol;
  DataType dt;
  int integerValue;
  bool booleanValue;
  std::string stringValue;

public:
  Variable() {
    this->dt = UNDEFINED;
  }
  Variable(const std::string& str) {
    this->dt = STRING;
    this->stringValue = str;
  }
  Variable(const std::string& str, const std::string& sym) {
    this->dt = STRING;
    this->stringValue = str;
    this->symbol = sym;
  }
  Variable(int num) {
    this->dt = INT;
    this->integerValue = num;
  }
  Variable(int num, const std::string& sym) {
    this->dt = INT;
    this->integerValue = num;
    this->symbol = sym;
  }
  Variable(bool boolean) {
    this->dt = BOOL;
    this->booleanValue = boolean;
  }
  Variable(bool boolean, const std::string& sym) {
    this->dt = BOOL;
    this->booleanValue = boolean;
    this->symbol = sym;
  }

  Variable(const Variable& v) {
    this->dt = v.dt;
    this->symbol = v.symbol;
    this->integerValue = v.integerValue;
    this->booleanValue = v.booleanValue;
    this->stringValue = v.stringValue;
  }

  void setSymbol(const std::string& str) {
    this->symbol = str;
  }

  std::string toString() {
    switch(this->dt) {
    case INT : return std::to_string(integerValue);
    case BOOL : return booleanValue ? "TRUE" : "FALSE";
    case STRING : return stringValue;
    case UNDEFINED : return "";
    };
  }

  bool isDefined() const {
    return this->dt != UNDEFINED;
  }
  std::string getSymbol() {
    return this->symbol;
  }

  bool operator=(const Variable& v) {
    switch(v.dt) {
    case INT : this->dt = INT; this->integerValue = v.integerValue; return true;
    case BOOL : this->dt = BOOL; this->booleanValue = v.booleanValue; return true;
    case STRING : this->dt = STRING; this->stringValue = v.stringValue; return true;
    case UNDEFINED : return false;
    }
  }
};


typedef std::unordered_map<std::string, Variable> State;

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
  virtual ExpressionStatus operator()() = 0;
};


/**
   A Statement is a single line of L Code.
   For example: A = 4 ;
   Even expressions such as
   FOR 4 { A += 1 ; PRINT A } ;
   are considerd a single Statement.
 */
class Statement {
private:
  std::vector<std::shared_ptr<Expression>> exps;
public:
  void addExpression(std::shared_ptr<Expression> exp) {
    this->exps.push_back(exp);
  }
  virtual std::string toString(int lineNum) {
    std::string ret = "";
    for(auto& exp : exps) {
      auto status = (*exp)();
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
    return this->reason + " LINE " + std::to_string(lineNum);
  }
};


//increasing a vector by a scalar.
//std::transform(myv1.begin(), myv1.end(), myv1.begin(),
//               std::bind(std::multiplies<T>(), std::placeholder::_1, 3));

std::shared_ptr<Statement> parsePrint(State& state, std::string sym) {
  auto var = state[sym];
  
  if(var.isDefined()) {
    class Print : public Expression {
    private:
      Variable data;
    public:
      Print(Variable d) {
        this->data = d;
      }
      virtual ExpressionStatus operator()() {
        return ExpressionStatus(ExpressionStatus::SUCCESS, this->data.getSymbol() + "=" + this->data.toString());
      }
    };

    std::shared_ptr<Expression> exp = std::make_shared<Print>(var);
    auto s = std::make_shared<Statement>();
    s->addExpression(exp);
    return s;
  }
  auto s = std::make_shared<InvalidStatement>("RUNTIME ERROR SYMBOL " + sym + " NOT DEFINED");
  return s; 
}


Variable parseLiteral(const std::string& str) {
  if(str == "TRUE") {
    return Variable(true);
  }
  else if(str == "FALSE") {
    return Variable(false);
  }
  else if(std::all_of(str.begin(), str.end(), std::function<int(int)>(isdigit))) {
    return Variable(std::stoi(str));
  }
  else return Variable();
}


std::shared_ptr<Statement> parseAssignment(State& state, std::string op, std::string leftOperand, std::string rightOperand) {
  auto leftVar = state[leftOperand];
  leftVar.setSymbol(leftOperand);
  Variable rightVar;
  if(std::all_of(rightOperand.begin(), rightOperand.end(), std::function<int(int)>(isupper))) {
    rightVar = state[rightOperand];
  }
  else {
    rightVar = parseLiteral(rightOperand);
  }

  class Assignment : public Expression {
  protected:
    State* state;
    Variable left;
    Variable right;
  public:
    Assignment(State& state, Variable left, Variable right) {
      this->state = &state;
      this->left = left;
      this->right = right;
    }
  };

  if(op == "=") {
    class BasicAssignment : public Assignment {
    public:
      BasicAssignment(State& state, Variable left, Variable right) : Assignment(state, left, right){}
      ExpressionStatus operator()() {
        if(!(this->left = this->right)) {
          return ExpressionStatus(ExpressionStatus::FAILURE, "RUNTIME ERROR: FAILED TO ASSIGN " + this->left.getSymbol());
        }
        (*(this->state))[this->left.getSymbol()] = left;
        return ExpressionStatus(ExpressionStatus::SUCCESS, "");
      }
    };
    std::shared_ptr<Expression> b = std::make_shared<BasicAssignment>(state, leftVar, rightVar);
    auto s = std::make_shared<Statement>();
    s->addExpression(b);
    return s;
  }
}


/**
   Given a vector of strings parses it into a statement object.
 */
std::shared_ptr<Statement> parseStatement(const std::vector<std::string>& words, State& state) {
  if(words.back() != ";") {
    return std::make_shared<InvalidStatement>("RUNTIME ERROR: EXPECTED ; ");
  }
  for(auto word = words.begin(); word != words.end(); word++) {
    if(*word == "PRINT") {
      if(++word == words.end()) {
        return std::make_shared<InvalidStatement>("RUNTIME ERROR: EXPECTED VARIABLE ");
      }
      return parsePrint(state, *word);
    }
    else {
      auto left = *word;
      if(++word == words.end()) {
        return std::make_shared<InvalidStatement>("RUNTIME ERROR: EXPECTED VARIABLE ");
      }

      auto op = *word;
      if(++word == words.end()) {
        return std::make_shared<InvalidStatement>("RUNTIME ERROR: EXPECTED VARIABLE ");
      }
      auto right = *word;
      return parseAssignment(state, op, left, right);
    }
  }
}

/*
  Parses a string into a vector of words using space as a delimiter.
*/
std::vector<std::string> parseString(std::string str) {
  std::stringstream stream(str);
  std::vector<std::string> words;
  std::string temp;
  while(stream >> temp) {
    words.push_back(temp);
  }
  return words;
}


int main() {
  std::string line;
  State state;
  unsigned long long int currentLine = 1;
  while(std::getline(std::cin, line)) {
    auto words = parseString(line);
    //TODO remove this
    auto temp = parseStatement(words, state)->toString(currentLine++);
    std::cout << temp;
  }
}
