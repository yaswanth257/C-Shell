#define _POSIX_C_SOURCE 200809L

#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 1024
#define MAX_TOKEN_SIZE 256

typedef enum
{
    TOKEN_NAME,
    TOKEN_PIPE,
    TOKEN_AMPERSAND,
    TOKEN_AND,
    TOKEN_SEMICOLON,
    TOKEN_INPUT_REDIRECT,
    TOKEN_OUTPUT_REDIRECT,
    TOKEN_APPEND_REDIRECT,
    TOKEN_END,
    TOKEN_INVALID
} token_type_t;

typedef struct
{
    token_type_t type;
    char value[MAX_TOKEN_SIZE];
} token_t;

typedef struct
{
    token_t tokens[MAX_TOKENS];
    int token_count;
    int current_token;
} parser_state_t;

static const char *skip_whitespace(const char *str)
{
    while (*str && isspace((unsigned char)*str))
    {
        str++;
    }
    return str;
}

static int is_name_char(char c)
{
    return c != '\0' && c != '&' && c != '>' && c != '<' && c != ';' &&
           c != '|' && !isspace((unsigned char)c);
}
// ############## LLM Generated Code Begins ##############

int tokenize_input(const char *input, parser_state_t *parser)
{
    parser->token_count = 0;
    parser->current_token = 0;
    const char *p = input;

    while (*p && parser->token_count < MAX_TOKENS - 1)
    {
        p = skip_whitespace(p);
        if (*p == '\0')
            break;

        token_t *token = &parser->tokens[parser->token_count];

        if (*p == '&' && *(p + 1) == '&')
        {
            token->type = TOKEN_AND;
            strcpy(token->value, "&&");
            p += 2;
        }
        else if (*p == '>' && *(p + 1) == '>')
        {
            token->type = TOKEN_APPEND_REDIRECT;
            strcpy(token->value, ">>");
            p += 2;
        }
        else if (*p == '|')
        {
            token->type = TOKEN_PIPE;
            strcpy(token->value, "|");
            p++;
        }
        else if (*p == '&')
        {
            token->type = TOKEN_AMPERSAND;
            strcpy(token->value, "&");
            p++;
        }
        else if (*p == ';')
        {
            token->type = TOKEN_SEMICOLON;
            strcpy(token->value, ";");
            p++;
        }
        else if (*p == '<')
        {
            token->type = TOKEN_INPUT_REDIRECT;
            strcpy(token->value, "<");
            p++;
        }
        else if (*p == '>')
        {
            token->type = TOKEN_OUTPUT_REDIRECT;
            strcpy(token->value, ">");
            p++;
        }
        else if (is_name_char(*p))
        {
            token->type = TOKEN_NAME;
            int i = 0;
            while (*p && is_name_char(*p) && i < MAX_TOKEN_SIZE - 1)
            {
                token->value[i++] = *p++;
            }
            token->value[i] = '\0';
        }
        else
        {
            token->type = TOKEN_INVALID;
            token->value[0] = *p;
            token->value[1] = '\0';
            return -1;
        }

        parser->token_count++;
    }

    parser->tokens[parser->token_count].type = TOKEN_END;
    strcpy(parser->tokens[parser->token_count].value, "");
    parser->token_count++;
    return 0;
}
static token_t *current_token(parser_state_t *parser)
{
    if (parser->current_token >= parser->token_count)
        return &parser->tokens[parser->token_count - 1];
    return &parser->tokens[parser->current_token];
}

static void advance_token(parser_state_t *parser)
{
    if (parser->current_token < parser->token_count - 1)
        parser->current_token++;
}

static int match_token(parser_state_t *parser, token_type_t type)
{
    return current_token(parser)->type == type;
}

static int consume_token(parser_state_t *parser, token_type_t type)
{
    if (match_token(parser, type))
    {
        advance_token(parser);
        return 1;
    }
    return 0;
}

int parse_atomic(parser_state_t *parser)
{
    if (!consume_token(parser, TOKEN_NAME))
        return -1;

    while (1)
    {
        if (match_token(parser, TOKEN_NAME))
        {
            advance_token(parser);
        }
        else if (match_token(parser, TOKEN_INPUT_REDIRECT))
        {
            advance_token(parser);
            if (!consume_token(parser, TOKEN_NAME))
                return -1;
        }
        else if (match_token(parser, TOKEN_OUTPUT_REDIRECT) || match_token(parser, TOKEN_APPEND_REDIRECT))
        {
            advance_token(parser);
            if (!consume_token(parser, TOKEN_NAME))
                return -1;
        }
        else
        {
            break;
        }
    }
    return 0;
}

// ############## LLM Generated Code Ends ################
int parse_cmd_group(parser_state_t *parser)
{
    if (parse_atomic(parser) != 0)
        return -1;

    while (match_token(parser, TOKEN_PIPE))
    {
        advance_token(parser);
        if (parse_atomic(parser) != 0)
            return -1;
    }
    return 0;
}

int parse_shell_cmd(parser_state_t *parser)
{
    if (parse_cmd_group(parser) != 0)
        return -1;

    while (match_token(parser, TOKEN_SEMICOLON))
    {
        advance_token(parser);

        if (match_token(parser, TOKEN_END))
            break;

        if (parse_cmd_group(parser) != 0)
            return -1;
    }

    consume_token(parser, TOKEN_AMPERSAND);

    if (!match_token(parser, TOKEN_END))
        return -1;

    return 0;
}

int parse_command(const char *input)
{
    parser_state_t parser;

    if (tokenize_input(input, &parser) != 0)
        return -1;

    if (parser.token_count == 1 && parser.tokens[0].type == TOKEN_END)
        return 0;

    if (parser.token_count > 1)
    {
        token_type_t first = parser.tokens[0].type;
        if (first == TOKEN_PIPE || first == TOKEN_AND || first == TOKEN_AMPERSAND ||
            first == TOKEN_INPUT_REDIRECT || first == TOKEN_OUTPUT_REDIRECT || first == TOKEN_APPEND_REDIRECT)
            return -1;
    }

    if (parse_shell_cmd(&parser) != 0)
        return -1;

    return 0;
}
