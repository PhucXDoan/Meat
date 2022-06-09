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
	plus,
	minus,
	asterisk,
	forward_slash,
	period,
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

internal String load_file(strlit file_path)
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

internal void free_file(String file)
{
	free(file.data);
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
					{ '+', SymbolKind::plus              },
					{ '-', SymbolKind::minus             },
					{ '*', SymbolKind::asterisk          },
					{ '/', SymbolKind::forward_slash     },
					{ '.', SymbolKind::period            },
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

int main(void)
{
	String file = load_file(DATA_DIR "basic_expressions.meat");
	DEFER { free_file(file); };

	Tokenizer tokenizer;
	tokenizer.stream        = file;
	tokenizer.current_index = 0;

	while (true)
	{
		Token token = eat_token(&tokenizer);

		if (token.kind == TokenKind::null)
		{
			ASSERT(tokenizer.current_index == tokenizer.stream.size); // Tokenizer must've finish the stream.
			break;
		}
		else
		{
			DEBUG_printf("Token : `%.*s`\n", PASS_STRING(token.string));
		}
	}

	return 0;
}
