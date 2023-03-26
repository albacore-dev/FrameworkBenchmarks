#pragma once

#include <fmt/core.h>

#include <string>

namespace skye_benchmark {

struct Object {
    std::string message;
};

struct World {
    int id{};
    int randomNumber{};
};

} // namespace skye_benchmark

template <>
struct fmt::formatter<skye_benchmark::Object> {
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const skye_benchmark::Object& model, FormatContext& ctx) const
    {
        return fmt::format_to(
            ctx.out(), "{{\"message\":\"{}\"}}", model.message);
    }
};

template <>
struct fmt::formatter<skye_benchmark::World> {
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const skye_benchmark::World& model, FormatContext& ctx) const
    {
        return fmt::format_to(
            ctx.out(), "{{\"id\":{},\"randomNumber\":{}}}", model.id,
            model.randomNumber);
    }
};
