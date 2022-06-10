#include <stdio.h>
#include <stdlib.h>
#include "unified.h"

#define IS_ALPHA(C)         (IN_RANGE((C), 'a', 'z' + 1) && IN_RANGE((C), 'A', 'Z' + 1))
#define IS_DIGIT(C)         (IN_RANGE((C), '0', '9' + 1))
#define PASS_STRING(STRING) (STRING).size, (STRING).data

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
	parenthesis_start,
	parenthesis_end,
};

enum struct TokenKind : u8
{
	null,
	symbol,
	number
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

// @TODO@ Handle non-existing file.
internal String init_string_from_file(strlit file_path)
{
	FILE* file;
	errno_t file_errno = fopen_s(&file, file_path, "rb");
	ASSERT(file_errno == 0);
	DEFER { fclose(file); };

	String string;

	fseek(file, 0, SEEK_END);
	string.size = ftell(file);
	string.data = reinterpret_cast<char*>(malloc(string.size));
	fseek(file, 0, SEEK_SET);
	fread(string.data, sizeof(char), string.size, file);

	return string;
}

internal void deinit_string_from_file(String file)
{
	free(file.data);
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

internal i32 get_operator_value(SymbolKind kind)
{
	switch (kind)
	{
		case SymbolKind::caret:
		return 3;

		case SymbolKind::asterisk:
		case SymbolKind::forward_slash:
		return 2;

		case SymbolKind::plus:
		case SymbolKind::minus:
		return 1;

		default:
		return 0;
	}
}

internal i32 get_operator_value(Token token)
{
	if (token.kind == TokenKind::symbol)
	{
		return get_operator_value(token.symbol.kind);
	}
	else
	{
		return 0;
	}
}

enum struct Associativity : u8
{
	null,
	left,
	right
};

internal Associativity get_operator_associativity(SymbolKind kind)
{
	switch (kind)
	{
		case SymbolKind::plus:
		case SymbolKind::minus:
		case SymbolKind::asterisk:
		case SymbolKind::forward_slash:
		return Associativity::left;

		case SymbolKind::caret:
		return Associativity::right;

		default:
		return Associativity::null;
	}
}

internal Associativity get_operator_associativity(Token token)
{
	if (token.kind == TokenKind::symbol)
	{
		return get_operator_associativity(token.symbol.kind);
	}
	else
	{
		return Associativity::null;
	}
}

internal Token peek_token(Tokenizer tokenizer)
{
	return eat_token(&tokenizer);
}

// @NOTE@ https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
internal ExpressionTree* eat_expression(Tokenizer* tokenizer, ExpressionTreeAllocator* allocator, i32 min_precendence = 1)
{
	ExpressionTree* current_tree;
	{
		Token token = peek_token(*tokenizer);

		if (token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::semicolon)
		{
			return 0;
		}
		else if (token.kind == TokenKind::number)
		{
			eat_token(tokenizer);
			current_tree = init_single_expression_tree(allocator, token, 0, 0);
		}
		else
		{
			ASSERT(false); // Unknown token.
			return 0;
		}
	}

	while (true)
	{
		Token token = peek_token(*tokenizer);

		if (token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::semicolon)
		{
			break;
		}
		else
		{
			ASSERT(get_operator_value(token));

			if (get_operator_value(token) >= min_precendence)
			{
				eat_token(tokenizer);

				i32 next_min_precendence = get_operator_value(token);
				if (get_operator_associativity(token) == Associativity::left)
				{
					next_min_precendence += 1;
				}
				else
				{
					ASSERT(get_operator_associativity(token) == Associativity::right);
				}

				current_tree = init_single_expression_tree(allocator, token, current_tree, eat_expression(tokenizer, allocator, next_min_precendence));
			}
			else
			{
				break;
			}
		}
	}

	return current_tree;
}

#if DEBUG
internal void DEBUG_print_expression_tree(ExpressionTree* tree, i32 depth, u32 path)
{
	if (tree)
	{
		if (!tree->left && !tree->right)
		{
			DEBUG_printf("Leaf : ");
			FOR_RANGE(i, depth)
			{
				DEBUG_printf("%i", (path >> i) & 0b1);
			}

			DEBUG_printf(" : `%.*s`\n", PASS_STRING(tree->token.string));
		}
		else
		{
			DEBUG_printf("Branch : ");
			FOR_RANGE(i, depth)
			{
				DEBUG_printf("%i", (path >> i) & 0b1);
			}
			DEBUG_printf(" : `%.*s`\n", PASS_STRING(tree->token.string));

			if (tree->left)
			{
				DEBUG_print_expression_tree(tree->left, depth + 1, path & ~(1 << depth));
			}
			if (tree->right)
			{
				DEBUG_print_expression_tree(tree->right, depth + 1, path | (1 << depth));
			}
		}
	}
}
#endif

int main(void)
{
	String file_string = init_string_from_file(DATA_DIR "basic_expressions.meat");
	DEFER { deinit_string_from_file(file_string); };

	Tokenizer tokenizer;
	tokenizer.stream        = file_string;
	tokenizer.current_index = 0;

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

	#if 0
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
			DEBUG_printf("Token : `%.*s`\n", PASS_STRING(token.string));
		}
	}
	#else
	ExpressionTree* expression_tree = eat_expression(&tokenizer, &expression_tree_allocator);

	DEBUG_print_expression_tree(expression_tree, 0, 0);

	DEFER { deinit_entire_expression_tree(&expression_tree_allocator, expression_tree); };
	#endif

	return 0;
}
