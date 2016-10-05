### Begin-lib eval
#
# The current version of this library can be found by searching
# http://www.armory.com/~ftp/
#
# Optional (see below) libraries: strtol and PlaceValues, available at the same
# place you found this.
#
# @(#) eval 3.0 2011-08-29
# 2005-04-28 John H. DuBois III (john@armory.com)
# 2006-06-05 1.1 Added logical operators and constants.
# 2006-06-29 1.2 Allow hex and ksh-style integers.
# 2007-05-31 1.3 Allow return of error messages on error, instead of exit.
# 2007-06-22 1.3.1 Fixed associativity of +-
# 2007-08-13 1.4 Added bitwise & and |
#            Fixed precedence of various operators
# 2009-08-29 1.5 Added params[], and the angle_units param.
# 2010-11-09 2.0 Merged constants[]/params[].
#            Replaced all further parameters with @values in params[].
#            Added alternate place-value conversion.
#            Made kshbase operation conditional on @kshbase.
# 2011-04-15 2.0.1 Corrected reference to ["angle_unit"]
# 2011-08-29 3.0 Allow string operations.
#            Added gen_keyword_expr().

# eval: A mathematical & string expression evaluator.  Inspired by calc3.
# All of the operators below that are part of C are evaluated according to
# their C precedence.
# This library handles:
# () [parenthesization]
# <function>() where <function>() is one of:
#     sin(), cos(), int(), log(), log10(), logn(), asin(), acos(), sqrt(),
#     srand(), tan(), atan2(), rand()
# ^ (exponentiation, not xor)
# * / %
# binary+-
# unary+-
# ! (Boolean not, not factorial)
# < > >= <= != ==
# ~ !~ (if @strings is given, see below)
# && || & |
# numbers, including hex (0xnnn) integers, decimal numeric constants with
# exponent in e notation, pi, e, and passed-in named constants.
# If the strtol library is included, ksh-style (base#value) integers can be
# handled.
# If the PlaceValues library is included, alternate-place-value numbers of the
# type described in the documentation for that library can be handled.
# If @strings is true, single- or double-quoted strings.

# Input variables:
#
# expr is the expression string to evaluate.
#
# params[] may contain numeric values indexed by names of the form
# [_[:alpha:]][_[:alnum:]]*
# If a name occurs in expr, its numeric value is substituted for it.
# The magic constant name "*" can be used to give a value to all constants for
# which an explicit value is not given.
#
# Besides constants, params[] may contain various parameters distinguished from
# constants by beginning with an "@".  These are:
#
# @angle_unit: If set to "d", the angular units used by the trigonometric
# functions for both input and output are in degrees.  If not set or set to
# a value other than "d", the angular unit used is radians.
#
# @noeval: If true, the expression is parsed but not actually evaluated.
# In this case the return value has no meaning unless it is an error message as
# described below.  This can be used to test for general sanity.  Also, an
# index will be added in params[] for every constant-name-like string in expr
# that is referenced.  To get a list of constants referenced in an expression,
# use @noeval and iterate over the indexes returned in params[], skipping any
# indexes that begin with @.
#
# @error_fatal: Determines the action taken if a bad expression is encountered.
# If true, eval will print an error message to stderr and exit 1.
# If false, eval() will return, with the return value being a string describing
# the error, prefixed with "!".  This is the default.
#
# @debug: If true, debugging information is printed to standard error.
#
# @separator, @placefactor, @wholevalue: These set the parameter to
# placeValuesToNum() of the same name.  Alternate place value conversion is
# enabled if @separator is given.  This requires the PlaceValues library.
#
# @kshbase: If true, ksh-style (base#value) integers are understood.  This
# requires the strtol library.
#
# @strings: If true, allow string operations.
# Values in params[] are not forced to be numeric.
# Strings may be included directly in expressions by surrounding them with ''
# or "".
# The additional operator ~ is allowed, for regular expression matching.
# Most of the operators have no meaning for strings.  Those that do:
# comparison, parenthesization, ||, &&.
#
# If @strings is true, these additional variables have meaning:
#
# @no_single_quotes: Do not recognized single-quoted strings in expressions
#
# @no_double_quotes: Do not recognized single-quoted strings in expressions
#
# Parameters that may be returned in params[]:
# 
# @max_components_seen: Set to the maximum number of components seen in an
# alternate place-value number.
#
# If @noeval is true, referenced constant names as described above.
#
# Various internal parameters are stored under indexes beginning with @.
# Also, string constants are stored under indexes beginning with $.

# Implementation notes:
#
# Whitespace is not preemptively removed in order to prevent errors in the
# initial expression from being missed.
#
function eval(expr, params,

	t, prefix, parenthesized, suffix, ret, sepPat)
{
    if (params["@strings"]) {
	expr = _eval_extractStrings(expr, params)
	if (expr == "!" && params["@error_message"] != "")
	    return "!" params["@error_message"]
    }
    # Evaluate function invocations and parenthesized expressions
    # Get a pair of parentheses that has no other parens inside it
    if (!(1 in _eval_boolean_ops))
	split("\\|\\| && \\| &", _eval_boolean_ops)
    params["@angle_factor"] = params["@angle_unit"] == "d" ? 45/atan2(1,1) : 1
    if ("@separator" in params) {
	sepPat = gensub(/./, "\\\\&", "g", params["@separator"])
	params["@alt_pat"] = "^[0-9." sepPat "]*[" sepPat "][0-9" sepPat ".]*"
    }
    while (_eval_match(expr, "\\([^()]*\\)", t)) {
	prefix = t[1]
	parenthesized = substr(t[2], 2, length(t[2])-2)	# Discard parentheses
	suffix = t[3]
	# If parenthesized section was preceded by a name,
	# it is a function invocation
	if (_eval_match(prefix, "[_[:alpha:]][_[:alnum:]]*", t, 0, 1)) {
	    prefix = t[1]
	    ret = _eval_func_call(t[2], parenthesized, params)
	}
	else
	    ret = _eval_boolean(parenthesized, params, 1)
	if (ret == "!")
	    break
	ret = prefix " " sprintf("%.17g", ret) " " suffix
	_eval_dbprint(params, "eval", expr, ret)
	expr = ret
    }
    if (ret != "!")
	ret = _eval_boolean(expr, params, 1)
    if (ret == "!")
	ret = "!" params["@error_message"]
    return ret
}

# Evaluate a function call.
# Input variables:
# func_name is the name of the function.
# parmlist is the parameter list.
# Return value: The evaluated result of the function call.
function _eval_func_call(func_name, parmlist, params,

	nparm, n, parms, f, minp, maxp, ret, elem, i)
{
    # Generate table of number of allowed parameters for each function.
    if (!("srand" in _min_parms)) {
	_min_parms["srand"] = 0
	_max_parms["srand"] = 1
	_min_parms["atan2"] = _min_parms["logn"] = 2
	_min_parms["rand"] = 0
	_max_parms["rand"] = 1
	split("cos int log log10 sin asin acos sqrt tan atan", elem, " ")
	for (i in elem)
	    _min_parms[elem[i]] = 1
	for (f in _min_parms)
	    if (!(f in _max_parms))
		_max_parms[f] = _min_parms[f]
    }
    if (!(func_name in _min_parms))
	return _eval_error(params, "Unknown function \"" func_name "\"")
    nparm = split(parmlist, parms, ",")
    minp = _min_parms[func_name]
    maxp = _max_parms[func_name]
    if (!(minp <= nparm && nparm <= maxp))
	return _eval_error(params, "Wrong number of arguments to function " func_name \
		"; expected " minp ((minp != maxp) ? "-" maxp : "") "; got " nparm)
    for (n = 1; n <= nparm; n++)
	if ((parms[n] = _eval_boolean(parms[n], params, 1)) == "!")
	    return "!"

    if (params["@noeval"])
	ret = 0
    else if (func_name == "cos")
	ret = cos(parms[1] / params["@angle_factor"])
    else if (func_name == "int")
	ret = int(parms[1])
    else if (func_name == "log")
	ret = log(parms[1])
    else if (func_name == "log10")
	ret = log(parms[1])/log(10)
    else if (func_name == "logn")
	ret = log(parms[2])/log(parms[1])
    else if (func_name == "sin")
	ret = sin(parms[1] / params["@angle_factor"])
    else if (func_name == "asin")
	ret = atan2(parms[1], sqrt(1-parms[1]*parms[1])) * params["@angle_factor"]
    else if (func_name == "acos")
	ret = atan2(sqrt(1-parms[1]*parms[1]), parms[1]) * params["@angle_factor"]
    else if (func_name == "sqrt")
	ret = sqrt(parms[1])
    else if (func_name == "srand") {
	if (nparm == 1)
	    ret = srand(parms[1])
	else
	    ret = srand()
    }
    else if (func_name == "tan")
	ret = sin(parms[1]) / cos(parms[1] / params["@angle_factor"])
    else if (func_name == "atan2")
	ret = atan2(parms[1], parms[2]) * params["@angle_factor"]
    else if (func_name == "atan")
	ret = atan2(parms[1], 1) * params["@angle_factor"]
    else if (func_name == "rand") {
	ret = rand()
	if (nparm == 1)
	    ret *= parms[1]
    }
    else	# we should never get here
	return _eval_error(params, "Unknown function \"" func_name "\"")
    if (params["@debug"]) {
	printf "%s(%s", func_name, parms[1] > "/dev/stderr"
	for (n = 2; n <= nparm; n++)
	    printf ",%s", parms[n] > "/dev/stderr"
	printf ") -> %s\n", ret > "/dev/stderr"
    }
    return ret
}

# Evaluate || && | &.  The order in which instances of a particular operator
# are evaluated does not matter since we do not do lazy evaluation
# and have no side effects.
function _eval_boolean(expr, params, opnum,

	t, t1, t3, ret, op, op_pat)
{
    op = op_pat = _eval_boolean_ops[opnum]
    if (op_pat == "")
	return _eval_equality(expr, params)
    gsub("\\\\", "", op)
    _eval_dbgot(params, "_eval_" op, expr)
    if (_eval_match(expr, op_pat, t)) {
	t1 = _eval_boolean(t[1], params, opnum+1)
	if (t1 == "!" || (t3 = _eval_boolean(t[3], params, opnum)) == "!")
	    return "!"
	if (op == "||")
	    ret = t1 || t3
	else if (op == "&&")
	    ret = t1 && t3
	else if (op == "|")
	    ret = or(t1, t3)
	else
	    ret = and(t1, t3)
	_eval_dbprint(params, "_eval_" op, expr, ret)
    }
    else
	ret = _eval_boolean(expr, params, opnum+1)
    return ret
}

# Evaluate != == !~ ~, left to right
function _eval_equality(expr, params,

	t, t1, t3, ret, op)
{
    _eval_dbgot(params, "_eval_equality", expr)
    # We require that the RHS of the expr not match our ops.
    if (_eval_match(expr, "[!=]=|!?~", t, 0, 0, "", 2)) {
	t1 = _eval_equality(t[1], params)
	op = t[2]
	if (t1 == "!" || (t3 = _eval_relational(t[3], params)) == "!")
	    return "!"

	if (op == "!=")
	    ret = t1 != t3
	else if (op == "==")
	    ret = t1 == t3
	else if (op == "~" && params["@strings"])
	    ret = t1 ~ t3
	else if (op == "!~" && params["@strings"])
	    ret = t1 !~ t3
	else
	    return _eval_error(params, "Unknown operator: " op)
	_eval_dbprint(params, "_eval_equality", expr, ret)
    }
    else
	ret = _eval_relational(expr, params)
    return ret
}

# Evaluate < > >= <=, left to right
function _eval_relational(expr, params,

	t, t1, t3, ret, op)
{
    _eval_dbgot(params, "_eval_relational", expr)
    # We require that the RHS of the expr not match our ops.
    if (_eval_match(expr, "[<>=]+", t, 0, 0, "[^<>]*$")) {
	t1 = _eval_relational(t[1], params)
	op = t[2]
	if (t1 == "!" || (t3 = _eval_plusminus(t[3], params)) == "!")
	    return "!"

	if (op == "<")
	    ret = t1 < t3
	else if (op == ">")
	    ret = t1 > t3
	else if (op == ">=")
	    ret = t1 >= t3
	else if (op == "<=")
	    ret = t1 <= t3
	else
	    return _eval_error(params, "Unknown operator: " op)
	_eval_dbprint(params, "_eval_relational", expr, ret)
    }
    else
	ret = _eval_plusminus(expr, params)
    return ret
}

# Evaluate binary +-, left to right
# If negated is true, the leftmost term is negated.  Subtraction is implemented
# this way because the limitations of awk (lack of a rightmost match operator)
# mean that we have to split terms off from left to right.  In the case of e.g.
# multiplicative operators, we can force a rightmost match by insisting that
# the righthand side not contain any of the multiplicative operator characters,
# but that will not work for summing operators because they may be used as part
# of constants (e.g. 10e+5).  Naive parsing would therefore cause t1-t2, where
# t2 is t2a-t2b, to be interpreted as t1-(t2a-t2b).  Instead we tell this
# function, when evaluating t2, to negate the first term in t2, so the overall
# evaluation becomes t1+(-t2a-t2b).
function _eval_plusminus(expr, params, negated,

	t, t1, t3, ret, len)
{
    # +- is a binary operator only if it follows a number/constant
    # or close-paren.  This pattern allows:
    # - A one-letter constant name at the beginning of the expression
    # - Anything ending in a digit or decimal point
    # - A multi-letter constant name
    # The purpose of this is to avoid treating e.g. 10e+5 as an addition,
    # while still allowing e.g. a10e+5, where a10e is a constant name.
    # We do this by insisting that if the character before the operator is a
    # letter, then it must be preceded by at least one other letter or
    # underscore (since constants are required to start with one of those),
    # and must be connected to that by the characters allowed in constants.
    if (_eval_match(expr,
	    "(^[_[:alpha:]]|[0-9.)]|[_[:alpha:]][_[:alnum:]]*[_[:alpha:]])[[:blank:]]*[-+]", t)) {
	# The expression is now split into:
	# t[1] t[2]: Left hand side of the expression, with trailing + or -
	# t[3]: Right hand side of the expression
	sub(/[[:blank:]]+/,"",t[2])
	len = length(t[2])
	t1 = _eval_mult(t[1] substr(t[2],1,len-1), params)
	if (t1 == "!" || (t3 = _eval_plusminus(t[3], params, substr(t[2],len) == "-")) == "!")
	    return "!"
	ret = (negated ? -t1 : t1) + t3
	_eval_dbprint(params, "_eval_plusminus", expr, ret)
    }
    else {
	ret = _eval_mult(expr, params)
	if (negated)
	    ret = -ret
    }
    return ret
}

# Evaluate */%, left to right
function _eval_mult(expr, params,

	t, t1, t3, ret, op)
{
    # Search expr for the rightmost instance of any of our operators,
    # so that the remaining (left) part can be passed to a recursive
    # call to this function, causing it to be evaluated first and so
    # satisfy the left-to-right requirement.
    if (_eval_match(expr, "[*/%]", t, 0, 0, "[^*/%]*$")) {
	t1 = _eval_mult(t[1], params)
	op = t[2]
	if (t1 == "!" || (t3 = _eval_exponentiation(t[3], params)) == "!")
	    return "!"
	if (params["@noeval"])
	    ret = 0
	else if (op == "*")
	    ret = t1 * t3
	else {
	    if (t3 == 0)
		return _eval_error(params, "Division by zero")
	    if (op == "/")
		ret = t1 / t3
	    else
		ret = t1 % t3
	}
	_eval_dbprint(params, "_eval_mult", expr, ret)
    }
    else
	ret = _eval_exponentiation(expr, params)
    return ret
}

# Evaluate ^, right to left
function _eval_exponentiation(expr, params,

	t, t1, t3, ret)
{
    if (_eval_match(expr, "\\^", t)) {
	t1 = _eval_bang(t[1], params)
	if (t1 == "!" || (t3 = _eval_exponentiation(t[3], params)) == "!")
	    return "!"
	ret = params["@noeval"] ? 0 : t1 ^ t3
	_eval_dbprint(params, "_eval_exponentiation", expr, ret)
    }
    else
	ret = _eval_bang(expr, params)
    return ret
}

# Evaluate !
function _eval_bang(expr, params,

	t, ret)
{
    if (_eval_match(expr, "!", t, 1)) {
	if ((ret = _eval_bang(t[3], params)) == "!")
	    return "!"
	ret = !ret
	_eval_dbprint(params, "_eval_bang", expr, ret)
    }
    else
	ret = _eval_num_const(expr, params)
    return ret
}

# Evaluate numbers & constants
function _eval_num_const(expr, params,

	ret, pvtnInfo)
{
    if (expr ~ "^[[:blank:]]*[-+]?[[:blank:]]*([0-9]+\\.?[0-9]*|\\.[0-9]+)([eE][+-]?[0-9]+)?[[:blank:]]*$") {
	gsub(/[[:blank:]]/, "", expr)
	ret = expr + 0
    }
    else {
	gsub(/^[[:blank:]]+|[[:blank:]]+$/, "", expr)

        if (params["@kshbase"] && (ret = kshbase(expr)) != "")
            ;
	else if ("@alt_pat" in params && match(expr, params["@alt_pat"])) {
	    ret = placeValuesToNum(expr, params["@separator"], params["@placefactor"],
		    params["@wholevalue"], 0, pvtnInfo)
	    if (substr(ret,1,1) == "!")
		return _eval_error(params, "Bad quantity \"" expr "\": " substr(ret,2))
	    if (pvtnInfo["num"] > params["@max_components_seen"])
		params["@max_components_seen"] = pvtnInfo["num"]
	}
	else if (expr in params) {
	    ret = params[expr]
	    if (!params["@strings"])
		ret += 0
	}
	else if ("*" in params) {
	    ret = params["*"]
	    if (!params["@strings"])
		ret += 0
	}
	else {
	    if (expr == "pi" || expr == "PI")
		ret = atan2(1,1)*4
	    else if (expr == "e" || expr == "E")
		ret = exp(1)
	    else if (expr ~ /^[_[:alpha:]][_[:alnum:]]*$/)
		if (params["@noeval"]) {
		    params[expr]
		    return 0
		}
		else
		    return _eval_error(params, "Undefined constant: " expr)
	    else
		return _eval_error(params, "Expected a number; got: " expr)
	}
    }
    _eval_dbprint(params, "_eval_num_const", expr, ret)
    return ret
}

function _eval_dbgot(params, funcname, expr)
{
    if (params["@debug"])
	printf "%s: Got expression: %s\n", funcname, expr > "/dev/stderr"
}

function _eval_dbprint(params, funcname, old, new)
{
    if (params["@debug"] && old != new)
	printf "%s: %s -> %s\n", funcname, old, new > "/dev/stderr"
}

function _eval_error(params, error)
{
    if (params["@error_fatal"]) {
	print "Evaluation error: " error > "/dev/stderr"
	exit 1
    }
    params["@error_message"] = error
    return "!"
}

# Match a string to a pattern and return the parts that precede the pattern,
# that match the pattern, and that follow the pattern.
# Input variables:
# s: String to find a match in.
# regex: Pattern to find in s.
# head: If head is true, regex is required to match the start of s.
# tail: If tail is true, regex is required to match the end of s.
# postpat: If given, the part of s after regex must match this pattern.
#     This can be used to limit the part of the string that regex is allowed to
#     match.
# r_len: If nonzero, this indicates that the rightmost instance of regex should
#     be matched.  r_len is the number of characters that regex will match (so
#     this capability will only work for patterns that will match a specific
#     number of characters).
# In both cases, whitespace is allowed.
# Output variables:
# t[1]: The part of s that preceded the matched part.
# t[2]: The matched part of s, with leading & trailing whitespace removed.
# t[3]: The part of s that followed the matched part.
# Return value:
# If regex is matched, 1.
# If not, 0.
function _eval_match(s, regex, t, head, tail, postpat, r_len,

	full_regex, t2, start, len)
{
    full_regex = "(" regex ")(" postpat ")"
    if (head)
	full_regex = "^[[:blank:]]*" full_regex
    if (tail)
	full_regex = full_regex "[[:blank:]]*$"
    if (r_len) {
	start = _eval_rmatch(s, full_regex, r_len)
	len = r_len
    }
    else {
	start = match(s, full_regex)
	len = RLENGTH
    }
    if (start == 0)
	return 0
    t[1] = substr(s,1,start-1)
    t2 = substr(s,start,len)
    t[3] = substr(s,start+len)
    if (postpat != "") {
	if (!match(t2, regex)) {	# should not happen
	    printf "eval: regex failure!" > "/dev/stderr"
	    exit 1
	}
	t[3] = substr(t2,RSTART+RLENGTH) t[3]
	t2 = substr(t2,RSTART,RLENGTH)
    }
    gsub(/^[[:blank:]]+|[[:blank:]]+$/,"",t2)
    t[2] = t2
    return 1
}

# Find rightmost match.
# This only works for regular expressions that will match a specific number of
# characters.
# Input variables:
# s is the string to compare against.
# pat is the pattern to find the rightmost instance of.
# len is the number of characters that pat will match.
# Return value: Index of the start of the rightmost match,
# or 0 if no match.
function _eval_rmatch(s, pat, len,

	num, elem, i, pos)
{
    num = split(s, elem, pat)
    if (num <= 1)
	return 0
    pos = (num-2) * len + 1
    for (i = 1; i < num; i++)
	pos += length(elem[i])
    return pos
}

function _eval_extractStrings(expr, params,

	newExpr, len, quotedVal, stringNum, i, inSQ, inDQ, c)
{
    len = length(expr)

    for (i = 1; i <= len; i++) {
	c = substr(expr, i, 1)
	if (inSQ && c == "'" || inDQ && c == "\"") {
	    inSQ = 0
	    inDQ = 0
	    param = "$" ++stringNum
	    params[param] = quotedVal
	    newExpr = newExpr param
	    quotedVal = ""
	    if (params["@debug"])
		printf "Extracted string constant \"%s\", stored as param %s\n", quotedVal, param > "/dev/stderr"
	}
	else if (c == "'" && !params["no_single_quotes"])
	    inSQ = 1
	else if (c == "\"" && !params["no_double_quotes"])
	    inDQ = 1
	else {
	    if (inSQ || inDQ)
		quotedVal = quotedVal c
	    else
		newExpr = newExpr c
	}
    }
    if (inSQ || inDQ)
	return _eval_error(params, "Unterminated quotes")
    return newExpr
}

# This function generates an expression that will perform a Boolean
# keyword-match test.
#
# Input variables:
# expr is an expression consisting of keywords to test for, along with the
# Boolean operators ! and |, with evaluation order controlled by parentheses.
# Any strings of characters other than those and double-quotes
# are taken to be keywords.  Keywords are separated by whitespace by default,
# but can be combined into longer strings with double-quotes.  Double quotes
# can also be used to quote words containing the special characters to make
# them be (parts of) keywords instead.  The conjunction of two terms gives the
# effect of a Boolean "and" between the terms.  This has a higher evaluation
# order precedence than the explicit "or" (|) operation.
#
# Globals:
# IGNORECASE will determine whether the keyword match is case sensitive or not.
#
# Output variables, return value:
# The return value is an expression suitable for passing to eval(), with params
# as the params array.  The constant "text" should be set to the text to
# search for keywords.
# If an expression does not yield a correct output expression, the return value
# is a (largely useless) error description preceded by a space.
# Example:
# expr = gen_keyword_expr("is (!test|foo)", params)
# (test for error message return)
# params["text"] = "this is a test"
# print eval(expr, params)
function gen_keyword_expr(expr, params,

	newExpr, len, i, c, w, lastType, ret)
{
    len = length(expr)

    for (i = 1; i <= len; i++) {
	c = substr(expr, i, 1)
	w = ""
	switch (c) {
	case /[[:space:]]/:
	    continue
	case "\"":
	    for (i++; i <= len; i++) {
		c = substr(expr, i, 1)
		if (c == "\"")
		    break
		else
		    w = w c
	    }
	    # Don't complain about unterminated quotes
	    type = "word"
	    break
	case /[()!|]/:
	    w = c
	    type = c
	    break
	default:
	    for (; i <= len; i++) {
		c = substr(expr, i, 1)
		if (c ~ /["()!|[:space:]]/) {
		    i--
		    break
		}
		else
		    w = w c
	    }
	    type = "word"
	}
	#printf "type: %s; value: \"%s\"\n", type, w
	if ((lastType == ")" || lastType == "word") && (type != "|" && type != ")"))
	    newExpr = newExpr "&&"
	if (type != "word")
	    newExpr = newExpr (w == "|" ? "||" : w)
	else {
	    gsub(/[^[:alnum:][:space:]]/, "\\\\&", w)
	    newExpr = newExpr "(text ~ \"\\<" w "\\>\")"
	}
	lastType = type
    }
    params["@noeval"] = 1
    params["@strings"] = 1
    params["@no_single_quotes"] = 1
    ret = eval(newExpr, params)
    delete params["@noeval"]
    if (ret ~ /^!/)
	return " " substr(ret, 2)
    else
	return newExpr
}
### End-lib eval
