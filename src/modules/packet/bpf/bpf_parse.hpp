#ifndef _OP_PACKET_BPF_PARSE_HPP_
#define _OP_PACKET_BPF_PARSE_HPP_

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace openperf::packet::bpf {

class generic_match_expr;
class valid_match_expr;
class signature_match_expr;
class unary_logical_expr;
class binary_logical_expr;

class expr_visitor
{
public:
    virtual void visit(const generic_match_expr& expr) = 0;
    virtual void visit(const valid_match_expr& expr) = 0;
    virtual void visit(const signature_match_expr& expr) = 0;
    virtual void visit(const unary_logical_expr& expr) = 0;
    virtual void visit(const binary_logical_expr& expr) = 0;
};

class expr
{
public:
    virtual ~expr() = default;
    virtual std::vector<expr*> get_children() const
    {
        return std::vector<expr*>();
    }
    virtual std::string to_string() const = 0;

    virtual bool is_special() const { return false; }
    virtual bool is_buildable() const { return true; }

    virtual void accept(expr_visitor& visitor) const = 0;

    bool has_special() const;
    bool has_all_special() const;
};

template <class T> class expr_base : public expr
{
public:
    void accept(expr_visitor& visitor) const override
    {
        visitor.visit(static_cast<const T&>(*this));
    }
};

class generic_match_expr : public expr_base<generic_match_expr>
{
public:
    generic_match_expr(std::string_view v)
        : str(v)
    {}

    std::string to_string() const override;

    std::string str;
};

class valid_match_expr : public expr_base<valid_match_expr>
{
public:
    enum class flag_type {
        ETH_FCS = 0x01,
        TCPUDP_CHKSUM = 0x02,
        SIGNATURE_PRBS = 0x04
    };

    valid_match_expr(uint32_t f)
        : flags(f)
    {}

    std::string to_string() const override;

    bool is_special() const override { return true; }

    uint32_t flags;
};

class signature_match_expr : public expr_base<signature_match_expr>
{
public:
    struct stream_id_range
    {
        uint32_t start;
        uint32_t end;
    };

    signature_match_expr(const std::optional<stream_id_range>& id)
        : stream_id(id)
    {}

    std::string to_string() const override;

    bool is_special() const override { return true; }

    std::optional<stream_id_range> stream_id;
};

enum class binary_logical_op { AND, OR };
enum class unary_logical_op { NOT };

class unary_logical_expr : public expr_base<unary_logical_expr>
{
public:
    unary_logical_expr(std::unique_ptr<expr>&& e, unary_logical_op o)
        : expr(std::forward<std::unique_ptr<class expr>>(e))
        , op(o)
    {}

    std::vector<expr*> get_children() const override { return {expr.get()}; }

    std::string to_string() const override;

    bool is_buildable() const override;

    std::unique_ptr<expr> expr;
    unary_logical_op op; /* not */
};

class binary_logical_expr : public expr_base<binary_logical_expr>
{
public:
    binary_logical_expr(std::unique_ptr<expr>&& l,
                        std::unique_ptr<expr>&& r,
                        binary_logical_op o)
        : lhs(std::forward<std::unique_ptr<expr>>(l))
        , rhs(std::forward<std::unique_ptr<expr>>(r))
        , op(o)
    {}

    std::vector<expr*> get_children() const override
    {
        return {lhs.get(), rhs.get()};
    }

    std::string to_string() const override;

    bool is_buildable() const override;

    std::unique_ptr<expr> lhs;
    std::unique_ptr<expr> rhs;
    binary_logical_op op; /* and, or */
};

std::unique_ptr<expr> bpf_parse_string(std::string_view str);

std::unique_ptr<expr> bpf_split_special(std::unique_ptr<expr>&& expr);

} // namespace openperf::packet::bpf

#endif // _OP_PACKET_BPF_PARSE_HPP_