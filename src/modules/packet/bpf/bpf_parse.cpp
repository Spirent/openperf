#include <cstdlib>
#include <iostream>
#include <sstream>
#include <utility>
#include <optional>
#include <list>
#include <cassert>

#include "packet/bpf/bpf_parse.hpp"

namespace openperf::packet::bpf {

enum class token_type {
    NONE,
    LPAREN,
    RPAREN,
    AND,
    OR,
    NOT,
    EQ,
    NEQ,
    GT,
    GTE,
    LT,
    LTE,
    WORD,
    VALID,
    SIGNATURE,
};

bool is_possible_two_byte_op(char ch)
{
    // ==, !=, <=, >=, ||, &&
    return (ch == '=' || ch == '!' || ch == '<' || ch == '>');
}

token_type to_token_type(std::string_view str)
{
    if (str == "and" || str == "&&") return token_type::AND;
    if (str == "or" || str == "||") return token_type::OR;
    if (str == "not" || str == "!") return token_type::NOT;
    if (str == "=") return token_type::EQ;
    if (str == "==") return token_type::EQ;
    if (str == "!=") return token_type::NEQ;
    if (str == "<") return token_type::LT;
    if (str == "<=") return token_type::LTE;
    if (str == ">") return token_type::GT;
    if (str == ">=") return token_type::GTE;
    if (str == "(") return token_type::LPAREN;
    if (str == ")") return token_type::RPAREN;
    if (str == "valid") return token_type::VALID;
    if (str == "signature") return token_type::SIGNATURE;
    return token_type::NONE;
}

std::string trim(const std::string& s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) { start++; }

    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

std::string to_string(binary_logical_op op)
{
    switch (op) {
    case binary_logical_op::AND:
        return "&&";
    case binary_logical_op::OR:
        return "||";
    }
}

std::string to_string(unary_logical_op op)
{
    switch (op) {
    case unary_logical_op::NOT:
        return "not";
    }
}

bool expr::has_special() const
{
    if (is_special()) return true;

    for (auto child : get_children()) {
        if (child->has_special()) return true;
    }
    return false;
}

bool expr::has_all_special() const
{
    if (is_special()) return true;

    auto children = get_children();
    if (children.empty()) return false;

    for (auto child : children) {
        if (!child->has_all_special()) return false;
    }
    return true;
}

std::string generic_match_expr::to_string() const { return "(" + str + ")"; }

std::string valid_match_expr::to_string() const
{
    std::string valid_str;

    if (flags & static_cast<uint32_t>(valid_match_expr::flag_type::ETH_FCS)) {
        valid_str = "fcs";
    }
    if (flags
        & static_cast<uint32_t>(valid_match_expr::flag_type::TCPUDP_CHKSUM)) {
        if (!valid_str.empty()) valid_str += " ";
        valid_str += "chksum";
    }
    if (flags
        & static_cast<uint32_t>(valid_match_expr::flag_type::SIGNATURE_PRBS)) {
        if (!valid_str.empty()) valid_str += " ";
        valid_str += "prbs";
    }

    return std::string("(valid ") + valid_str + std::string(")");
}

std::string signature_match_expr::to_string() const
{
    std::string str = "(signature";

    if (stream_id.has_value()) {
        auto id = stream_id.value();
        str += " streamid ";
        if (id.start == id.end) {
            str += std::to_string(id.start);
        } else {
            str += std::to_string(id.start);
            str += "-";
            str += std::to_string(id.end);
        }
    }
    str += std::string(")");
    return str;
}

std::string unary_logical_expr::to_string() const
{
    auto str = expr->to_string();
    if (!str.empty() && str[0] == '(') return bpf::to_string(op) + str;
    return bpf::to_string(op) + "(" + str + ")";
}

std::string binary_logical_expr::to_string() const
{
    return "(" + lhs->to_string() + " " + bpf::to_string(op) + " "
           + rhs->to_string() + ")";
}

class tokenizer
{
public:
    struct token
    {
        token_type type = token_type::NONE;
        std::string value;
    };

    tokenizer(std::string_view str)
        : m_str(str)
        , m_offset(0)
    {}

    const token& get_next()
    {
        m_token.value.clear();
        m_token.type = token_type::NONE;

        while (m_offset != m_str.size()) {
            auto ch = m_str[m_offset];
            if (std::isspace(ch)) {
                m_offset += 1;
                if (!m_token.value.empty()) {
                    m_token.type = to_token_type(m_token.value);
                    if (m_token.type == token_type::NONE) {
                        m_token.type = token_type::WORD;
                    }
                    return m_token;
                }
                continue;
            }
            if (!m_token.value.empty()) {
                const auto special_chars = std::string("[].:-");
                if (isalnum(m_token.value[0])
                    || special_chars.find(m_token.value[0])
                           != std::string::npos) {
                    if (!(isalnum(ch)
                          || special_chars.find(ch) != std::string::npos)) {
                        m_token.type = to_token_type(m_token.value);
                        if (m_token.type == token_type::NONE) {
                            m_token.type = token_type::WORD;
                        }
                        return m_token;
                    }
                } else {
                    // Handle operators
                    if (m_token.value.length() == 1) {
                        if (auto type = to_token_type(m_token.value);
                            type != token_type::NONE) {
                            // Special case for 2 byte operators ==, !=, >=, <=
                            if (is_possible_two_byte_op(m_token.value[0])) {
                                m_token.value += ch;
                                m_token.type = to_token_type(m_token.value);
                                if (m_token.type != token_type::NONE) {
                                    m_offset += 1;
                                    return m_token;
                                }
                                m_token.value.resize(1);
                            }
                            m_token.type = type;
                            return m_token;
                        }
                    } else if (m_token.value.length() == 2) {
                        m_token.type = to_token_type(m_token.value);
                        if (m_token.type != token_type::NONE) {
                            return m_token;
                        }
                    }
                }
            }
            m_token.value += ch;
            m_offset += 1;
        }
        if (!m_token.value.empty()) {
            m_token.type = to_token_type(m_token.value);
            if (m_token.type == token_type::NONE) {
                m_token.type = token_type::WORD;
            }
            return m_token;
        }
        m_token.type = token_type::NONE;
        return m_token;
    }

private:
    std::string m_str;
    size_t m_offset;
    token m_token;
};

bool is_binary_logical_op(const token_type& t)
{
    return (t == token_type::AND || t == token_type::OR);
}

bool is_unary_logical_op(const token_type& t) { return (t == token_type::NOT); }

bool is_logical_op(const token_type& t)
{
    return is_binary_logical_op(t) || is_unary_logical_op(t);
}

binary_logical_op to_binary_logical_op(const token_type& t)
{
    switch (t) {
    case token_type::AND:
        return binary_logical_op::AND;
    case token_type::OR:
        return binary_logical_op::OR;
    default:
        throw std::runtime_error("Can not convert token to binary_logical_op");
    }
}

unary_logical_op to_unary_logical_op(const token_type& t)
{
    switch (t) {
    case token_type::NOT:
        return unary_logical_op::NOT;
    default:
        throw std::runtime_error("Can not convert token to unary_logical_op");
    }
}

bool is_compare_op(const token_type& t)
{
    return (t == token_type::EQ || t == token_type::NEQ || t == token_type::GT
            || t == token_type::GTE || t == token_type::LT
            || t == token_type::LTE);
}

void expr_accum(std::unique_ptr<expr>& accum,
                std::unique_ptr<expr>&& e,
                std::optional<binary_logical_op>& op)
{
    if (!accum) {
        accum = std::forward<std::unique_ptr<expr>>(e);
    } else {
        if (!op) { throw std::runtime_error("Missing logical operator"); }
        accum = std::make_unique<binary_logical_expr>(
            std::move(accum),
            std::forward<std::unique_ptr<expr>>(e),
            op.value());
        op = {};
    }
}

class parser
{
public:
    parser(tokenizer& tokenizer)
        : m_tokenizer(tokenizer)
        , m_paren_level(0)
    {
        consume();
    }

    std::unique_ptr<expr> parse() { return parse_logical_expr(); }

private:
    std::unique_ptr<expr> parse_logical_expr(bool unary = false)
    {
        std::optional<binary_logical_op> logical_op;
        std::unique_ptr<expr> accum_expr;

        while (m_token.type != token_type::NONE) {
            switch (m_token.type) {
            case token_type::LPAREN: {
                ++m_paren_level;
                consume();
                auto sub_expr = parse_logical_expr();
                if (m_token.type != token_type::RPAREN) {
                    throw std::runtime_error("Missing ')'");
                }
                --m_paren_level;
                consume();
                expr_accum(accum_expr, std::move(sub_expr), logical_op);
            } break;
            case token_type::RPAREN: {
                // Don't consume the token because caller will handle it
                if (m_paren_level <= 0) {
                    throw std::runtime_error("Mismatched parenthesis");
                }
                return accum_expr;
            } break;
            case token_type::NOT: {
                consume();
                auto sub_expr = parse_logical_expr(true);
                sub_expr = std::make_unique<unary_logical_expr>(
                    std::move(sub_expr), unary_logical_op::NOT);
                expr_accum(accum_expr, std::move(sub_expr), logical_op);
            } break;
            case token_type::AND:
            case token_type::OR: {
                if (logical_op) {
                    throw std::runtime_error("Incorrect logical operator usage "
                                             + m_token.value);
                }
                if (unary) {
                    // urnary operator does not associate with conjunctive
                    return accum_expr;
                }
                logical_op = to_binary_logical_op(m_token.type);
                consume();
            } break;
            case token_type::VALID: {
                auto sub_expr = parse_valid_match_expr();
                expr_accum(accum_expr, std::move(sub_expr), logical_op);
            } break;
            case token_type::SIGNATURE: {
                auto sub_expr = parse_signature_match_expr();
                expr_accum(accum_expr, std::move(sub_expr), logical_op);
            } break;
            case token_type::WORD: {
                auto sub_expr = parse_match_expr();
                expr_accum(accum_expr, std::move(sub_expr), logical_op);
            } break;
            case token_type::NONE:
                break;
            default:
                throw std::runtime_error("Unexpected token found "
                                         + m_token.value);
                break;
            }
        }
        return accum_expr;
    }

    std::unique_ptr<expr> parse_valid_match_expr()
    {
        uint32_t flags = 0;
        bool done = false;

        assert(m_token.type == token_type::VALID);
        consume();

        while (m_token.type != token_type::NONE && !done) {
            switch (m_token.type) {
            case token_type::WORD: {
                if (m_token.value == "fcs") {
                    flags |= static_cast<uint32_t>(
                        valid_match_expr::flag_type::ETH_FCS);
                } else if (m_token.value == "chksum") {
                    flags |= static_cast<uint32_t>(
                        valid_match_expr::flag_type::TCPUDP_CHKSUM);
                } else if (m_token.value == "prbs") {
                    flags |= static_cast<uint32_t>(
                        valid_match_expr::flag_type::SIGNATURE_PRBS);
                } else {
                    throw std::runtime_error("Unexpected valid expr token "
                                             + m_token.value);
                }
                consume();
            } break;
            default: {
                done = true;
            } break;
            }
        }
        if (flags == 0) {
            throw std::runtime_error("Unexpected valid expr missing fields");
        }
        return std::make_unique<valid_match_expr>(flags);
    }

    std::pair<uint32_t, uint32_t> parse_range(const std::string& str)
    {
        auto found = str.find('-');
        if (found == std::string::npos) {
            char* endptr = nullptr;
            auto start = strtoul(str.c_str(), &endptr, 0);
            if (*endptr != '\0') {
                throw std::runtime_error("Error parsing integer value");
            }
            return std::make_pair(start, start);
        } else {
            char* endptr = nullptr;
            const auto start_str = str.substr(0, found);
            const auto end_str = str.substr(found + 1);
            auto start = strtoul(start_str.c_str(), &endptr, 0);
            if (*endptr != '\0') {
                throw std::runtime_error("Error parsing integer value");
            }
            auto end = strtoul(end_str.c_str(), &endptr, 0);
            if (*endptr != '\0') {
                throw std::runtime_error("Error parsing integer value");
            }
            return std::make_pair(start, end);
        }
    }

    std::unique_ptr<expr> parse_signature_match_expr()
    {
        std::string key_str;
        std::optional<signature_match_expr::stream_id_range> stream_id;
        bool done = false;

        assert(m_token.type == token_type::SIGNATURE);
        consume();

        while (m_token.type != token_type::NONE && !done) {
            switch (m_token.type) {
            case token_type::WORD: {
                if (key_str.empty()) {
                    if (m_token.value == "streamid") {
                        if (stream_id.has_value()) {
                            throw std::runtime_error("Already got streamid");
                        }
                        key_str = m_token.value;
                    } else {
                        throw std::runtime_error("Unexpected signature match "
                                                 + m_token.value);
                    }
                } else {
                    if (key_str == "streamid") {
                        auto [start, end] = parse_range(m_token.value);
                        stream_id =
                            signature_match_expr::stream_id_range{start, end};
                        key_str.clear();
                    }
                }
                consume();
            } break;
            default: {
                done = true;
            } break;
            }
        }

        return std::make_unique<signature_match_expr>(stream_id);
    }

    std::string parse_match_expr_term()
    {
        std::string term_str;
        bool done = false;

        while (m_token.type != token_type::NONE && !done) {
            switch (m_token.type) {
            case token_type::WORD: {
                if (!term_str.empty()) { term_str.append(" "); }
                term_str.append(m_token.value);
                consume();
            } break;
            case token_type::LPAREN: {
                ++m_paren_level;
                consume();
                auto sub_expr = parse_match_expr_term();
                if (m_token.type != token_type::RPAREN) {
                    throw std::runtime_error("Missing )");
                }
                --m_paren_level;
                term_str.append("(" + sub_expr + ")");
                consume();
            } break;
            case token_type::RPAREN: {
                if (m_paren_level <= 0) {
                    throw std::runtime_error("Mismatched parenthesis");
                }
                return term_str;
            } break;
            case token_type::AND:
            case token_type::OR:
            case token_type::NOT:
                // If see a logical operator then term is done
                done = true;
                break;
            case token_type::EQ:
            case token_type::NEQ:
            case token_type::LT:
            case token_type::LTE:
            case token_type::GT:
            case token_type::GTE:
                // If see comparison operator than term is done
                done = true;
                break;
            default: {
                term_str += m_token.value;
                consume();
            } break;
            }
        }
        if (term_str.empty()) {
            throw std::runtime_error("Error parsing match expr term");
        }
        return term_str;
    }

    std::unique_ptr<expr> parse_match_expr()
    {
        auto lhs = parse_match_expr_term();
        if (is_compare_op(m_token.type)) {
            auto op = m_token.value;
            consume();
            auto rhs = parse_match_expr_term();
            return std::make_unique<generic_match_expr>(lhs + " " + op + " "
                                                        + rhs);
        }
        return std::make_unique<generic_match_expr>(lhs);
    }

    void consume() { m_token = m_tokenizer.get_next(); }

    tokenizer& m_tokenizer;
    tokenizer::token m_token;
    int m_paren_level;
};

std::unique_ptr<expr> bpf_parse_string(std::string_view str)
{
    tokenizer tokenizer(str);
    parser parser(tokenizer);

    return parser.parse();
}

void expr_move_target_to_lhs(binary_logical_expr& bexpr, expr* target)
{
    if (bexpr.rhs.get() == target) { std::swap(bexpr.lhs, bexpr.rhs); }
}

void expr_move_target_to_rhs(binary_logical_expr& bexpr, expr* target)
{
    if (bexpr.lhs.get() == target) { std::swap(bexpr.lhs, bexpr.rhs); }
}

void expr_toggle_binary_op(binary_logical_expr& expr)
{
    expr.op = (expr.op == binary_logical_op::AND) ? binary_logical_op::OR
                                                  : binary_logical_op::AND;
}

bool expr_is_special_ok(const expr* ex)
{
    if (auto bexpr = dynamic_cast<const binary_logical_expr*>(ex)) {
        bool lhs_has_special = bexpr->lhs->has_special();
        bool rhs_has_special = bexpr->rhs->has_special();

        if (!lhs_has_special && !rhs_has_special)
            return true; // All normal expressions

        bool lhs_has_all_special = bexpr->lhs->has_all_special();
        if (!lhs_has_all_special)
            return false; // LHS has special and normal expressions

        if (!rhs_has_special)
            return true; // LHS has all special and RHS has no special
                         // expressions

        bool rhs_has_all_special = bexpr->rhs->has_all_special();
        if (!rhs_has_all_special)
            return false; // RHS has special and normal expressions

        return true;
    }
    if (auto uexpr = dynamic_cast<const unary_logical_expr*>(ex)) {
        if (uexpr->expr->has_special() && !uexpr->expr->has_all_special()) {
            return false;
        }
        return true;
    }

    // Either a normal or special expression by itself is OK
    return true;
}

std::unique_ptr<expr> expr_remove_double_not(std::unique_ptr<expr>&& ex)
{
    std::unique_ptr<expr> result = std::forward<std::unique_ptr<expr>>(ex);
    if (auto uexpr = dynamic_cast<unary_logical_expr*>(result.get())) {
        if (auto uexpr_child =
                dynamic_cast<unary_logical_expr*>(uexpr->expr.get())) {
            if (uexpr->op == unary_logical_op::NOT
                && uexpr_child->op == unary_logical_op::NOT) {
                // Get rid of double NOT
                result = std::move(uexpr_child->expr);
                return expr_remove_double_not(std::move(result));
            }
        }
        uexpr->expr = expr_remove_double_not(std::move(uexpr->expr));
        return result;
    }
    if (auto bexpr = dynamic_cast<binary_logical_expr*>(result.get())) {
        bexpr->lhs = expr_remove_double_not(std::move(bexpr->lhs));
        bexpr->rhs = expr_remove_double_not(std::move(bexpr->rhs));
    }
    return result;
}

std::unique_ptr<expr> bpf_split_special(std::unique_ptr<expr>&& ex)
{
    std::unique_ptr<expr> result = std::forward<std::unique_ptr<expr>>(ex);
    constexpr auto split_err_str =
        "Can not split BPF into special and normal expressions";

    // Get rid of double NOT to simplify expression parsing
    result = expr_remove_double_not(std::move(result));

    if (expr_is_special_ok(result.get())) { return result; }

    if (auto uexpr = dynamic_cast<unary_logical_expr*>(result.get())) {
        if (auto bexpr =
                dynamic_cast<binary_logical_expr*>(uexpr->expr.get())) {
            // Apply DeMorgan's Rule to move the NOT into the binary expression
            expr_toggle_binary_op(*bexpr);
            bexpr->lhs = std::make_unique<unary_logical_expr>(
                std::move(bexpr->lhs), unary_logical_op::NOT);
            bexpr->rhs = std::make_unique<unary_logical_expr>(
                std::move(bexpr->rhs), unary_logical_op::NOT);
            result = std::move(uexpr->expr);
            return bpf_split_special(std::move(result));
        }
        throw std::runtime_error(split_err_str);
    }

    if (auto bexpr = dynamic_cast<binary_logical_expr*>(result.get())) {
        bool lhs_has_special = bexpr->lhs->has_special();
        bool rhs_has_special = bexpr->rhs->has_special();
        if (rhs_has_special && !lhs_has_special) {
            std::swap(bexpr->lhs, bexpr->rhs);
            std::swap(lhs_has_special, rhs_has_special);
        }
        if (lhs_has_special && !bexpr->lhs->has_all_special()) {
            // Split normal terms into RHS of child LHS expression
            bexpr->lhs = bpf_split_special(std::move(bexpr->lhs));
            if (auto bexpr_child =
                    dynamic_cast<binary_logical_expr*>(bexpr->lhs.get())) {
                if (bexpr_child->rhs->has_special()) {
                    throw std::runtime_error(split_err_str);
                }
                if (bexpr->op != bexpr_child->op) {
                    throw std::runtime_error(split_err_str);
                }
                // Same operators can be reordered
                bexpr->rhs = std::make_unique<binary_logical_expr>(
                    std::move(bexpr_child->rhs),
                    std::move(bexpr->rhs),
                    bexpr->op);
                bexpr->lhs = std::move(bexpr_child->lhs);
            }
        }
        if (rhs_has_special) {
            // Split special terms into LHS of child RHS expression
            bexpr->rhs = bpf_split_special(std::move(bexpr->rhs));
            if (auto bexpr_child =
                    dynamic_cast<binary_logical_expr*>(bexpr->rhs.get())) {
                if (bexpr->op != bexpr_child->op) {
                    throw std::runtime_error(split_err_str);
                }
                // Same operators can be reordered
                bexpr->lhs = std::make_unique<binary_logical_expr>(
                    std::move(bexpr->lhs),
                    std::move(bexpr_child->lhs),
                    bexpr->op);
                bexpr->rhs = std::move(bexpr_child->rhs);
            }
        }
    }
    return result;
}

} // namespace openperf::packet::bpf