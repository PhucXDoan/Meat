#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "unified.h"

#define IS_ALPHA(C)         (IN_RANGE((C), 'a', 'z' + 1) && IN_RANGE((C), 'A', 'Z' + 1))
#define IS_DIGIT(C)         (IN_RANGE((C), '0', '9' + 1))
#define PASS_STRING(STRING) (STRING).size, (STRING).data
#define STRLIT(STR)         (String { sizeof(STR) - 1, STR });

struct String
{
	i32   size;
	char* data;
};

enum struct SymbolKind : u8
{
	null,
	semicolon,
	plus,
	minus,
	asterisk,
	forward_slash,
	period,
	caret,
	exclamation_point,
	parenthesis_start,
	parenthesis_end,

	parenthetical_application
};

enum struct TokenKind : u8
{
	null,
	symbol,
	number,
	identifier
};

struct Token
{
	TokenKind kind;
	String    string;
	union
	{
		struct
		{
			SymbolKind kind;
		} symbol;
	};
};

struct Tokenizer
{
	String stream;
	i32    current_index;
};

struct ExpressionTree
{
	Token           token;
	ExpressionTree* left;
	ExpressionTree* right;
};

struct ExpressionTreeAllocator
{
	ExpressionTree* memory;
	ExpressionTree* available_expression_tree;
};

// @TODO@ Handle non-existing files.
internal Tokenizer init_tokenizer_from_file(strlit file_path)
{
	FILE* file;
	errno_t file_errno = fopen_s(&file, file_path, "rb");
	ASSERT(file_errno == 0);
	DEFER { fclose(file); };

	Tokenizer tokenizer;
	tokenizer.current_index = 0;

	fseek(file, 0, SEEK_END);
	tokenizer.stream.size = ftell(file);
	tokenizer.stream.data = reinterpret_cast<char*>(malloc(tokenizer.stream.size));
	fseek(file, 0, SEEK_SET);
	fread(tokenizer.stream.data, sizeof(char), tokenizer.stream.size, file);

	return tokenizer;
}

internal void deinit_tokenizer_from_file(Tokenizer tokenizer)
{
	free(tokenizer.stream.data);
}

internal ExpressionTree* init_single_expression_tree(ExpressionTreeAllocator* allocator, Token token, ExpressionTree* left, ExpressionTree* right)
{
	ASSERT(allocator->available_expression_tree);
	ExpressionTree* allocation           = allocator->available_expression_tree;
	allocator->available_expression_tree = allocator->available_expression_tree->left;
	allocation->token = token;
	allocation->left  = left;
	allocation->right = right;
	return allocation;
}

internal void deinit_entire_expression_tree(ExpressionTreeAllocator* allocator, ExpressionTree* tree)
{
	if (tree)
	{
		if (tree->left)
		{
			deinit_entire_expression_tree(allocator, tree->left);
		}
		if (tree->right)
		{
			deinit_entire_expression_tree(allocator, tree->right);
		}

		tree->left                           = allocator->available_expression_tree;
		allocator->available_expression_tree = tree;
	}
}

internal void eat_white_space(Tokenizer* tokenizer)
{
	while (tokenizer->current_index < tokenizer->stream.size)
	{
		switch (tokenizer->stream.data[tokenizer->current_index])
		{
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			{
				tokenizer->current_index += 1;
			} break;

			default:
			{
				return;
			} break;
		}
	}
}

// @TODO@ Leading decimal support.
internal Token eat_token(Tokenizer* tokenizer)
{
	eat_white_space(tokenizer);

	if (tokenizer->current_index < tokenizer->stream.size)
	{
		if (IS_DIGIT(tokenizer->stream.data[tokenizer->current_index]))
		{
			Token token;
			token.kind        = TokenKind::number;
			token.string.size = 1;
			token.string.data = tokenizer->stream.data + tokenizer->current_index;

			bool32 has_decimal = false;
			for (tokenizer->current_index += 1; tokenizer->current_index < tokenizer->stream.size; tokenizer->current_index += 1)
			{
				if (IS_DIGIT(tokenizer->stream.data[tokenizer->current_index]))
				{
					token.string.size += 1;
				}
				else if (tokenizer->stream.data[tokenizer->current_index] == '.' && !has_decimal)
				{
					token.string.size += 1;
					has_decimal        = true;
				}
				else
				{
					if (tokenizer->stream.data[tokenizer->current_index] == '.' && has_decimal)
					{
						ASSERT(false); // A decimal has already been used in the number.
						return {};
					}

					break;
				}
			}

			return token;
		}
		else
		{
			constexpr struct { char c; SymbolKind kind; } SINGLE_SYMBOLS[] =
				{
					{ ';', SymbolKind::semicolon         },
					{ '+', SymbolKind::plus              },
					{ '-', SymbolKind::minus             },
					{ '*', SymbolKind::asterisk          },
					{ '/', SymbolKind::forward_slash     },
					{ '.', SymbolKind::period            },
					{ '^', SymbolKind::caret             },
					{ '!', SymbolKind::exclamation_point },
					{ '(', SymbolKind::parenthesis_start },
					{ ')', SymbolKind::parenthesis_end   }
				};

			FOR_ELEMS(SINGLE_SYMBOLS)
			{
				if (tokenizer->stream.data[tokenizer->current_index] == it->c)
				{
					Token token;
					token.kind        = TokenKind::symbol;
					token.string.size = 1;
					token.string.data = tokenizer->stream.data + tokenizer->current_index;
					token.symbol.kind = it->kind;

					tokenizer->current_index += token.string.size;

					return token;
				}
			}

			ASSERT(false); // Couldn't parse for a token.
			return {};
		}
	}
	else
	{
		return {};
	}
}

internal Token peek_token(Tokenizer tokenizer)
{
	return eat_token(&tokenizer);
}

internal bool32 get_operator_info(i32* precedence, bool32* is_right_associative, SymbolKind kind)
{
	switch (kind)
	{
		case SymbolKind::exclamation_point:
		*precedence           = 4;
		*is_right_associative = false;
		return true;

		case SymbolKind::caret:
		*precedence           = 3;
		*is_right_associative = true;
		return true;

		case SymbolKind::parenthetical_application:
		*precedence           = 2;
		*is_right_associative = false;
		return true;

		case SymbolKind::asterisk:
		case SymbolKind::forward_slash:
		*precedence           = 1;
		*is_right_associative = false;
		return true;

		case SymbolKind::plus:
		case SymbolKind::minus:
		*precedence           = 0;
		*is_right_associative = false;
		return true;
	}

	return false;
}

// @NOTE@ https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
internal ExpressionTree* eat_expression(Tokenizer* tokenizer, ExpressionTreeAllocator* allocator, i32 min_precedence = 0)
{
	Token parenthetical_application_token;
	parenthetical_application_token.kind        = TokenKind::symbol;
	parenthetical_application_token.string      = STRLIT("()");
	parenthetical_application_token.symbol.kind = SymbolKind::parenthetical_application;

	i32    parenthetical_application_precedence;
	bool32 parenthetical_application_is_right_associative;
	bool32 is_parenthetical_application_operator = get_operator_info(&parenthetical_application_precedence, &parenthetical_application_is_right_associative, SymbolKind::parenthetical_application);
	ASSERT(is_parenthetical_application_operator);

	Token multiplication_token;
	multiplication_token.kind        = TokenKind::symbol;
	multiplication_token.string      = STRLIT("*");
	multiplication_token.symbol.kind = SymbolKind::asterisk;

	i32    multiplication_precedence;
	bool32 multiplication_is_right_associative;
	bool32 is_multiplication_operator = get_operator_info(&multiplication_precedence, &multiplication_is_right_associative, SymbolKind::asterisk);
	ASSERT(is_multiplication_operator);

	ExpressionTree* current_tree;

	{
		Token token = peek_token(*tokenizer);

		if (token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::parenthesis_start)
		{
			eat_token(tokenizer);
			current_tree = eat_expression(tokenizer, allocator, 0);
			ASSERT(current_tree);
			token = eat_token(tokenizer);
			ASSERT(token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::parenthesis_end);
			current_tree = init_single_expression_tree(allocator, parenthetical_application_token, 0, current_tree);
		}
		else if (token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::minus)
		{
			eat_token(tokenizer);
			current_tree = init_single_expression_tree(allocator, token, 0, eat_expression(tokenizer, allocator, multiplication_precedence + (multiplication_is_right_associative ? 0 : 1)));
		}
		else if (token.kind == TokenKind::number)
		{
			eat_token(tokenizer);
			current_tree = init_single_expression_tree(allocator, token, 0, 0);
		}
		else
		{
			return 0;
		}
	}

	while (true)
	{
		Token  token = peek_token(*tokenizer);
		i32    operator_precedence;
		bool32 operator_is_right_associative;
		if (token.kind == TokenKind::symbol && get_operator_info(&operator_precedence, &operator_is_right_associative, token.symbol.kind) && operator_precedence >= min_precedence)
		{
			eat_token(tokenizer);
			if (token.symbol.kind == SymbolKind::exclamation_point)
			{
				current_tree = init_single_expression_tree(allocator, token, current_tree, 0);
			}
			else
			{
				current_tree = init_single_expression_tree(allocator, token, current_tree, eat_expression(tokenizer, allocator, operator_precedence + (operator_is_right_associative ? 0 : 1)));
				ASSERT(current_tree->right);
			}
		}
		else if (token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::parenthesis_start && parenthetical_application_precedence >= min_precedence)
		{ // @TODO@ This is assuming all parenthetical applications are multiplications.
			eat_token(tokenizer);
			ExpressionTree* right_hand_side = eat_expression(tokenizer, allocator, 0);
			token = eat_token(tokenizer);
			ASSERT(token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::parenthesis_end);
			current_tree = init_single_expression_tree(allocator, parenthetical_application_token, 0, init_single_expression_tree(allocator, parenthetical_application_token, current_tree, right_hand_side));
		}
		else if (token.kind == TokenKind::number && current_tree->token.kind == TokenKind::symbol && current_tree->token.symbol.kind == SymbolKind::parenthetical_application && parenthetical_application_precedence >= min_precedence)
		{
			current_tree = init_single_expression_tree(allocator, multiplication_token, current_tree, eat_expression(tokenizer, allocator, multiplication_precedence));
		}
		else
		{
			break;
		}
	}

	return current_tree;
}

// @TODO@ Make this more robust.
internal f32 parse_number(String string)
{
	char buffer[64];
	sprintf_s(buffer, sizeof(buffer), "%.*s", PASS_STRING(string));
	f32 result;
	sscanf_s(buffer, "%f", &result);
	return result;
}

#if DEBUG
internal void DEBUG_print_expression_tree(ExpressionTree* tree, i32 depth = 0, u64 path = 0)
{
	if (tree)
	{
		if (!tree->left && !tree->right)
		{
			DEBUG_printf("Leaf : ");
			FOR_RANGE(i, depth)
			{
				DEBUG_printf("%llu", (path >> i) & 0b1);
			}

			DEBUG_printf(" : `%.*s`\n", PASS_STRING(tree->token.string));
		}
		else
		{
			DEBUG_printf("Branch : ");
			FOR_RANGE(i, depth)
			{
				DEBUG_printf("%llu", (path >> i) & 0b1);
			}
			DEBUG_printf(" : `%.*s`\n", PASS_STRING(tree->token.string));

			if (tree->left)
			{
				DEBUG_print_expression_tree(tree->left, depth + 1, path & ~(1ULL << depth));
			}
			if (tree->right)
			{
				DEBUG_print_expression_tree(tree->right, depth + 1, path | (1ULL << depth));
			}
		}
	}
}

internal bool32 DEBUG_token_eq(Token a, Token b)
{
	if (a.kind == TokenKind::symbol && b.kind == TokenKind::symbol && a.symbol.kind != b.symbol.kind)
	{
		return false;
	}

	return a.kind == b.kind && memcmp(a.string.data, b.string.data, a.string.size) == 0;
}

internal bool32 DEBUG_expression_tree_eq(ExpressionTree* a, ExpressionTree* b)
{
	if (a == b)
	{
		return true;
	}
	else if (a && b)
	{
		return DEBUG_token_eq(a->token, b->token) && DEBUG_expression_tree_eq(a->left, b->left) && DEBUG_expression_tree_eq(a->right, b->right);
	}
	else
	{
		return false;
	}
}

internal f32 DEBUG_evaluate_expression_tree(ExpressionTree* tree)
{
	ASSERT(tree);

	switch (tree->token.kind)
	{
		case TokenKind::symbol:
		{
			switch (tree->token.symbol.kind)
			{
				case SymbolKind::plus:
				{
					return DEBUG_evaluate_expression_tree(tree->left) + DEBUG_evaluate_expression_tree(tree->right);
				} break;

				case SymbolKind::minus:
				{
					if (tree->left)
					{
						return DEBUG_evaluate_expression_tree(tree->left) - DEBUG_evaluate_expression_tree(tree->right);
					}
					else
					{
						return -DEBUG_evaluate_expression_tree(tree->right);
					}
				} break;

				case SymbolKind::asterisk:
				{
					return DEBUG_evaluate_expression_tree(tree->left) * DEBUG_evaluate_expression_tree(tree->right);
				} break;

				case SymbolKind::forward_slash:
				{
					return DEBUG_evaluate_expression_tree(tree->left) / DEBUG_evaluate_expression_tree(tree->right);
				} break;

				case SymbolKind::caret:
				{
					return powf(DEBUG_evaluate_expression_tree(tree->left), DEBUG_evaluate_expression_tree(tree->right));
				} break;

				case SymbolKind::exclamation_point:
				{
					ASSERT(!tree->right);
					return static_cast<f32>(tgamma(DEBUG_evaluate_expression_tree(tree->left) + 1.0));
				} break;

				case SymbolKind::parenthetical_application:
				{
					if (tree->left)
					{ // @TODO@ This is assuming all parenthetical applications are multiplications.
						return DEBUG_evaluate_expression_tree(tree->left) * DEBUG_evaluate_expression_tree(tree->right);
					}
					else
					{
						return DEBUG_evaluate_expression_tree(tree->right);
					}
				} break;

				default:
				{
					ASSERT(false); // Unknown symbol.
					return 0.0f;
				} break;
			}
		} break;

		case TokenKind::number:
		{
			ASSERT(!tree->left);
			ASSERT(!tree->right);
			return parse_number(tree->token.string);
		} break;

		case TokenKind::identifier:
		{
			ASSERT(false); // @TODO@
			return 0.0f;
		} break;

		default:
		{
			ASSERT(false); // Unknown token.
			return 0.0f;
		} break;
	}
}
#endif

int main(void)
{
	Tokenizer tokenizer = init_tokenizer_from_file(DATA_DIR "basic_expressions.meat");
	DEFER { deinit_tokenizer_from_file(tokenizer); };

	constexpr i32 EXPRESSION_TREE_COUNT = 64;
	ExpressionTreeAllocator expression_tree_allocator;
	expression_tree_allocator.memory                    = reinterpret_cast<ExpressionTree*>(malloc(EXPRESSION_TREE_COUNT * sizeof(ExpressionTree)));
	expression_tree_allocator.available_expression_tree = expression_tree_allocator.memory;
	DEFER
	{
		i32 counter = 0;
		for (ExpressionTree* tree = expression_tree_allocator.available_expression_tree; tree; tree = tree->left)
		{
			counter += 1;
		}
		ASSERT(counter == EXPRESSION_TREE_COUNT);
		free(expression_tree_allocator.memory);
	};
	FOR_RANGE(i, EXPRESSION_TREE_COUNT - 1)
	{
		expression_tree_allocator.available_expression_tree[i].left = &expression_tree_allocator.available_expression_tree[i + 1];
	}
	expression_tree_allocator.available_expression_tree[EXPRESSION_TREE_COUNT - 1].left = 0;

	{
		ExpressionTree* expression_tree = eat_expression(&tokenizer, &expression_tree_allocator);
		DEFER { deinit_entire_expression_tree(&expression_tree_allocator, expression_tree); };
		Token terminating_token = eat_token(&tokenizer);
		DEBUG_print_expression_tree(expression_tree);
		ASSERT(terminating_token.kind == TokenKind::symbol && terminating_token.symbol.kind == SymbolKind::semicolon);

		DEBUG_printf(">> %f\n", DEBUG_evaluate_expression_tree(expression_tree));
	}

	tokenizer.current_index = 0;
	while (true)
	{
		Token token = eat_token(&tokenizer);

		if (token.kind == TokenKind::null)
		{
			ASSERT(tokenizer.current_index == tokenizer.stream.size); // Tokenizer must finish the stream.
			break;
		}
		else
		{
			if (token.kind == TokenKind::number)
			{
				DEBUG_printf("Token : `%.*s` : %f\n", PASS_STRING(token.string), parse_number(token.string));
			}
			else
			{
				DEBUG_printf("Token : `%.*s`\n", PASS_STRING(token.string));
			}
		}
	}

	return 0;
}
