#include <iostream>

#include "operator.h"



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

  // Otherwise, we start by evaluating our operands:
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
