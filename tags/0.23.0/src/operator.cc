#include <iostream>
#include <cstdio>

#include "operator.h"
#include "errors.h"
#include "symbol.h"
#include "processor.h"

#include <typeinfo>

static bool isFloat(Value *v)
{
  if(v &&  (typeid(*v) == typeid(Float)))
    return true;
  return false;
}
static bool isInteger(Value *v)
{
  if(v &&  (typeid(*v) == typeid(Integer)))
    return true;
  return false;
}
static bool isBoolean(Value *v)
{
  if(v &&  (typeid(*v) == typeid(Boolean)))
    return true;
  return false;
}

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
  return toString();
}

string BinaryOperator::showType()
{
  return showOp();
}


string BinaryOperator::toString()
{
  return string("(" + leftExpr->toString() + showOp() + rightExpr->toString() + ")");
}

Value *BinaryOperator::evaluate()
{

  return applyOp(leftExpr->evaluate(), rightExpr->evaluate());
}
Expression *BinaryOperator::getLeft() {
  return leftExpr;
}

Expression *BinaryOperator::getRight() {
  return rightExpr;
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
  delete expr;
}


string UnaryOperator::showType()
{
  return showOp();
}


string UnaryOperator::show()
{
  return toString();
}


string UnaryOperator::toString()
{
  return string(showOp() + "(" + expr->toString() + ")");
}



Value* UnaryOperator::evaluate()
{
  Value* tmp, *vReturn;

  // start evaluating our operand expression:
  tmp = expr->evaluate();

  // Apply the specific operator to the evaluated operand:
  vReturn = applyOp(tmp);

  // If the result is constant, save the result for future use:
  //if (tmp->isConstant()) {
  //  value = tmp;
  //}
  delete tmp;
  return vReturn;
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
int ComparisonOperator::set_break(ObjectBreakTypes bt, ObjectActionTypes at, Expression *pExpr)
{
  return get_bp().set_break(bt, at, (Register *)0, this);

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
  
  int left = (int)lInteger->getVal();
  int right = (int)rInteger->getVal();
  
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
  if(isFloat(lval) || isFloat(rval)) {
    double d1,d2;

    lval->get(d1);
    rval->get(d2);

    return new Float(d1+d2);
  }

  // Try to add as integers. (An exception is thrown if the values
  // cannot be type casted.

  gint64 i1,i2;

  lval->get(i1);
  rval->get(i2);

  return new Integer(i1+i2);

  //throw new TypeMismatch(showOp(), lval->showType(), rval->showType());
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

  if(isFloat(lval) || isFloat(rval))
    throw new TypeMismatch(showOp(), lval->showType(), rval->showType());

  gint64 i1,i2;

  lval->get(i1);
  rval->get(i2);

  return new Integer(i1 & i2);

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

Value *OpLogicalAnd::applyOp(Value* lval, Value* rval)
{
  if(isBoolean(lval) && isBoolean(rval)) {
    bool b1 = (static_cast<Boolean *>(lval))->getVal();
    bool b2 = (static_cast<Boolean *>(rval))->getVal();

    return new Boolean(b1 & b2);
  }

  throw new TypeMismatch(showOp(), lval->showType(), rval->showType());

}

//------------------------------------------------------------------------
OpLogicalOr::OpLogicalOr(Expression* leftExpr, Expression* rightExpr)
  : BinaryOperator("||",leftExpr,rightExpr)
{
}

OpLogicalOr::~OpLogicalOr()
{
}

Value *OpLogicalOr::applyOp(Value* lval, Value* rval)
{
  if(isBoolean(lval) && isBoolean(rval)) {
    bool b1 = (static_cast<Boolean *>(lval))->getVal();
    bool b2 = (static_cast<Boolean *>(rval))->getVal();

    return new Boolean(b1 | b2);
  }

  throw new TypeMismatch(showOp(), lval->showType(), rval->showType());

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
  if(isFloat(lval) || isFloat(rval)) {
    double d1,d2;

    lval->get(d1);
    rval->get(d2);

    return new Float(d1-d2);
  }

  gint64 i1,i2;

  lval->get(i1);
  rval->get(i2);

  return new Integer(i1-i2);

  //  throw new TypeMismatch(showOp(), lval->showType(), rval->showType());

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
  if(isFloat(lval) || isFloat(rval)) {
    double d1,d2;

    lval->get(d1);
    rval->get(d2);

    return new Float(d1*d2);
  }

  gint64 i1,i2;

  lval->get(i1);
  rval->get(i2);

  return new Integer(i1*i2);

  throw new TypeMismatch(showOp(), lval->showType(), rval->showType());
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
  if(isFloat(lval) || isFloat(rval))
    throw new TypeMismatch(showOp(), lval->showType(), rval->showType());

  gint64 i1,i2;

  lval->get(i1);
  rval->get(i2);

  return new Integer(i1 | i2);
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

  if(isFloat(lval) || isFloat(rval))
    throw new TypeMismatch(showOp(), lval->showType(), rval->showType());

  gint64 i1,i2;

  lval->get(i1);
  rval->get(i2);

  return new Integer(i1 ^ i2);
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

  if(isFloat(lval) || isFloat(rval)) {
    double d1,d2;

    lval->get(d1);
    rval->get(d2);

    if(d2 == 0.0)
      throw new Error("Divide by zero");

    return new Float(d1/d2);
  }

  gint64 i1,i2;

  lval->get(i1);
  rval->get(i2);

  if(i2 == 0)
    throw new Error("Divide by zero");

  return new Integer(i1/i2);

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

  if(isFloat(lval) || isFloat(rval))
    throw new TypeMismatch(showOp(), lval->showType(), rval->showType());

  gint64 i1;
  gint64 i2;

  rval->get(i2);

  if(i2 < 0  || i2 > 63)
    throw new Error("Operator " + showOp() + " bad shift count");

  lval->get(i1);

  return new Integer(i1<<i2);
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

  if(isFloat(lval) || isFloat(rval))
    throw new TypeMismatch(showOp(), lval->showType(), rval->showType());

  gint64 i1;
  gint64 i2;

  rval->get(i2);

  if(i2 < 0  || i2 > 63)
    throw new Error("Operator " + showOp() + " bad shift count");

  lval->get(i1);

  return new Integer(i1>>i2);
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
  return new Boolean(!bVal);
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

  if (isInteger(operand)) {
    Integer* iOp = (Integer*)(operand);
    rVal = new Integer(-(iOp->getVal()));
  }
  else if (isFloat(operand)) {
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
  return new Integer(~ op->getVal() );
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

  if (isInteger(operand)) {
    Integer* iOp = (Integer*)(operand);
    rVal = new Integer(iOp->getVal());
  }
  else if (isFloat(operand) ) {
    Float* fOp = (Float*)(operand);
    rVal = new Float(fOp->getVal());
  }
  else {
    throw new TypeMismatch(showOp(), operand->showType());
  }

  return rVal;
}

/******************************************************************************
 The unary '*' operator - indirect access
******************************************************************************/
OpIndirect::OpIndirect(Expression* expr)
  : UnaryOperator("*", expr)
{
}

OpIndirect::~OpIndirect()
{
}


Value* OpIndirect::applyOp(Value* operand)
{
  Value* rVal=0;
  if (isInteger(operand)) {
    Integer* iOp = (Integer*)(operand);
    // hmm ... what about ema? Need a different indirection operator?
    Register *pReg = get_active_cpu()->rma.get_register(*iOp);
    if(pReg) {
      rVal = new Integer(pReg->get());
    }
    else {
      static char sFormat[] = "Value $%x is an invalid memory address";
      char sMsg[sizeof(sFormat) + 10];
      sprintf(sMsg, sFormat, (int)iOp->getVal());
      throw new Error(string(sMsg));
    }
  }
  else if (isFloat(operand) ) {
    Float* fOp = (Float*)(operand);
    rVal = new Float(fOp->getVal());
  }
  else {
    throw new TypeMismatch(showOp(), operand->showType());
  }

  return rVal;
}

/******************************************************************************
 The unary '*' operator - indirect access
******************************************************************************/
OpAddressOf::OpAddressOf(Expression* expr)
  : UnaryOperator("&", expr)
{
}

OpAddressOf::~OpAddressOf()
{
}

Value* OpAddressOf::evaluate()
{
  Value* tmp;

  // start evaluating our operand expression:
  LiteralSymbol *pSym = dynamic_cast<LiteralSymbol *>(expr);
  if (pSym) {
    tmp = applyOp(pSym->GetSymbol());
  }
  else {
    throw new TypeMismatch(showOp(), expr->showType());
  }
  return tmp;
}

Value* OpAddressOf::applyOp(Value* operand)
{
  Value* rVal=0;

  /*
  register_symbol *pReg =dynamic_cast<register_symbol*>(operand);
  if (pReg) {
    rVal = new Integer(pReg->getAddress());
  }
  else {
    AliasedSymbol *pSym = dynamic_cast<AliasedSymbol*>(operand);
    if (pSym) {
      pReg = dynamic_cast<register_symbol*>(pSym);
      if (pReg) {
        rVal = new Integer(pReg->getAddress());
      }
    }
  }
  */
  Register *pReg = dynamic_cast<Register*>(operand);
  rVal = pReg ? new Integer(pReg->getAddress()) : 0;
  if(rVal==0) {
    throw new TypeMismatch(showOp(), operand->showType());
  }
  return rVal;
}

