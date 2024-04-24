#include "tests.hpp"
#include "Expr.hpp"

const Expr a = Symbol("a");
const Expr b = Symbol("b");
const Expr c = Symbol("c");
const Expr d = Symbol("d");
const Expr f = Symbol("f");
const Expr g = Symbol("g");
const Expr h = Symbol("h");
const Expr j = Symbol("j");
Expr actual,expected;

#define TEST(STRING,EXPECT) \
	actual = Expr{STRING}; \
	expected = EXPECT; \
	ASSERT_RELATION(actual,==,expected)

void test_basic_operators(){

	TEST("a,b",List(a,b));
	TEST("a,b,c",List(a,b,c));
	TEST(" a , b , c ",List(a,b,c));

	TEST("a+b",Add(a,b));
	TEST("a+b+c",Add(a,b,c));
	TEST(" a + b + c ",Add(a,b,c));

	TEST("a-b",Sub(a,b));
	TEST("a-b-c",Sub(Sub(a,b),c));
	TEST(" a - b - c ",Sub(Sub(a,b),c));

	TEST("a*b",Mul(a,b));
	TEST("a*b*c",Mul(a,b,c));
	TEST(" a * b * c ",Mul(a,b,c));

	TEST("a/b",Div(a,b));
	TEST("a/b/c",Div(Div(a,b),c));
	TEST(" a / b / c ",Div(Div(a,b),c));

	TEST("-a",Neg(a));
	TEST("--a",Neg(Neg(a)));
	TEST(" - a ",Neg(a));
	TEST(" - - a ",Neg(Neg(a)));

	TEST("a^b",Pow(a,b));
	TEST("a^b^c",Pow(a,Pow(b,c)));
	TEST(" a ^ b ^ c ",Pow(a,Pow(b,c)));

	TEST("abcd",Symbol("abcd"));
	TEST("  abcd  ",Symbol("abcd"));
	TEST("1234",Number(1234));
	TEST("  1234  ",Number(1234));
}
const Test basic_op_test("basic operator string-to-expr",test_basic_operators);

void test_pemdas(){
	TEST("a+b-c*d/f^g",
		Add(
			a,
			Sub(
				b,
				Mul(
					c,
					Div(
						d,
						Pow(
							f,
							g))))));

	TEST("a^b/c*d-f+g",
		Add(
			Sub(
				Mul(
					Div(
						Pow(
							a,
							b
						),
						c
					),
					d
				),
				f
			),
			g
		));

	TEST("-a+-b--c*-d/-f^-g",
		Add(
			Neg(a),
			Sub(
				Neg(b),
				Mul(
					Neg(c),
					Div(
						Neg(d),
						Neg(Pow(
							f,
							Neg(g))))))));

	TEST("(((a)/(b))/((c)/(d)))/(((f)/(g))/((h)/(j)))",
		Div(
			Div(
				Div(a,b),
				Div(c,d)
			),
			Div(
				Div(f,g),
				Div(h,j)
			)
		));

	TEST("(((a)))",a);
}

const Test pemdas_test{"pemdas in string-to-expr",test_pemdas};
