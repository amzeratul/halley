#pragma once

#include <cstdint>
#include "halley/text/string_converter.h"

namespace Halley {
    enum class MathOp : uint8_t {
        Add,
        Subtract,
        Multiply,
        Divide,
        Max,
        Min
    };
	
	template <>
	struct EnumNames<MathOp> {
		constexpr std::array<const char*, 6> operator()() const {
			return{{
				"+",
				"-",
				"*",
				"/",
				"max",
				"min"
			}};
		}
	};

    enum class MathRelOp : uint8_t {
        Equal,
        Different,
        Less,
        LessOrEqual,
        Greater,
        GreaterOrEqual,
    };
	
	template <>
	struct EnumNames<MathRelOp> {
		constexpr std::array<const char*, 6> operator()() const {
			return{{
				"==",
				"!=",
				"<",
				"<=",
				">",
				">="
			}};
		}
	};

    class MathOps {
    public:
        template <typename T>
        static T apply(MathOp op, const T& a, const T& b)
        {
            switch (op) {
            case MathOp::Add:
                return a + b;
            case MathOp::Subtract:
                return a - b;
            case MathOp::Multiply:
                return a * b;
            case MathOp::Divide:
                return a / b;
            case MathOp::Max:
                return std::max(a, b);
            case MathOp::Min:
                return std::min(a, b);            
            }
            return a;
        }

        template <typename T>
        static bool compare(MathRelOp op, const T& a, const T& b)
        {
            switch (op) {
            case MathRelOp::Equal:
                return a == b;
            case MathRelOp::Different:
                return a != b;
            case MathRelOp::Less:
                return a < b;
            case MathRelOp::LessOrEqual:
                return a <= b;
            case MathRelOp::Greater:
                return a > b;
            case MathRelOp::GreaterOrEqual:
                return a >= b;
            }
            return false;
        }

        template <MathRelOp op, typename T>
        static bool compare(const T& a, const T& b)
        {
            switch (op) {
            case MathRelOp::Equal:
                return a == b;
            case MathRelOp::Different:
                return a != b;
            case MathRelOp::Less:
                return a < b;
            case MathRelOp::LessOrEqual:
                return a <= b;
            case MathRelOp::Greater:
                return a > b;
            case MathRelOp::GreaterOrEqual:
                return a >= b;
            }
            return false;
        }

        template <MathRelOp op, typename T>
        static bool compareEq(const T& a, const T& b)
        {
            switch (op) {
            case MathRelOp::Equal:
                return a == b;
            case MathRelOp::Different:
                return a != b;
            default:
                return false;
            }
            return false;
        }
    };
}
