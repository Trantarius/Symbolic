#include "tests.hpp"
#include "Expr.hpp"
#include "actions.hpp"

const Expr a = Symbol("a");
const Expr b = Symbol("b");
const Expr c = Symbol("c");
const Expr d = Symbol("d");
const Expr f = Symbol("f");
const Expr g = Symbol("g");
const Expr h = Symbol("h");
const Expr j = Symbol("j");
Expr actual,expected;

#define TEST(NAME,STRING,EXPECT) \
Test NAME (#NAME, [](){ \
	actual = Expr{STRING}; \
	expected = EXPECT; \
	ASSERT_EQUAL(actual,expected); \
});

TEST(parse_list1,"a,b",List(a,b));
TEST(parse_list2,"a,b,c",List(a,b,c));
TEST(parse_list3," a , b , c ",List(a,b,c));

TEST(parse_add1,"a+b",Add(a,b));
TEST(parse_add2,"a+b+c",Add(a,b,c));
TEST(parse_add3," a + b + c ",Add(a,b,c));

TEST(parse_sub1,"a-b",Sub(a,b));
TEST(parse_sub2,"a-b-c",Sub(Sub(a,b),c));
TEST(parse_sub3," a - b - c ",Sub(Sub(a,b),c));

TEST(parse_mul1,"a*b",Mul(a,b));
TEST(parse_mul2,"a*b*c",Mul(a,b,c));
TEST(parse_mul3," a * b * c ",Mul(a,b,c));

TEST(parse_div1,"a/b",Div(a,b));
TEST(parse_div2,"a/b/c",Div(Div(a,b),c));
TEST(parse_div3," a / b / c ",Div(Div(a,b),c));

TEST(parse_neg1,"-a",Neg(a));
TEST(parse_neg2,"--a",Neg(Neg(a)));
TEST(parse_neg3," - a ",Neg(a));
TEST(parse_neg4," - - a ",Neg(Neg(a)));

TEST(parse_pow1,"a^b",Pow(a,b));
TEST(parse_pow2,"a^b^c",Pow(a,Pow(b,c)));
TEST(parse_pow3," a ^ b ^ c ",Pow(a,Pow(b,c)));

TEST(parse_symbol1,"abcd",Symbol("abcd"));
TEST(parse_symbol2,"  abcd  ",Symbol("abcd"));
TEST(parse_integer1,"1234",Integer(1234));
TEST(parse_integer2,"  1234  ",Integer(1234));
TEST(parse_float1,"12.125",Float(12.125));
TEST(parse_float2,"  12.125  ",Float(12.125));

TEST(pemdas1,"a+b-c*d/f^g",
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

TEST(pemdas2,"a^b/c*d-f+g",
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

TEST(neg_pemdas,"-a+-b--c*-d/-f^-g",
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

TEST(paren_pemdas,"(((a)/(b))/((c)/(d)))/(((f)/(g))/((h)/(j)))",
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

TEST(parens,"(((a)))",a);

#undef TEST
#define TEST(NAME,STRING,EXPECT) \
Test NAME (#NAME, [](){ \
	actual = perform(Expr{STRING}); \
	expected = EXPECT; \
	ASSERT_EQUAL(actual,expected); \
});

TEST(int_perf,"1234",Integer(1234));
TEST(add_perf,"13+47",Integer(60));
TEST(sub_perf,"47-13",Integer(34));
TEST(mul_perf,"13*4",Integer(52));
TEST(div_perf,"52/4",Integer(13));
TEST(neg_perf,"-432",Integer(-432));
TEST(pow_perf,"2^10",Integer(1024));

