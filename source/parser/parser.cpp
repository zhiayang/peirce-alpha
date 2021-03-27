// parser.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "defs.h"
#include "ast.h"

#include <tuple>
#include <optional>

namespace parser
{
	using zst::Ok;
	using zst::Err;

	using TT = TokenType;
	using ResultTy = zst::Result<ast::Expr*, Error>;

	// it's not like i stole this from my own project or anything, b-b-baka
	namespace {
		template <typename> struct is_result : std::false_type { };
		template <typename T, typename E> struct is_result<zst::Result<T, E>> : std::true_type { };

		template <typename T> struct remove_array { using type = T; };
		template <typename T, size_t N> struct remove_array<const T(&)[N]> { using type = const T*; };
		template <typename T, size_t N> struct remove_array<T(&)[N]> { using type = T*; };

		// uwu
		template <typename... Ts>
		using concatenator = decltype(std::tuple_cat(std::declval<Ts>()...));

		[[maybe_unused]] std::tuple<> __unwrap() { return { }; }

		template <typename A, typename = std::enable_if_t<is_result<A>::value>>
		std::tuple<std::optional<typename A::value_type>> __unwrap(const A& a)
		{
			return std::make_tuple(a
				? std::optional<typename A::value_type>(a.unwrap())
				: std::nullopt
			);
		}

		template <typename A, typename = std::enable_if_t<!std::is_array_v<A> && !is_result<A>::value>>
		std::tuple<std::optional<A>> __unwrap(const A& a) { return std::make_tuple(std::optional(a)); }

		template <typename A, typename... As>
		auto __unwrap(const A& a, As&&... as)
		{
			auto x = __unwrap(a);
			auto xs = std::tuple_cat(__unwrap(as)...);

			return std::tuple_cat(x, xs);
		}

		template <typename A, size_t... Is, typename... As>
		std::tuple<As...> __drop_one_impl(std::tuple<A, As...> tup, std::index_sequence<Is...> seq)
		{
			return std::make_tuple(std::get<(1+Is)>(tup)...);
		}

		template <typename A, typename... As>
		std::tuple<As...> __drop_one(std::tuple<A, As...> tup)
		{
			return __drop_one_impl(tup, std::make_index_sequence<sizeof...(As)>());
		}

		[[maybe_unused]] std::optional<std::tuple<>> __transpose()
		{
			return std::make_tuple();
		}

		template <typename A>
		[[maybe_unused]] std::optional<std::tuple<A>> __transpose(std::tuple<std::optional<A>> tup)
		{
			auto elm = std::get<0>(tup);
			if(!elm.has_value())
				return std::nullopt;

			return elm.value();
		}

		template <typename A, typename... As, typename = std::enable_if_t<sizeof...(As) != 0>>
		[[maybe_unused]] std::optional<std::tuple<A, As...>> __transpose(std::tuple<std::optional<A>, std::optional<As>...> tup)
		{
			auto elm = std::get<0>(tup);
			if(!elm.has_value())
				return std::nullopt;

			auto next = __transpose(__drop_one(tup));
			if(!next.has_value())
				return std::nullopt;

			return std::tuple_cat(std::make_tuple(elm.value()), next.value());
		}



		[[maybe_unused]] std::tuple<> __get_error() { return { }; }

		template <typename A, typename = std::enable_if_t<is_result<A>::value>>
		[[maybe_unused]] std::tuple<typename A::error_type> __get_error(const A& a)
		{
			if(a) return typename A::error_type();
			return a.error();
		}

		template <typename A, typename = std::enable_if_t<!is_result<A>::value>>
		[[maybe_unused]] std::tuple<> __get_error(const A& a) { return std::tuple<>(); }


		template <typename A, typename... As>
		auto __get_error(const A& a, As&&... as)
		{
			auto x = __get_error(std::forward<const A&>(a));
			auto xs = std::tuple_cat(__get_error(as)...);

			return std::tuple_cat(x, xs);
		}

		// this is named 'concat', but just take the first one.
		template <typename... Err>
		Error __concat_errors(Err&&... errs)
		{
			Error ret;
			([&ret](auto e) { if(ret.msg.empty()) ret = e; }(errs), ...);

			return ret;
		}
	}


	template <typename Ast, typename... Args>
	static ResultTy makeAST(Args&&... args)
	{
		auto opts = __unwrap(std::forward<Args&&>(args)...);
		auto opt = __transpose(opts);
		if(opt.has_value())
		{
			auto foozle = [](auto... xs) -> Ast* {
				return new Ast(xs...);
			};

			return Ok<Ast*>(std::apply(foozle, opt.value()));
		}

		return Err(std::apply([](auto&&... xs) { return __concat_errors(xs...); }, __get_error(std::forward<Args&&>(args)...)));
	}




	static int get_binary_precedence(TT op)
	{
		switch(op)
		{
			case TT::And:               return 800;
			case TT::Or:                return 800;

			case TT::LeftArrow:         return 700;
			case TT::RightArrow:        return 700;
			case TT::DoubleArrow:       return 700;

			default:
				return -1;
		}
	}

	static bool is_right_associative(TT op)
	{
		// trick question -- they all are.
		switch(op)
		{
			case TT::And:
			case TT::Or:
			case TT::LeftArrow:
			case TT::RightArrow:
			case TT::DoubleArrow:
				return true;

			default:
				return false;
		}
	}

	struct State
	{
		State(std::vector<Token> ts) : tokens(std::move(ts)) { }

		bool match(TT t)
		{
			if(tokens.size() == 0 || tokens.front() != t)
				return false;

			this->pop();
			return true;
		}

		const Token& peek(size_t n = 0)
		{
			return tokens.size() <= n ? eof : tokens[n];
		}

		Token pop()
		{
			if(!tokens.empty())
			{
				auto ret = this->peek();
				tokens.erase(tokens.begin());
				return ret;
			}
			else
			{
				return eof;
			}
		}

		bool empty()
		{
			return tokens.empty();
		}

		std::vector<Token> tokens;
		Token eof { TT::EndOfFile, Location { 0, 0 }, "" };
	};

	static ResultTy parseExpr(State& st);
	static ResultTy parseUnary(State& st);
	static ResultTy parseParenthesised(State& st);
	static ResultTy parseRhs(State& st, ResultTy lhs, int prio);

	ResultTy parse(zbuf::str_view str)
	{
		if(str.empty())
			return Err(Error { .msg = "empty input", .loc = Location { 0, 0 } });

		auto tokens = lex(str);
		if(!tokens) return Err(tokens.error());

		auto st = State(std::move(*tokens));

		auto ret = parseExpr(st);
		if(!ret) return ret;

		if(!st.empty())
		{
			auto foo = st.peek();
			return Err(Error {
				.msg = zpr::sprint("junk at end of expression: '{}'", foo.text),
				.loc = foo.loc
			});
		}
		else
		{
			return Ok(*ret);
		}
	}

	static ResultTy parseExpr(State& st)
	{
		auto left = parseUnary(st);
		if(!left) return left;

		return parseRhs(st, left, 0);
	}

	static ResultTy parseUnary(State& st)
	{
		if(st.peek() == TT::LParen)
		{
			return parseParenthesised(st);
		}
		else if(st.peek() == TT::Not)
		{
			st.pop();
			return makeAST<ast::Not>(parseUnary(st));
		}
		else if(st.peek() == TT::Top)
		{
			st.pop();
			return makeAST<ast::Lit>(true);
		}
		else if(st.peek() == TT::Bottom)
		{
			st.pop();
			return makeAST<ast::Lit>(false);
		}
		else if(st.peek() == TT::Identifier)
		{
			return makeAST<ast::Var>(st.pop().text.str());
		}

		return Err(Error {
			.msg = zpr::sprint("unexpected end of input"),
			.loc = Location { }
		});
	}

	static ResultTy parseParenthesised(State& st)
	{
		Location open;
		if(auto x = st.pop(); x != TT::LParen)
			return Err(Error { .msg = zpr::sprint("expected '('"), .loc = x.loc });
		else
			open = x.loc;

		auto expr = parseExpr(st);
		if(!expr) return expr;

		if(st.pop() != TT::RParen)
			return Err(Error { .msg = zpr::sprint("expected ')' to match this '('"), .loc = open });

		return expr;
	}


	static ResultTy parseRhs(State& st, ResultTy lhs, int prio)
	{
		if(!lhs || st.empty() || prio == -1)
			return lhs;

		// note that for simplicity we just treat everything as right-associative,
		// since both & and | are associative operators.
		while(true)
		{
			auto oper = st.peek();
			auto prec = get_binary_precedence(oper);
			if(prec < prio)
				return lhs;

			st.pop();

			auto rhs = parseUnary(st);
			if(!rhs) return rhs;

			auto next_op = st.peek();
			if(auto np = get_binary_precedence(next_op); np != -1)
			{
				// check for mixing & and | without parens
				if(np == prec && next_op != oper)
				{
					return Err(Error {
						.msg = zpr::sprint("cannot mix operators '{}' and '{}' without disambiguating parentheses",
							oper.text, next_op.text),
						.loc = next_op.loc
					});
				}

				if(is_right_associative(next_op))
					rhs = parseRhs(st, rhs, prec - 1);
				else
					rhs = parseRhs(st, rhs, prec + 1);

				if(!rhs) return rhs;
			}

			switch(oper)
			{
				case TT::And:
					lhs = makeAST<ast::And>(lhs, rhs);
					if(!lhs) return lhs;
					break;

				case TT::Or:
					lhs = makeAST<ast::Or>(lhs, rhs);
					if(!lhs) return lhs;
					break;

				case TT::RightArrow:
					lhs = makeAST<ast::Implies>(lhs, rhs);
					if(!lhs) return lhs;
					break;

				case TT::LeftArrow:
					lhs = makeAST<ast::Implies>(rhs, lhs);
					if(!lhs) return lhs;
					break;

				case TT::DoubleArrow:
					lhs = makeAST<ast::BidirImplies>(lhs, rhs);
					if(!lhs) return lhs;
					break;

				default:
					return Err(Error { .msg = zpr::sprint("unexpected token '{}'", oper.text), .loc = oper.loc });
			}
		}
	}
}
