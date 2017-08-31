// MacroParser.hpp
/////////////////////////////////////////////////////////////////////////

#ifndef MACROPARSER_HPP_
#define MACROPARSER_HPP_

/////////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <cstdio>
#include <cassert>

/////////////////////////////////////////////////////////////////////////
// the BNF (subset of C language:
//
// <constant_expression> ::= <conditional_expression> (',' <conditional_expression>)*
//                         | <conditional_expression>
// <conditional_expression> ::= <logical_or_expression>
//                            | <logical_or_expression> '?' <constant_expression> ':' <conditional_expression>
// <logical_or_expression> ::= <logical_and_expression> ('||' <logical_and_expression>)*
// <logical_and_expression> ::= <inclusive_or_expression> ('&&' <inclusive_or_expression>)*
// <inclusive_or_expression> ::= <exclusive_or_expression> ('|' <exclusive_or_expression>)*
// <exclusive_or_expression> ::= <and_expression> ('^' <and_expression>)*
// <and_expression> ::= <equality_expression> ('&' <equality_expression>)*
// <equality_expression> ::= <relational_expression> (('=='|'!=') <relational_expression>)*
// <relational_expression> ::= <shift_expression> (('<'|'>'|'<='|'>=') <shift_expression>)*
// <shift_expression> ::= <additive_expression> (('<<'|'>>') <additive_expression>)*
// <additive_expression> ::= <multiplicative_expression> (('+'|'-') <multiplicative_expression>)*
// <multiplicative_expression> ::= <cast_expression> (('*'|'/'|'%') <cast_expression>)*
// <cast_expression> ::= <unary_expression>
// <unary_expression> ::= <postfix_expression>
//                      | <unary_operator>* <postfix_expression>
// <postfix_expression> ::= <primary_expression>
// <primary_expression> ::= <identifier>
//                        | <constant>
//                        | <string>
//                        | '(' <constant_expression> ')'
namespace MacroParser
{
    typedef std::string string_type;

    enum TokenType
    {
        TOK_IDENT,
        TOK_INTEGER,
        TOK_STRING,
        TOK_SYMBOL,
        TOK_EOF
    };

    inline bool isdigit(char ch)
    {
        return '0' <= ch && ch <= '9';
    }
    inline bool isoctal(char ch)
    {
        return '0' <= ch && ch <= '7';
    }
    inline bool isxdigit(char ch)
    {
        return isdigit(ch) || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F');
    }
    inline bool islower(char ch)
    {
        return 'a' <= ch && ch <= 'z';
    }
    inline bool isupper(char ch)
    {
        return 'A' <= ch && ch <= 'Z';
    }
    inline bool isalpha(char ch)
    {
        return islower(ch) || isupper(ch);
    }
    inline bool isalnum(char ch)
    {
        return isalpha(ch) || isdigit(ch);
    }
#undef iscsymf
#undef iscsym
    inline bool iscsymf(char ch)
    {
        return isalpha(ch) || ch == '_';
    }
    inline bool iscsym(char ch)
    {
        return isalnum(ch) || ch == '_';
    }
    inline bool isspace(char ch)
    {
        return strchr(" \t\n\r\f\v", ch) != NULL;
    }

    /////////////////////////////////////////////////////////////////////////

    class Token
    {
    public:
        string_type m_str;
        TokenType m_type;
        int m_integer;

        Token(string_type str, TokenType type)
            : m_str(str), m_type(type), m_integer(0)
        {
        }
        Token(string_type str, TokenType type, int i)
            : m_str(str), m_type(type), m_integer(i)
        {
        }

        void print() const
        {
            std::printf("(%d %s %d)", (int)m_type, m_str.c_str(), m_integer);
        }
    };
    typedef std::vector<Token> tokens_type;

    /////////////////////////////////////////////////////////////////////////

    class StringScanner
    {
    public:
        StringScanner(const string_type& str) : m_str(str), m_index(0)
        {
        }

        char getch()
        {
            if (m_index < m_str.size())
                return m_str[m_index++];
            return -1;
        }
        void ungetch()
        {
            if (m_index > 0)
                --m_index;
        }
        const char *peek() const
        {
            return &m_str[m_index];
        }
        bool match_get(const char *psz, string_type& str)
        {
            size_t len = strlen(psz);
            if (memcmp(peek(), psz, len) == 0)
            {
                str = psz;
                skip(len);
                return true;
            }
            return false;
        }
        void skip(size_t count)
        {
            if (m_index + count <= m_str.size())
                m_index += count;
        }

        string_type get_quoted();

    protected:
        string_type     m_str;
        size_t          m_index;
    };

    /////////////////////////////////////////////////////////////////////////

    class TokenStream
    {
    public:
        tokens_type     m_tokens;

        TokenStream(StringScanner& scanner);
        bool read_tokens();

              Token& token();
        const Token& token() const;
        void unget(size_t count = 1);
        bool next();

        size_t index() const;
        bool index(size_t i);
        size_t size() const;

        TokenType type() const;
        string_type str() const;
        int integer() const;

        void print() const;

        void push_back(const Token& t);

    protected:
        size_t          m_index;
        StringScanner&  m_scanner;

        char getch()
        {
            return m_scanner.getch();
        }
        void ungetch()
        {
            m_scanner.ungetch();
        }
    };

    /////////////////////////////////////////////////////////////////////////
    // AST

    enum AstID
    {
        ASTID_BINARY,
        ASTID_COMMA,
        ASTID_IDENT,
        ASTID_INTEGER,
        ASTID_STRING,
        ASTID_TRIPLE,
        ASTID_UNARY
    };

    struct BaseAst
    {
        AstID m_id;

        BaseAst(AstID id) : m_id(id)
        {
        }
        virtual ~BaseAst()
        {
        }
        virtual void print() const
        {
            std::printf("[AST#%d]", (int)m_id);
        }
    };

    struct IdentAst : public BaseAst
    {
        string_type     m_name;

        IdentAst(const string_type& name) : BaseAst(ASTID_IDENT), m_name(name)
        {
        }
        virtual void print() const
        {
            BaseAst::print();
            std::printf(":%s.", m_name.c_str());
        }
    };

    struct IntegerAst : public BaseAst
    {
        int     m_value;

        IntegerAst(int value) : BaseAst(ASTID_INTEGER), m_value(value)
        {
        }
        virtual void print() const
        {
            BaseAst::print();
            std::printf(":%d.", m_value);
        }
    };

    struct StringAst : public BaseAst
    {
        string_type m_str;

        StringAst(const string_type& str) : BaseAst(ASTID_STRING), m_str(str)
        {
        }
        virtual void print() const
        {
            BaseAst::print();
            std::printf(":%s.", m_str.c_str());
        }
    };

    struct UnaryAst : public BaseAst
    {
        string_type m_str;
        BaseAst *m_arg;

        UnaryAst(const string_type& str, BaseAst *arg = NULL)
            : BaseAst(ASTID_UNARY), m_str(str), m_arg(arg)
        {
        }
        ~UnaryAst()
        {
            delete m_arg;
        }
        virtual void print() const
        {
            BaseAst::print();
            std::printf(":%s;", m_str.c_str());
            m_arg->print();
        }
    };

    struct BinaryAst : public BaseAst
    {
        string_type m_str;
        BaseAst *m_left;
        BaseAst *m_right;

        BinaryAst(const string_type& str, BaseAst *left, BaseAst *right)
            : BaseAst(ASTID_BINARY), m_str(str), m_left(left), m_right(right)
        {
        }
        ~BinaryAst()
        {
            delete m_left;
            delete m_right;
        }
        virtual void print() const
        {
            BaseAst::print();
            std::printf(":%s<", m_str.c_str());
            m_left->print();
            std::printf(",");
            m_right->print();
            std::printf(">");
        }
    };

    struct TripleAst : public BaseAst
    {
        string_type m_str;
        BaseAst *m_first;
        BaseAst *m_second;
        BaseAst *m_third;

        TripleAst(const string_type& str,
                  BaseAst *first, BaseAst *second, BaseAst *third)
            : BaseAst(ASTID_BINARY), m_str(str),
              m_first(first), m_second(second), m_third(third)
        {
        }
        ~TripleAst()
        {
            delete m_first;
            delete m_second;
            delete m_third;
        }
        virtual void print() const
        {
            BaseAst::print();
            std::printf(":%s<", m_str.c_str());
            m_first->print();
            std::printf(",");
            m_second->print();
            std::printf(",");
            m_third->print();
            std::printf(">");
        }
    };

    struct CommaAst : public BaseAst
    {
        std::vector<BaseAst *> m_args;

        CommaAst() : BaseAst(ASTID_COMMA)
        {
        }
        ~CommaAst()
        {
            for (size_t i = 0; i < m_args.size(); ++i)
            {
                delete m_args[i];
            }
        }
        void add(BaseAst *ast)
        {
            m_args.push_back(ast);
        }
        virtual void print() const
        {
            BaseAst::print();
            std::printf("<");
            if (m_args.size())
            {
                m_args[0]->print();
                for (size_t i = 1; i < m_args.size(); ++i)
                {
                    std::printf(",");
                    m_args[i]->print();
                }
            }
            std::printf(">");
        }
    };

    /////////////////////////////////////////////////////////////////////////

    class Parser
    {
    public:
        Parser(TokenStream& stream) : m_stream(stream), m_ast(NULL)
        {
        }
        ~Parser()
        {
            delete m_ast;
        }

        BaseAst *ast() const
        {
            return m_ast;
        }

        bool parse();

        BaseAst *visit_constant_expression();
        BaseAst *visit_conditional_expression();
        BaseAst *visit_logical_or_expression();
        BaseAst *visit_logical_and_expression();
        BaseAst *visit_inclusive_or_expression();
        BaseAst *visit_exclusive_or_expression();
        BaseAst *visit_and_expression();
        BaseAst *visit_equality_expression();
        BaseAst *visit_relational_expression();
        BaseAst *visit_shift_expression();
        BaseAst *visit_additive_expression();
        BaseAst *visit_multiplicative_expression();
        BaseAst *visit_cast_expression()
        {
            return visit_unary_expression();
        }
        BaseAst *visit_unary_expression();
        BaseAst *visit_postfix_expression()
        {
            return visit_primary_expression();
        }
        BaseAst *visit_primary_expression();

    protected:
        TokenStream m_stream;
        BaseAst *m_ast;

        size_t index() const
        {
            return m_stream.index();
        }
        void index(size_t i)
        {
            m_stream.index(i);
        }
        bool next()
        {
            return m_stream.next();
        }
        Token& token()
        {
            return m_stream.token();
        }
        const Token& token() const
        {
            return m_stream.token();
        }
        TokenType type() const
        {
            return m_stream.type();
        }
        string_type str() const
        {
            return m_stream.str();
        }
    };

    bool eval_ast(const BaseAst *ast, int& value);

    /////////////////////////////////////////////////////////////////////////

    inline TokenStream::TokenStream(StringScanner& scanner)
        : m_index(0), m_scanner(scanner)
    {
    }

    inline void TokenStream::unget(size_t count/* = 1*/)
    {
        if (count <= m_index)
            m_index -= count;
        else
            m_index = 0;
    }

    inline bool TokenStream::next()
    {
        if (m_index + 1 < size())
        {
            ++m_index;
            return true;
        }
        return false;
    }

    inline void TokenStream::push_back(const Token& t)
    {
        m_tokens.push_back(t);
    }

    inline Token& TokenStream::token()
    {
        assert(m_index <= size());
        return m_tokens[m_index];
    }

    inline const Token& TokenStream::token() const
    {
        assert(m_index <= size());
        return m_tokens[m_index];
    }

    inline TokenType TokenStream::type() const
    {
        return token().m_type;
    }

    inline string_type TokenStream::str() const
    {
        return token().m_str;
    }

    inline int TokenStream::integer() const
    {
        return token().m_integer;
    }

    inline size_t TokenStream::index() const
    {
        return m_index;
    }

    inline size_t TokenStream::size() const
    {
        return m_tokens.size();
    }

    inline bool TokenStream::index(size_t i)
    {
        if (i <= size())
        {
            m_index = i;
            return true;
        }
        return false;
    }

    inline void TokenStream::print() const
    {
        std::printf("#: %d\n", (int)size());
        size_t i = 0;
        tokens_type::const_iterator it, end = m_tokens.end();
        for (it = m_tokens.begin(); it != end; ++it)
        {
            std::printf("token#%d: ", (int)i++);
            it->print();
            std::printf("\n");
        }
    }

    inline bool TokenStream::read_tokens()
    {
        m_tokens.clear();

        char ch;
        string_type str;
        for (;;)
        {
            ch = getch();
            if (isspace(ch))
            {
                continue;
            }
            else if (isdigit(ch))
            {
                str.clear();
                str += ch;
                for (;;)
                {
                    ch = getch();
                    if (ch == -1)
                    {
                        break;
                    }
                    if (!isalnum(ch) && ch != '_')
                    {
                        ungetch();
                        break;
                    }
                    str += ch;
                }
                int i = strtol(str.c_str(), NULL, 0);
                Token token(str, TOK_INTEGER, i);
                m_tokens.push_back(token);
            }
            else if (iscsymf(ch) || ch == '_')
            {
                str.clear();
                str += ch;
                for (;;)
                {
                    ch = getch();
                    if (ch == -1)
                    {
                        break;
                    }
                    if (!iscsym(ch))
                    {
                        ungetch();
                        break;
                    }
                    str += ch;
                }
                Token token(str, TOK_IDENT);
                m_tokens.push_back(token);
            }
            else if (ch == '"')
            {
                ungetch();
                str = m_scanner.get_quoted();
                Token token(str, TOK_STRING);
                m_tokens.push_back(token);
            }
            else if (ch == -1)
            {
                Token token("", TOK_EOF);
                m_tokens.push_back(token);
                return true;
            }
            else
            {
                str.clear();
                str += ch;
                if (strchr("+-*/%~()^,?:", ch) != NULL)
                {
                    Token token(str, TOK_SYMBOL);
                    m_tokens.push_back(token);
                }
                else
                {
                    ungetch();
                    if (m_scanner.match_get("<<", str) ||
                        m_scanner.match_get("<=", str) ||
                        m_scanner.match_get("<", str) ||
                        m_scanner.match_get(">>", str) ||
                        m_scanner.match_get(">=", str) ||
                        m_scanner.match_get(">", str) ||
                        m_scanner.match_get("==", str) ||
                        m_scanner.match_get("&&", str) ||
                        m_scanner.match_get("&", str) ||
                        m_scanner.match_get("||", str) ||
                        m_scanner.match_get("|", str) ||
                        m_scanner.match_get("!=", str) ||
                        m_scanner.match_get("!", str))
                    {
                        Token token(str, TOK_SYMBOL);
                        m_tokens.push_back(token);
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }
    }

    inline string_type StringScanner::get_quoted()
    {
        string_type ret;
        char ch = getch();
        assert(ch == '"');
        ret += ch;

        string_type str;
        for (;;)
        {
            if (match_get("\"\"", str) ||
                match_get("\\\"", str) ||
                match_get("\\\\", str))
            {
                ret += str;
                continue;
            }
            ch = getch();
            if (ch == -1)
                break;
            ret += ch;
            if (ch == '"')
                break;
        }
        return ret;
    }

    inline bool Parser::parse()
    {
        if (m_stream.size() == 0)
            return false;

        delete m_ast;
        m_ast = visit_constant_expression();
        if (m_ast != NULL && type() == TOK_EOF)
            return true;

        delete m_ast;
        m_ast = NULL;
        return false;
    }

    // <constant_expression> ::= <conditional_expression> (',' <conditional_expression>)*
    //                         | <conditional_expression>
    inline BaseAst *Parser::visit_constant_expression()
    {
        BaseAst *cond = visit_conditional_expression();
        if (cond == NULL)
            return NULL;

        if (type() == TOK_SYMBOL && str() == ",")
        {
            CommaAst *comma = new CommaAst();
            do
            {
                next();
                cond = visit_conditional_expression();
                if (cond == NULL)
                {
                    delete comma;
                    return NULL;
                }
                comma->add(cond);
            } while (type() == TOK_SYMBOL && str() == ",");
            return comma;
        }
        return cond;
    }

    // <conditional_expression> ::= <logical_or_expression>
    //                            | <logical_or_expression> '?' <constant_expression> ':' <conditional_expression>
    inline BaseAst* Parser::visit_conditional_expression()
    {
        BaseAst *first = visit_logical_or_expression();
        if (first == NULL)
            return NULL;

        if (type() == TOK_SYMBOL && str() == "?")
        {
            next();
            BaseAst *second = visit_constant_expression();
            if (second)
            {
                if (type() == TOK_SYMBOL && str() == ":")
                {
                    next();
                    BaseAst *third = visit_conditional_expression();
                    if (third)
                    {
                        return new TripleAst("?", first, second, third);
                    }
                }
            }
            delete second;
            delete first;
            return NULL;
        }
        return first;
    }

    // <logical_or_expression> ::= <logical_and_expression> ('||' <logical_and_expression>)*
    inline BaseAst* Parser::visit_logical_or_expression()
    {
        BaseAst *first = visit_logical_and_expression();
        while (first)
        {
            if (type() == TOK_SYMBOL && str() == "||")
            {
                next();
                BaseAst *second = visit_logical_and_expression();
                if (second)
                {
                    first = new BinaryAst("||", first, second);
                    continue;
                }
                delete first;
                return NULL;
            }
            else
            {
                break;
            }
        }
        return first;
    }

    // <logical_and_expression> ::= <inclusive_or_expression> ('&&' <inclusive_or_expression>)*
    inline BaseAst* Parser::visit_logical_and_expression()
    {
        BaseAst *first = visit_inclusive_or_expression();
        while (first)
        {
            if (type() == TOK_SYMBOL && str() == "&&")
            {
                next();
                BaseAst *second = visit_inclusive_or_expression();
                if (second)
                {
                    first = new BinaryAst("&&", first, second);
                    continue;
                }
                delete first;
                return NULL;
            }
            else
            {
                break;
            }
        }
        return first;
    }

    // <inclusive_or_expression> ::= <exclusive_or_expression> ('|' <exclusive_or_expression>)*
    inline BaseAst* Parser::visit_inclusive_or_expression()
    {
        BaseAst *first = visit_exclusive_or_expression();
        while (first)
        {
            if (type() == TOK_SYMBOL && str() == "|")
            {
                next();
                BaseAst *second = visit_exclusive_or_expression();
                if (second)
                {
                    first = new BinaryAst("|", first, second);
                    continue;
                }
                delete first;
                return NULL;
            }
            else
            {
                break;
            }
        }
        return first;
    }

    // <exclusive_or_expression> ::= <and_expression> ('^' <and_expression>)*
    inline BaseAst* Parser::visit_exclusive_or_expression()
    {
        BaseAst *first = visit_and_expression();
        while (first)
        {
            if (type() == TOK_SYMBOL && str() == "^")
            {
                next();
                BaseAst *second = visit_and_expression();
                if (second)
                {
                    first = new BinaryAst("^", first, second);
                    continue;
                }
                delete first;
                return NULL;
            }
            else
            {
                break;
            }
        }
        return first;
    }

    // <and_expression> ::= <equality_expression> ('&' <equality_expression>)*
    inline BaseAst* Parser::visit_and_expression()
    {
        BaseAst *first = visit_equality_expression();
        while (first)
        {
            if (type() == TOK_SYMBOL && str() == "&")
            {
                next();
                BaseAst *second = visit_equality_expression();
                if (second)
                {
                    first = new BinaryAst("&", first, second);
                    continue;
                }
                delete first;
                return NULL;
            }
            else
            {
                break;
            }
        }
        return first;
    }

    // <equality_expression> ::= <relational_expression> (('=='|'!=') <relational_expression>)*
    inline BaseAst* Parser::visit_equality_expression()
    {
        BaseAst *first = visit_relational_expression();
        while (first)
        {
            string_type s = str();
            if (type() == TOK_SYMBOL && (s == "==" || s == "!="))
            {
                next();
                BaseAst *second = visit_relational_expression();
                if (second)
                {
                    first = new BinaryAst(s, first, second);
                    continue;
                }
                delete first;
                return NULL;
            }
            else
            {
                break;
            }
        }
        return first;
    }

    // <relational_expression> ::= <shift_expression> (('<'|'>'|'<='|'>=') <shift_expression>)*
    inline BaseAst* Parser::visit_relational_expression()
    {
        BaseAst *first = visit_shift_expression();
        while (first)
        {
            string_type s = str();
            if (type() == TOK_SYMBOL &&
                (s == "<" || s == ">" || s == "<=" || s == ">="))
            {
                next();
                BaseAst *second = visit_shift_expression();
                if (second)
                {
                    first = new BinaryAst(s, first, second);
                    continue;
                }
                delete first;
                return NULL;
            }
            else
            {
                break;
            }
        }
        return first;
    }

    // <shift_expression> ::= <additive_expression> (('<<'|'>>') <additive_expression>)*
    inline BaseAst* Parser::visit_shift_expression()
    {
        BaseAst *first = visit_additive_expression();
        while (first)
        {
            string_type s = str();
            if (type() == TOK_SYMBOL && (s == "<<" || s == ">>"))
            {
                next();
                BaseAst *second = visit_additive_expression();
                if (second)
                {
                    first = new BinaryAst(s, first, second);
                    continue;
                }
                delete first;
                return NULL;
            }
            else
            {
                break;
            }
        }
        return first;
    }

    // <additive_expression> ::= <multiplicative_expression> (('+'|'-') <multiplicative_expression>)*
    inline BaseAst* Parser::visit_additive_expression()
    {
        BaseAst *first = visit_multiplicative_expression();
        while (first)
        {
            string_type s = str();
            if (type() == TOK_SYMBOL && (s == "+" || s == "-"))
            {
                next();
                BaseAst *second = visit_multiplicative_expression();
                if (second)
                {
                    first = new BinaryAst(s, first, second);
                    continue;
                }
                delete first;
                return NULL;
            }
            else
            {
                break;
            }
        }
        return first;
    }

    // <multiplicative_expression> ::= <cast_expression> (('*'|'/'|'%') <cast_expression>)*
    inline BaseAst* Parser::visit_multiplicative_expression()
    {
        BaseAst *first = visit_cast_expression();
        while (first)
        {
            string_type s = str();
            if (type() == TOK_SYMBOL && (s == "*" || s == "/" || s == "%"))
            {
                next();
                BaseAst *second = visit_cast_expression();
                if (second)
                {
                    first = new BinaryAst(s, first, second);
                    continue;
                }
                delete first;
                return NULL;
            }
            else
            {
                break;
            }
        }
        return first;
    }

    // <unary_expression> ::= <postfix_expression>
    //                      | <unary_operator>* <postfix_expression>
    inline BaseAst* Parser::visit_unary_expression()
    {
        UnaryAst *ret = NULL, *first = NULL;
        while (type() == TOK_SYMBOL &&
               (str() == "+" || str() == "-" || str() == "~" || str() == "!"))
        {
            ret = new UnaryAst(str(), ret);
            if (first == NULL)
                first = ret;
            next();
        }
        if (ret)
        {
            BaseAst *postfix = visit_postfix_expression();
            if (postfix)
            {
                first->m_arg = postfix;
                return ret;
            }
            delete ret;
            return NULL;
        }
        else
        {
            return visit_postfix_expression();
        }
    }

    // <primary_expression> ::= <identifier>
    //                        | <constant>
    //                        | <string>
    //                        | '(' <constant_expression> ')'
    inline BaseAst* Parser::visit_primary_expression()
    {
        BaseAst *ast;
        switch (type())
        {
        case TOK_IDENT:
            ast = new IdentAst(str());
            next();
            return ast;
        case TOK_INTEGER:
            ast = new IntegerAst(token().m_integer);
            next();
            break;
        case TOK_STRING:
            ast = new StringAst(str());
            next();
            break;
        case TOK_SYMBOL:
            if (str() == "(")
            {
                next();
                ast = visit_constant_expression();
                if (ast)
                {
                    if (type() == TOK_SYMBOL && str() == ")")
                        return ast;
                    delete ast;
                    ast = NULL;
                }
            }
            else
            {
                ast = NULL;
            }
            break;
        case TOK_EOF:
            ast = NULL;
            break;
        }
        return ast;
    }

    inline bool eval_binary(const BaseAst *ast, int& value)
    {
        const BinaryAst *binary = (const BinaryAst *)ast;
        int left, right;
        if (!eval_ast(binary->m_left, left) || !eval_ast(binary->m_right, right))
        {
            value = 0;
            return false;
        }

        if (binary->m_str == "+") value = left + right;
        else if (binary->m_str == "-") value = left - right;
        else if (binary->m_str == "*") value = left * right;
        else if (binary->m_str == "/") value = left / right;
        else if (binary->m_str == "%") value = left % right;
        else if (binary->m_str == "^") value = left ^ right;
        else if (binary->m_str == "<<") value = left << right;
        else if (binary->m_str == "<=") value = left <= right;
        else if (binary->m_str == "<") value = left < right;
        else if (binary->m_str == ">>") value = left >> right;
        else if (binary->m_str == ">=") value = left >= right;
        else if (binary->m_str == ">") value = left > right;
        else if (binary->m_str == "==") value = left == right;
        else if (binary->m_str == "&&") value = left && right;
        else if (binary->m_str == "&") value = left & right;
        else if (binary->m_str == "||") value = left || right;
        else if (binary->m_str == "|") value = left | right;
        else if (binary->m_str == "!=") value = left != right;
        else return false;
        return true;
    }
    inline bool eval_comma(const BaseAst *ast, int& value)
    {
        const CommaAst *comma = (const CommaAst *)ast;
        if (comma->m_args.empty())
        {
            value = 0;
            return false;
        }
        return eval_ast(comma->m_args[comma->m_args.size() - 1], value);
    }
    inline bool eval_ident(const BaseAst *ast, int& value)
    {
        value = 0;
        return false;
    }
    inline bool eval_integer(const BaseAst *ast, int& value)
    {
        const IntegerAst *integer = (const IntegerAst *)ast;
        value = integer->m_value;
        return true;
    }
    inline bool eval_string(const BaseAst *ast, int& value)
    {
        //const StringAst *str = (const StringAst *)ast;
        value = 0;
        return false;
    }
    inline bool eval_triple(const BaseAst *ast, int& value)
    {
        const TripleAst *triple = (const TripleAst *)ast;
        if (triple->m_str != "?")
        {
            value = 0;
            return false;
        }
        int first, second, third;
        if (!eval_ast(triple->m_first, first) ||
            !eval_ast(triple->m_second, second) ||
            !eval_ast(triple->m_third, third))
        {
            value = 0;
            return false;
        }
        value = first ? second : third;
        return true;
    }
    inline bool eval_unary(const BaseAst *ast, int& value)
    {
        const UnaryAst *unary = (const UnaryAst *)ast;
        if (!eval_ast(unary->m_arg, value))
            return false;

        if (unary->m_str == "+") value = value;
        else if (unary->m_str == "-") value = -value;
        else if (unary->m_str == "~") value = ~value;
        else if (unary->m_str == "!") value = !value;
        else if (unary->m_str == "+") value = value;
        else if (unary->m_str == "+") value = value;
        else return false;
        return true;
    }

    inline bool eval_ast(const BaseAst *ast, int& value)
    {
        switch (ast->m_id)
        {
        case ASTID_BINARY:
            return eval_binary(ast, value);
        case ASTID_COMMA:
            return eval_comma(ast, value);
        case ASTID_IDENT:
            return eval_ident(ast, value);
        case ASTID_INTEGER:
            return eval_integer(ast, value);
        case ASTID_STRING:
            return eval_string(ast, value);
        case ASTID_TRIPLE:
            return eval_triple(ast, value);
        case ASTID_UNARY:
            return eval_unary(ast, value);
        default:
            value = 0;
            return false;
        }
    }
} // namespace MacroParser

/////////////////////////////////////////////////////////////////////////

#endif  // ndef MACROPARSER_HPP_
