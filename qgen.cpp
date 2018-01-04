/*
 * This file is part of Ample.
 *
 * Ample is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ample is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ample.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qgen.h"

QGen::~QGen()
{
}

QMap<QString, QGen::UserOperator> QGen::userOperators = QMap<QString, QGen::UserOperator>();

QGen QGen::makeSymb(const giac::unary_function_ptr *p, const giac::gen &args) const
{
    return QGen(giac::symbolic(p, args), context);
}

bool QGen::isNegativeConstant() const
{
    return  (isInteger() && integerValue() < 0) ||
            (isDouble() && doubleValue() < 0) ||
            (isRational() && approximateValue() > 0);
}

bool QGen::isElementary() const {
    return  isUnaryFunction(giac::at_exp) || isUnaryFunction(giac::at_ln) || isUnaryFunction(giac::at_log10) ||
            isUnaryFunction(giac::at_sin) || isUnaryFunction(giac::at_cos) || isUnaryFunction(giac::at_tan) ||
            isUnaryFunction(giac::at_sec) || isUnaryFunction(giac::at_csc) || isUnaryFunction(giac::at_cot) ||
            isUnaryFunction(giac::at_asin) || isUnaryFunction(giac::at_acos) || isUnaryFunction(giac::at_atan) ||
            isUnaryFunction(giac::at_acot) || isUnaryFunction(giac::at_sinh) || isUnaryFunction(giac::at_cosh) ||
            isUnaryFunction(giac::at_tanh) || isUnaryFunction(giac::at_asinh) || isUnaryFunction(giac::at_acosh) ||
            isUnaryFunction(giac::at_atanh) || isUnaryFunction(giac::at_det) || isUnaryFunction(giac::at_det_minor);
}

int QGen::operandCount() const
{
    return isSymbolic() && _SYMBptr->feuille.type == giac::_VECT ? int(_SYMBptr->feuille._VECTptr->size()) : -1;
}

QGen QGen::firstOperand() const
{
    if (!isSymbolic() || operandCount() < 1)
        return undefined();
    return QGen(_SYMBptr->feuille._VECTptr->front(), context);
}

QGen QGen::lastOperand() const
{
    if (!isSymbolic() || operandCount() < 1)
        return undefined();
    return QGen(_SYMBptr->feuille._VECTptr->back(), context);
}

QGen QGen::secondOperand() const
{
    if (!isSymbolic() || operandCount() < 2)
        return undefined();
    return QGen(_SYMBptr->feuille._VECTptr->at(1), context);
}

QGen QGen::thirdOperand() const
{
    if (!isSymbolic() || operandCount() < 3)
        return undefined();
    return QGen(_SYMBptr->feuille._VECTptr->at(2), context);
}

QGen QGen::nthOperand(int n) const
{
    if (!isSymbolic() || operandCount() <= qAbs(n))
        return undefined();
    giac::vecteur &vect = *_SYMBptr->feuille._VECTptr;
    return QGen(vect.at(n >= 0 ? n : int(vect.size()) + n), context);
}

bool QGen::isUnitApplicationOperator(bool &hasConstant) const
{
    if (!isUnitApplicationOperator())
        return false;
    Vector identifiers = QGen(giac::_lname(secondOperand(), context)).toVector();
    Vector::const_iterator it;
    for (it = identifiers.begin(); it != identifiers.end(); ++it)
    {
        if (it->toString().endsWith("_"))
        {
            hasConstant = true;
            break;
        }
    }
    return true;
}

bool QGen::isFunctionApplicationOperator(QString &functionName, Vector &arguments) const
{
    if (!isFunctionApplicationOperator())
        return false;
    Vector args = unaryFunctionArgument().toVector();
    functionName = args.front().toString();
    args.pop_front();
    arguments = args;
    return true;
}

bool QGen::isMappingOperator(Vector &variables, QGen &expression) const
{
    if (!isMappingOperator())
        return false;
    variables = firstOperand().toVector();
    expression = secondOperand();
    return true;
}

bool QGen::isIntervalOperator(QGen &lowerBound, QGen &upperBound) const
{
    if (!isIntervalOperator())
        return false;
    lowerBound = firstOperand();
    upperBound = secondOperand();
    return true;
}

bool QGen::isRationalExpression() const
{
    if (isReciprocalOperator())
        return true;
    if (isProductOperator())
    {
        giac::const_iterateur it;
        giac::vecteur &args(*_SYMBptr->feuille._VECTptr);
        for (it = args.begin(); it != args.end(); ++it)
        {
            if (it->is_symb_of_sommet(giac::at_inv))
                return true;
        }
    }
    return false;
}

bool QGen::isRationalExpression(QGen &numerator, QGen &denominator) const
{
    if (isReciprocalOperator())
    {
        numerator = QGen(1, context);
        denominator = unaryFunctionArgument();
        return true;
    }
    if (!isProductOperator())
        return false;
    QGen::Vector factors = unaryFunctionArgument().toVector();
    QGen::Vector::const_iterator it;
    numerator = QGen(1, context);
    denominator = QGen(1, context);
    for (it = factors.begin(); it != factors.end(); ++it)
    {
        if (it->isReciprocalOperator())
            denominator = denominator * it->unaryFunctionArgument();
        else
            numerator = *it * numerator;
    }
    return !denominator.isOne();
}

bool QGen::isDoubleLetterIdentifier() const
{
    QString text = toString();
    return isIdentifier() && text.length() == 2 && text.begin()->isLetter() && *text.begin() == *text.end();
}

bool QGen::isInequation() const
{
    return  isLessThanOperator() || isGreaterThanOperator() ||
            isLessThanOrEqualOperator() || isGreaterThanOrEqualOperator();
}

int QGen::isInequation(QGen &leftHandSide, QGen &rightHandSide) const
{
    if (!isInequation())
        return 0;
    leftHandSide = firstOperand();
    rightHandSide = secondOperand();
    if (isLessThanOperator())
        return InequalityType::LessThan;
    if (isLessThanOrEqualOperator())
        return InequalityType::LessThanOrEqualTo;
    if (isGreaterThanOperator())
        return InequalityType::GreaterThan;
    if (isGreaterThanOrEqualOperator())
        return InequalityType::GreaterThanOrEqualTo;
    return 0; // unreachable
}

int QGen::operatorType(int &priority) const
{
    if (isSumOperator() || isHadamardSumOperator()) {
        priority = AdditionPriority; return OperatorType::Associative; }
    if (isProductOperator() || isAmpersandProductOperator() || isHadamardProductOperator() || isComposeOperator()) {
        priority = MultiplicationPriority; return OperatorType::Associative; }
    if (isConjunctionOperator() || isDisjunctionOperator() || isExclusiveOrOperator()) {
        priority = LogicalPriority; return OperatorType::Associative; }
    if (isBitwiseAndOperator() || isBitwiseOrOperator() || isBitwiseXorOperator()) {
        priority = BitwisePriority; return OperatorType::Associative; }
    if (isUnionOperator() || isIntersectionOperator()) {
        priority = SetPriority; return OperatorType::Associative; }
    if (isMinusOperator() || isIncrementOperator() || isDecrementOperator() || isNegationOperator() ||
            isRealPartOperator() || isImaginaryPartOperator() || isTraceOperator()) {
        priority = UnaryPriority; return OperatorType::Unary; }
    if (isComplexConjugateOperator() || isFactorialOperator() ||
            isTranspositionOperator() || isDerivativeOperator(true)) {
        priority = ExponentiationPriority; return OperatorType::Unary; }
    if (isReciprocalOperator()) {
        priority = DivisionPriority; return OperatorType::Unary; }
    if (isUnitApplicationOperator()) {
        priority = MultiplicationPriority; return OperatorType::Binary; }
    if (isPowerOperator() || isHadamardPowerOperator() || isFunctionalPowerOperator()) {
        priority = ExponentiationPriority; return OperatorType::Binary; }
    if (isHadamardDifferenceOperator()) {
        priority = DifferencePriority; return OperatorType::Binary; }
    if (isHadamardDivisionOperator()) {
        priority = MultiplicationPriority; return OperatorType::Binary; }
    if (isCrossProductOperator()) {
        priority = MultiplicationPriority; return OperatorType::Binary; }
    if (isSetDifferenceOperator()) {
        priority = SetPriority; return OperatorType::Binary; }
    if (isIntervalOperator()) {
        priority = IntervalPriority; return OperatorType::Binary; }
    if (isInequation()) {
        priority = InequationPriority; return OperatorType::Binary; }
    if (isElementOperator() || isEqualOperator() || isNotEqualOperator()) {
        priority = ComparisonPriority; return OperatorType::Binary; }
    if (isEquation()) {
        priority = EquationPriority; return OperatorType::Binary; }
    if (isFunctionApplicationOperator() || isAtOperator()) {
        priority = ApplicationPriority; return OperatorType::Binary; }
    if (isMappingOperator()) {
        priority = EquationPriority; return OperatorType::Binary; }
    if (isAssignmentOperator() || isArrayAssignmentOperator()) {
        priority = AssignmentPriority; return OperatorType::Binary; }
    if (isConditionalOperator() || isIfThenElseOperator()) {
        priority = ConditionalPriority; return OperatorType::Ternary; }
    return 0;
}

bool QGen::isOperator(int &type) const
{
    int priority, t;
    if ((t = operatorType(priority)) == 0)
        return false;
    type = t;
    return true;
}

bool QGen::isOperator() const
{
    int t;
    return isOperator(t);
}

bool QGen::isAssociativeOperator(Vector &arguments) const
{
    int t;
    if (!isOperator(t) || t != OperatorType::Associative)
        return false;
    arguments = flattenOperands();
    return true;
}

bool QGen::isBinaryOperator(QGen &left, QGen &right) const
{
    int t;
    if (!isOperator(t) || t != OperatorType::Binary)
        return false;
    left = firstOperand();
    right = secondOperand();
    return true;
}

bool QGen::isUnaryOperator(QGen &operand) const
{
    int t;
    if (!isOperator(t) || t != OperatorType::Unary)
        return false;
    operand = unaryFunctionArgument();
    return true;
}

int QGen::operatorPriority() const
{
    int priority;
    if (operatorType(priority) == 0)
        return -1;
    return priority;
}

bool QGen::isDerivativeOperator(bool simple) const
{
    return isUnaryFunction(giac::at_derive) && !simple == unaryFunctionArgument().isVector();
}

bool QGen::isUserOperator(QString &name) const
{
    if (!isUserOperator())
        return false;
    name = QString(_SYMBptr->sommet.ptr()->s);
    return true;
}

bool QGen::isIndefiniteIntegral(QGen &expression, QGen &variable) const
{
    if (!isIntegral() || unaryFunctionArgument().length() != 2)
        return false;
    expression = firstOperand();
    variable = secondOperand();
    return true;
}

bool QGen::isDefiniteIntegral(QGen &expression, QGen &variable, QGen &lowerBound, QGen &upperBound) const
{
    if (!isIntegral() || operandCount() < 3)
        return false;
    expression = firstOperand();
    variable = secondOperand();
    if (operandCount() == 3)
    {
        QGen last = lastOperand();
        if (!last.isEquation() || !last.secondOperand().isIntervalOperator())
            return false;
        QGen interval = last.secondOperand();
        lowerBound = interval.firstOperand();
        upperBound = interval.secondOperand();
    }
    else if (operandCount() == 4)
    {
        lowerBound = thirdOperand();
        upperBound = lastOperand();
    }
    else return false;
    return true;
}

QGen::Vector QGen::flattenOperands() const
{
    Q_ASSERT(isSymbolic());
    Vector operands;
    QGen argument = unaryFunctionArgument();
    if (!argument.isVector())
        operands.push_back(argument);
    else
    {
        Vector argumentVector = argument.toVector();
        Vector::iterator it;
        for (int i = 0 ; i < argumentVector.length(); ++i) {
            QGen operand = argumentVector.at(i);
            if (operand.isUnaryFunction(unaryFunctionPointer())) {
                Vector subOperands = operand.flattenOperands();
                for (it = subOperands.begin(); it != subOperands.end(); ++it)
                    operands.push_back(*it);
            }
            else operands.push_back(operand);
        }
    }
    return operands;
}

void QGen::resizeVector(int n)
{
    if (isVector())
        _VECTptr->resize(n >= 0 ? n : qMax(int(_VECTptr->size()) + n, 0));
}

QGen::Vector QGen::toVector() const
{
    Vector vector;
    if (isVector())
    {
        giac::const_iterateur it;
        for (it = _VECTptr->begin(); it != _VECTptr->end(); ++it)
            vector.append(QGen(*it, context));
    }
    return vector;
}

qreal QGen::approximateValue() const
{
    QGen e(giac::_evalf(*this, context), context);
    return e.isDouble() ? e.doubleValue() : 0;
}

bool QGen::isComplex(QGen &realPart, QGen &imaginaryPart) const
{
    if (!isComplex())
        return false;
    realPart = *_CPLXptr;
    imaginaryPart = *(_CPLXptr + 1);
    return true;
}

bool QGen::isFraction(QGen &numerator, QGen &denominator) const
{
    if (!isFraction())
        return false;
    numerator = _FRACptr->num;
    denominator = _FRACptr->den;
    return true;
}

bool QGen::isModular(QGen &value, QGen &modulus) const
{
    if (!isModular())
        return false;
    value = *_MODptr;
    modulus = *(_MODptr + 1);
    return true;
}

QPicture QGen::toPicture(const QString &family, int size) const
{
}

QString QGen::toLaTeX() const
{
    QGen latex = giac::_latex(*this, context);
    if (latex.isString())
        return latex.stringValue();
    return "";
}

QString QGen::toMathML() const
{
    QGen mathML = giac::_mathml(*this, context);
    if (mathML.isString())
        return mathML.stringValue();
    return "";
}

giac::gen QGen::seq(const giac::gen &g1, const giac::gen &g2)
{
    return giac::makesequence(g1, g2);
}

giac::gen QGen::seq(const giac::gen &g1, const giac::gen &g2, const giac::gen &g3)
{
    return giac::makesequence(g1, g2, g3);
}

giac::gen QGen::seq(const giac::gen &g1, const giac::gen &g2, const giac::gen &g3, const giac::gen &g4)
{
    return giac::makesequence(g1, g2, g3, g4);
}

giac::gen QGen::vec(const giac::gen &g)
{
    return giac::makevecteur(g);
}

giac::gen QGen::vec(const giac::gen &g1, const giac::gen &g2)
{
    return giac::makevecteur(g1, g2);
}

giac::gen QGen::vec(const giac::gen &g1, const giac::gen &g2, const giac::gen &g3)
{
    return giac::makevecteur(g1, g2, g3);
}

giac::gen QGen::vec(const giac::gen &g1, const giac::gen &g2, const giac::gen &g3, const giac::gen &g4)
{
    return giac::makevecteur(g1, g2, g3, g4);
}

QGen QGen::operator ()(const Vector &arguments) const
{
    if (!isFunction())
        return undefined();
    giac::vecteur args;
    Vector::const_iterator it;
    for (it = arguments.begin(); it != arguments.end(); ++it)
        args.push_back(*it);
    return QGen(giac::_apply(seq(*this, vec(args)), context)._VECTptr->front(), context);
}

QGen QGen::operator ()(const QGen &g) const
{
    if (isVector() && g.isInteger() && g.integerValue() < length())
        return QGen(_VECTptr->at(g.integerValue()), context);
    Vector arguments;
    arguments.append(g);
    return this->operator ()(arguments);
}

QGen QGen::operator ()(const QGen &g1, const QGen &g2) const
{
    if (isMatrix() && g1.isInteger() && g2.isInteger() &&
            g1.integerValue() < rows() && g2.integerValue() < columns())
        return QGen(giac::_at(seq(*this, vec(g1, g2)), context), context);
    Vector arguments;
    arguments.append(g1);
    arguments.append(g2);
    return this->operator ()(arguments);
}

QGen QGen::operator ()(const QGen &g1, const QGen &g2, const QGen &g3) const
{
    Vector arguments;
    arguments.append(g1);
    arguments.append(g2);
    arguments.append(g3);
    return this->operator ()(arguments);
}

QGen QGen::operator ()(const QGen &g1, const QGen &g2, const QGen &g3, const QGen &g4) const
{
    Vector arguments;
    arguments.append(g1);
    arguments.append(g2);
    arguments.append(g3);
    arguments.append(g4);
    return this->operator ()(arguments);
}

QGen QGen::identifier(const QString &name, const giac::context *ctx)
{
    return QGen(giac::identificateur(name.toStdString()), ctx);
}

QGen QGen::string(const QString &text, const giac::context *ctx)
{
    return QGen(giac::string2gen(text.toStdString()), ctx);
}

QGen QGen::fraction(int numerator, int denominator, const giac::context *ctx)
{
    return QGen(giac::fraction(numerator, denominator), ctx);
}

QGen QGen::binomial(const QGen &n, const QGen &k, const giac::context *ctx)
{
    return QGen(giac::symbolic(giac::at_binomial, giac::makesequence(n, k)), ctx);
}

QGen QGen::plusInfinity(const giac::context *ctx)
{
    return QGen(giac::symbolic(giac::at_plus, giac::_IDNT_infinity()), ctx);
}

QGen QGen::minusInfinity(const giac::context *ctx)
{
    return QGen(giac::symbolic(giac::at_neg, giac::_IDNT_infinity()), ctx);
}

bool QGen::defineBinaryOperator(const QString &name, OperatorPriority priority, const QString &symbol,
                                const QGen &realFunction, bool checkProperties, const giac::context *ctx)
{
    giac::gen binary = giac::gen(giac::_BINARY_OPERATOR).change_subtype(giac::_INT_MUPADOPERATOR);
    giac::gen args = seq(giac::string2gen(name.toStdString()), realFunction, binary);
    if (giac::is_zero(giac::user_operator(args, ctx)))
        return false;
    UserOperator op;
    op.context = ctx;
    op.priority = priority;
    op.symbol = symbol;
    op.isRelation = false;
    if (checkProperties)
    {
        QGen x = QGen::identifier(" x_tmp"), y = QGen::identifier(" y_tmp"), z = QGen::identifier(" z_tmp");
        QGen d = realFunction(z,realFunction(x,y)) - realFunction(realFunction(x,y),z);
        op.isAssociative = d.simplify().isZero();
        d = realFunction(x,y) - realFunction(y,x);
        op.isCommutative = d.simplify().isZero();
    }
    else
    {
        op.isAssociative = false;
        op.isCommutative = false;
    }
    return true;
}

bool QGen::defineBinaryRelation(const QString &name, OperatorPriority priority, const QString &symbol,
                                const QGen &booleanFunction, const giac::context *ctx)
{
    giac::gen binary = giac::gen(giac::_BINARY_OPERATOR).change_subtype(giac::_INT_MUPADOPERATOR);
    giac::gen args = seq(giac::string2gen(name.toStdString()), booleanFunction, binary);
    if (giac::is_zero(giac::user_operator(args, ctx)))
        return false;
    UserOperator op;
    op.context = ctx;
    op.priority = priority;
    op.symbol = symbol;
    op.isRelation = true;
    op.isAssociative = false;
    op.isCommutative = false;
    return true;
}

bool QGen::findUserOperator(const QString &name, UserOperator &properties)
{
    QMap<QString, UserOperator>::const_iterator it = userOperators.constFind(name);
    if (it == userOperators.constEnd())
        return false;
    properties = *it;
    return true;
}
