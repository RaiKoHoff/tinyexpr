/*
 * TINYEXPR - Tiny recursive descent parser and evaluation engine in C
 *
 * Copyright (c) 2015, 2016 Lewis Van Winkle
 *
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgement in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "tinyexpr.h"
#include <stdio.h>
#include "minctest.h"


typedef struct {
    const char *expr;
    double answer;
} test_case;


test_case cases[] = {
    {"1", 1},
    {"(1)", 1},

    {"2+1", 2+1},
    {"(((2+(1))))", 2+1},
    {"3+2", 3+2},

    {"3+2+4", 3+2+4},
    {"(3+2)+4", 3+2+4},
    {"3+(2+4)", 3+2+4},
    {"(3+2+4)", 3+2+4},

    {"3*2*4", 3*2*4},
    {"(3*2)*4", 3*2*4},
    {"3*(2*4)", 3*2*4},
    {"(3*2*4)", 3*2*4},

    {"3-2-4", 3-2-4},
    {"(3-2)-4", (3-2)-4},
    {"3-(2-4)", 3-(2-4)},
    {"(3-2-4)", 3-2-4},

    {"3/2/4", 3.0/2.0/4.0},
    {"(3/2)/4", (3.0/2.0)/4.0},
    {"3/(2/4)", 3.0/(2.0/4.0)},
    {"(3/2/4)", 3.0/2.0/4.0},

    {"(3*2/4)", 3.0*2.0/4.0},
    {"(3/2*4)", 3.0/2.0*4.0},
    {"3*(2/4)", 3.0*(2.0/4.0)},

    {"asin sin .5", 0.5},
    {"sin asin .5", 0.5},
    {"ln exp .5", 0.5},
    {"exp ln .5", 0.5},

    {"asin sin-.5", -0.5},
    {"asin sin-0.5", -0.5},
    {"asin sin -0.5", -0.5},
    {"asin (sin -0.5)", -0.5},
    {"asin (sin (-0.5))", -0.5},
    {"asin sin (-0.5)", -0.5},
    {"(asin sin (-0.5))", -0.5},

    {"log1000", 3},
    {"log1e3", 3},
    {"log 1000", 3},
    {"log 1e3", 3},
    {"log(1000)", 3},
    {"log(1e3)", 3},
    {"log1.0e3", 3},
    {"10^5*5e-5", 5},

    {"100^.5+1", 11},
    {"100 ^.5+1", 11},
    {"100^+.5+1", 11},
    {"100^--.5+1", 11},
    {"100^---+-++---++-+-+-.5+1", 11},

    {"100^-.5+1", 1.1},
    {"100^---.5+1", 1.1},
    {"100^+---.5+1", 1.1},
    {"1e2^+---.5e0+1e0", 1.1},
    {"--(1e2^(+(-(-(-.5e0))))+1e0)", 1.1},

    {"sqrt 100 + 7", 17},
    {"sqrt 100 * 7", 70},
    {"sqrt (100 * 100)", 100},

};


test_case errors[] = {
    {"", 1},
    {"1+", 2},
    {"1)", 2},
    {"(1", 2},
    {"1**1", 3},
    {"1*2(+4", 4},
    {"1*2(1+4", 4},
    {"a+5", 1},
    {"A+5", 1},
    {"Aa+5", 1},
    {"1^^5", 3},
    {"1**5", 3},
    {"sin(cos5", 8},
};


const char *nans[] = {
    "0/0",
    "1%0",
    "1%(1%0)",
    "(1%0)%1",
};


void test_results() {
    int i;
    for (i = 0; i < sizeof(cases) / sizeof(test_case); ++i) {
        const char *expr = cases[i].expr;
        const double answer = cases[i].answer;

        int err;
        const double ev = te_interp(expr, &err);
        lok(!err);
        lfequal(ev, answer);
    }
}


void test_syntax() {
    int i;
    for (i = 0; i < sizeof(errors) / sizeof(test_case); ++i) {
        const char *expr = errors[i].expr;
        const int e = errors[i].answer;

        int err;
        const double r = te_interp(expr, &err);
        lequal(err, e);
        lok(r != r);

        te_expr *n = te_compile(expr, 0, 0, &err);
        lequal(err, e);
        lok(!n);

        const double k = te_interp(expr, 0);
        lok(k != k);
    }
}


void test_nans() {
    int i;
    for (i = 0; i < sizeof(nans) / sizeof(const char *); ++i) {
        const char *expr = nans[i];

        int err;
        const double r = te_interp(expr, &err);
        lequal(err, 0);
        lok(r != r);

        te_expr *n = te_compile(expr, 0, 0, &err);
        lok(n);
        lequal(err, 0);
        const double c = te_eval(n);
        lok(c != c);
        te_free(n);
    }
}


void test_variables() {

    double x, y;
    te_variable lookup[] = {{"x", &x}, {"y", &y}};

    int err;

    te_expr *expr1 = te_compile("cos x + sin y", lookup, 2, &err);
    lok(!err);

    te_expr *expr2 = te_compile("x+x+x-y", lookup, 2, &err);
    lok(!err);

    te_expr *expr3 = te_compile("x*y^3", lookup, 2, &err);
    lok(!err);

    for (y = 2; y < 3; ++y) {
        for (x = 0; x < 5; ++x) {
            double ev = te_eval(expr1);
            lfequal(ev, cos(x) + sin(y));

            ev = te_eval(expr2);
            lfequal(ev, x+x+x-y);

            ev = te_eval(expr3);
            lfequal(ev, x*y*y*y);
        }
    }

    te_free(expr1);
    te_free(expr2);
    te_free(expr3);
}



#define cross_check(a, b) do {\
    expr = te_compile((a), &lookup, 1, &err);\
    lfequal(te_eval(expr), (b));\
    lok(!err);\
    te_free(expr);\
}while(0)

void test_functions() {

    double x;
    te_variable lookup = {"x", &x};

    int err;
    te_expr *expr;

    for (x = -5; x < 5; x += .2) {
        cross_check("abs x", fabs(x));
        cross_check("acos x", acos(x));
        cross_check("asin x", asin(x));
        cross_check("atan x", atan(x));
        cross_check("ceil x", ceil(x));
        cross_check("cos x", cos(x));
        cross_check("cosh x", cosh(x));
        cross_check("exp x", exp(x));
        cross_check("floor x", floor(x));
        cross_check("ln x", log(x));
        cross_check("log x", log10(x));
        cross_check("sin x", sin(x));
        cross_check("sinh x", sinh(x));
        cross_check("sqrt x", sqrt(x));
        cross_check("tan x", tan(x));
        cross_check("tanh x", tanh(x));
    }
}


int main(int argc, char *argv[])
{
    lrun("Results", test_results);
    lrun("Syntax", test_syntax);
    lrun("NaNs", test_nans);
    lrun("Variables", test_variables);
    lrun("Functions", test_functions);
    lresults();

    return lfails != 0;
}
