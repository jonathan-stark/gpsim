#include <iostream>

#include "operator.h"
#include "errors.h"


BinaryOperator::BinaryOperator(string  opString, 
			       Expression* _leftExpr, 
			       Expression* _rightExpr) : Operator(opString)
{
  leftExpr = _leftExpr;
  rightExpr = _rightExpr;
  value = 0;
}


BinaryOperator::~BinaryOperator()
{
  delete leftExpr;
  delete rightExpr;
  delete value;
}


Value* BinaryOperator::shortCircuit(Value* leftValue)
{
  return 0;
}

string BinaryOperator::show()
{
  return string(showType() + ":" + toString());
}

string BinaryOperator::showType()
{
  return showOp();
}


string BinaryOperator::toString()
{
  return string("(" + leftExpr->show() + ", " + rightExpr->show() + ")");
}

Value *BinaryOperator::evaluate()
{

  Value* lVal;
  Value* rVal;
  Value* tmp;

  cout << toString() << endl;

  lVal = leftExpr->evaluate();

  // If this operator wants to make a short-circuit decision
  // based on our evaluated left operand, we let that operator
  // class make its decision right now before we eval the right
  // operand:
  //  tmp = shortCircuit(lVal);
  //  if (tmp) {
  //    return(tmp);
  //  }

  rVal = rightExpr->evaluate();

  // Apply the specific operator to the evaluated operands:
  tmp = applyOp(lVal, rVal);

  // If the result is constant, save the result for future use:
  //if (tmp->isConstant()) {
  //  value = tmp;
  //}

  return tmp;
}

/*****************************************************************
 * The basic unary operator class.
 */
UnaryOperator::UnaryOperator(string theOpString, Expression* expr_)
  : Operator(theOpString)
{
  expr = expr_;
  value = 0;
}

UnaryOperator::~UnaryOperator()
{
}


string UnaryOperator::showType()
{
  return showOp();
}


string UnaryOperator::show()
{
  return string(showType() + " (" + expr->show() + ")");
}


string UnaryOperator::toString()
{
  return string("(" + expr->show() + ")");
}



Value* UnaryOperator::evaluate()
{
  Value* tmp;

  // start evaluating our operand expression:
  tmp = expr->evaluate();

  // Apply the specific operator to the evaluated operand:
  tmp = applyOp(tmp);

  // If the result is constant, save the result for future use:
  //if (tmp->isConstant()) {
  //  value = tmp;
  //}

  return tmp;
}

/*****************************************************************
 * Comparison operators
 */
ComparisonOperator::ComparisonOperator(string opString, 
				       Expression* leftExpr,
				       Expression* rightExpr)
  : BinaryOperator(opString,leftExpr,rightExpr), 
    bLess(false), bEqual(false), bGreater(false)
{
}

ComparisonOperator:: ~ComparisonOperator()
{
}

Value* ComparisonOperator::applyOp(Value* leftValue, Value* rightValue)
{
  return new Boolean(leftValue->compare(this,rightValue));
}

/******************************************************************************
 Operator: AbstractRange 
 
 *****************************************************************************/
OpAbstractRange::OpAbstractRange(Expression *lVal, Expression *rVal)
  : BinaryOperator(":", lVal, rVal)
{
}


OpAbstractRange::~OpAbstractRange()
{
}


Value* OpAbstractRange::applyOp(Value* lVal, Value* rVal)
{
  Value* result=0;

  Integer* lInteger = Integer::typeCheck(lVal, showOp());
  Integer* rInteger = Integer::typeCheck(rVal, showOp());
  
  int left = lInteger->getVal();
  int right = rInteger->getVal();
  
  result = new AbstractRange(left, right);
  return(result);
}

//------------------------------------------------------------------------


OpAdd::OpAdd(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("+",leftExpr,rightExpr)
{
}

OpAdd::~OpAdd()
{
}

Value *OpAdd::applyOp(Value *lval, Value *rval)
{

  return new Integer(lval->getAsInt() + rval->getAsInt());

}
//------------------------------------------------------------------------


OpAnd::OpAnd(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("&",leftExpr,rightExpr)
{
}

OpAnd::~OpAnd()
{
}

Value *OpAnd::applyOp(Value *lval, Value *rval)
{

  return new Integer(lval->getAsInt() & rval->getAsInt());

}

//------------------------------------------------------------------------
OpEq::OpEq(Expression* leftExpr, Expression* rightExpr)
  : ComparisonOperator("==",leftExpr,rightExpr)
{
  bEqual = true;
}

OpEq::~OpEq()
{
}

//------------------------------------------------------------------------
OpGe::OpGe(Expression* leftExpr, Expression* rightExpr)
  : ComparisonOperator(">=",leftExpr,rightExpr)
{
  bEqual = true;
  bGreater = true;
}

OpGe::~OpGe()
{
}

//------------------------------------------------------------------------
OpGt::OpGt(Expression* leftExpr, Expression* rightExpr)
  : ComparisonOperator(">",leftExpr,rightExpr)
{
  bGreater = true;
}

OpGt::~OpGt()
{
}

//------------------------------------------------------------------------
OpLe::OpLe(Expression* leftExpr, Expression* rightExpr)
  : ComparisonOperator("<=",leftExpr,rightExpr)
{
  bLess = true;
  bEqual = true;
}

OpLe::~OpLe()
{
}
//------------------------------------------------------------------------
OpLt::OpLt(Expression* leftExpr, Expression* rightExpr)
  : ComparisonOperator("<",leftExpr,rightExpr)
{
  bLess = true;
}

OpLt::~OpLt()
{
}

//------------------------------------------------------------------------
OpLogicalAnd::OpLogicalAnd(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("&&",leftExpr,rightExpr)
{
}

OpLogicalAnd::~OpLogicalAnd()
{
}

Value *OpLogicalAnd::applyOp(Value* leftValue, Value* rightValue)
{
  return new Boolean(leftValue && rightValue);
}

//------------------------------------------------------------------------
OpLogicalOr::OpLogicalOr(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("||",leftExpr,rightExpr)
{
}

OpLogicalOr::~OpLogicalOr()
{
}

Value *OpLogicalOr::applyOp(Value* leftValue, Value* rightValue)
{
  return new Boolean(leftValue || rightValue);
}


//------------------------------------------------------------------------
OpNe::OpNe(Expression* leftExpr, Expression* rightExpr)
  : ComparisonOperator("!=",leftExpr,rightExpr)
{
  bLess = true;
  bGreater = true;
}

OpNe::~OpNe()
{
}


//------------------------------------------------------------------------


OpSub::OpSub(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("-",leftExpr,rightExpr)
{
}

OpSub::~OpSub()
{
}

Value *OpSub::applyOp(Value *lval, Value *rval)
{

  return new Integer(lval->getAsInt() - rval->getAsInt());

}

//------------------------------------------------------------------------


OpMpy::OpMpy(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("*",leftExpr,rightExpr)
{
}

OpMpy::~OpMpy()
{
}

Value *OpMpy::applyOp(Value *lval, Value *rval)
{

  return new Integer(lval->getAsInt() * rval->getAsInt());
}

//------------------------------------------------------------------------


OpOr::OpOr(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("|",leftExpr,rightExpr)
{
}

OpOr::~OpOr()
{
}

Value *OpOr::applyOp(Value *lval, Value *rval)
{

  return new Integer(lval->getAsInt() | rval->getAsInt());

}

//------------------------------------------------------------------------


OpXor::OpXor(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("^",leftExpr,rightExpr)
{
}

OpXor::~OpXor()
{
}

Value *OpXor::applyOp(Value *lval, Value *rval)
{

  return new Integer(lval->getAsInt() ^ rval->getAsInt());

}

//------------------------------------------------------------------------


OpDiv::OpDiv(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("/",leftExpr,rightExpr)
{
}

OpDiv::~OpDiv()
{
}

Value *OpDiv::applyOp(Value *lval, Value *rval)
{
  int i = rval->getAsInt();

  if(i == 0)
    throw new Error("Operator " + showOp() + " Divide by 0");

  return new Integer(lval->getAsInt() / i);

}

//------------------------------------------------------------------------


OpShl::OpShl(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("<<",leftExpr,rightExpr)
{
}

OpShl::~OpShl()
{
}

Value *OpShl::applyOp(Value *lval, Value *rval)
{

  int i = rval->getAsInt();

  if(i < 0  || i > 31)
    throw new Error("Operator " + showOp() + " bad shift count");

  return new Integer(lval->getAsInt() << i);

}

//------------------------------------------------------------------------


OpShr::OpShr(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator(">>",leftExpr,rightExpr)
{
}

OpShr::~OpShr()
{
}

Value *OpShr::applyOp(Value *lval, Value *rval)
{

  int i = rval->getAsInt();

  if(i < 0  || i > 31)
    throw new Error("Operator " + showOp() + " bad shift count");

  return new Integer(lval->getAsInt() >> i);

}


/******************************************************************************
 The logical NOT operator '!'.
******************************************************************************/
OpLogicalNot::OpLogicalNot(Expression* expr)
  : UnaryOperator("!", expr)
{
}

OpLogicalNot::~OpLogicalNot()
{
}

Value* OpLogicalNot::applyOp(Value* operand)
{
  Boolean* op;
  bool bVal;

  op = Boolean::typeCheck(operand, showOp());
  bVal = op->getVal();
  return new Boolean(!bVal, op->isConstant());
}

/******************************************************************************
 The unary 'negate' operator.
******************************************************************************/
OpNegate::OpNegate(Expression* expr)
  : UnaryOperator("-", expr)
{
}

OpNegate::~OpNegate()
{
}


Value* OpNegate::applyOp(Value* operand)
{
  Value* rVal=0;

  if (typeid(*operand) == typeid(Integer)) {
    Integer* iOp = (Integer*)(operand);
    rVal = new Integer(-(iOp->getVal()));
  }
  else if (typeid(*operand) == typeid(Float)) {
    Float* fOp = (Float*)(operand);
    rVal = new Float(-(fOp->getVal()));
  }
  else {
    throw new TypeMismatch(showOp(), operand->showType());
  }

  return rVal;
}

/******************************************************************************
 The unary ones complement operator '~'.
******************************************************************************/
OpOnescomp::OpOnescomp(Expression* expr)
  : UnaryOperator("~", expr)
{
}

OpOnescomp::~OpOnescomp()
{
}


Value* OpOnescomp::applyOp(Value* operand)
{
  Integer* op;
  
  op = Integer::typeCheck(operand, showOp());  
  return new Integer(~(op->getVal()), op->isConstant());
}

/******************************************************************************
 The unary 'plus' operator.
******************************************************************************/
OpPlus::OpPlus(Expression* expr)
  : UnaryOperator("+", expr)
{
}

OpPlus::~OpPlus()
{
}


Value* OpPlus::applyOp(Value* operand)
{
  Value* rVal=0;

  if (typeid(*operand) == typeid(Integer)) {
    Integer* iOp = (Integer*)(operand);
    rVal = new Integer(iOp->getVal());
  }
  else if (typeid(*operand) == typeid(Float)) {
    Float* fOp = (Float*)(operand);
    rVal = new Float(fOp->getVal());
  }
  else {
    throw new TypeMismatch(showOp(), operand->showType());
  }

  return rVal;
}

