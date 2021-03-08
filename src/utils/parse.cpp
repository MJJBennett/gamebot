#include "parse.hpp"
#include <iostream>

#include "api/emoji.hpp"
#include "components/config.hpp"
#include <algorithm>
#include <cctype>

using namespace qb;

std::string qb::parse::trim_leading_ignored(std::string str)
{
    return {std::find_if(str.begin(), str.end(), [](char c) { return std::isalpha(c) || c == '!'; }),
            str.end()};
}

std::string qb::parse::trim(std::string str, const std::string& to_trim)
{
    return ltrim(rtrim(str, to_trim));
}

std::string qb::parse::ltrim(std::string str, const std::string& to_trim)
{
    return {std::find_if(str.begin(), str.end(), [to_trim](char c) { return !in(c, to_trim); }), str.end()};
}

std::string qb::parse::rtrim(std::string str, const std::string& to_trim)
{
    return {str.begin(),
            std::find_if(str.rbegin(), str.rend(), [to_trim](char c) { return !in(c, to_trim); }).base()};
}

std::vector<std::string> qb::parse::split(const std::string& str, char delim)
{
    std::vector<std::string> v;
    auto itr = str.begin();
    while (itr != str.end())
    {
        itr = std::find_if_not(itr, str.end(), [delim](char c) { return c == delim; });
        const auto itr_end = std::find_if(itr, str.end(), [delim](char c) { return c == delim; });
        v.push_back({itr, itr_end});
        itr = itr_end;
    }
    return v;
}

std::string qb::parse::remove_non_cmd(std::string str)
{
    auto pred = [](char c) { return std::isalpha(c) || c == '!'; };
    return std::string(std::find_if(str.begin(), str.end(), pred), str.end());
}

bool qb::parse::startswithword(const std::string& str, const std::string& start)
{
    return ((start.size() == str.size()) && str.substr(0, start.size()) == start) ||
           ((start.size() < str.size()) && !isalpha(str[start.size()]) && str.substr(0, start.size()) == start);
}

qb::parse::SplitNumber qb::parse::split_number(const std::string& s)
{
    auto start = std::find_if(s.begin(), s.end(), [](char c) { return c != ' '; });
    if (start == s.end()) return {};
    auto num_end = std::find_if(start, s.end(), [](char c) { return !isdigit(c); });
    if (start == num_end) return SplitNumber{{}, s};
    return SplitNumber{std::stoi(std::string(start, num_end)), {num_end, s.end()}};
}

bool qb::parse::is_valid_time_trailer(const std::string& s)
{
    // d4h -> d
    // daysofthemonth5555 -> daysofthemonth
    // desparate55 -> desparate
    std::string non_digits{s.begin(),
                           std::find_if(s.begin(), s.end(), [](char c) { return isdigit(c); })};
    // very bad but very functional! A for effort
    if (non_digits.size() > 7 || non_digits.size() == 0) return false;
    if (::qb::parse::in(non_digits[0], std::string{"hmsd"})) return true;
    return false;
}

std::chrono::duration<long> get_seconds(const std::string& trailer, long num)
{
    using ld = std::chrono::duration<long>;
    if (trailer[0] == 's') return ld(num);
    if (trailer[0] == 'm') return ld(num * 60);
    if (trailer[0] == 'h') return ld(num * 60 * 60);
    if (trailer[0] == 'd') return ld(num * 60 * 60 * 24);
    return ld(0);
}

void qb::parse::decompose_argument(qb::parse::DecomposedCommand& res, std::string in)
{
    try
    {
        // Check if we only got a number
        auto [first_num, first_trail] = split_number(in);
        if (first_num && first_trail.empty())
        {
            res.numeric_arguments.push_back(*first_num);
            return;
        }
        // Check if we only got a string (no leading number)
        if (!first_num)
        {
            res.arguments.push_back(in);
            return;
        }
        // Okay, we got a number with trailing characters.
        if (!is_valid_time_trailer(first_trail))
        {
            res.arguments.push_back(in);
            return;
        }
        // no months+ support yet
        std::chrono::duration<long> s(0);
        s += get_seconds(first_trail, *first_num);
        std::string prev_trail = first_trail; // = d5h or similar
        while (true)
        {
            // so currently prev_trail is something like d123d123d
            // 5h or similar
            prev_trail = std::string{std::find_if(prev_trail.begin(), prev_trail.end(), isdigit),
                                     prev_trail.end()};
            // now we do the above and have 123d123d
            // now here we get 123, d123d
            // if trailer is empty, we return. if not a valid time trailer,
            // we also return.
            // eventually we get down to 123d
            // then our next iteration we don't find any numbers
            // so we return:
            if (prev_trail.empty())
            {
                res.duration_arguments.push_back(s);
                return;
            }
            auto [num, trail] = split_number(prev_trail);
            prev_trail        = trail;
            // Okay, random trailing numbers, nevermind.
            if (trail.empty() || !is_valid_time_trailer(trail))
            {
                res.arguments.push_back(in);
                return;
            }
            s += get_seconds(trail, *num);
        }
    }
    catch (const std::exception& e)
    {
        res.arguments.push_back(in);
        return;
    }
}

qb::parse::DecomposedCommand qb::parse::decompose_command(const std::string& command)
{
    qb::parse::DecomposedCommand ret;
    auto cmds = split(command);
    if (cmds.size() < 2) return ret;
    for (auto itr = cmds.begin() + 1; itr != cmds.end(); itr++)
    {
        bool found_str = false;
        if (startswith(*itr, "\"")) {
            for (auto itr_2 = itr; itr_2 != cmds.end(); itr_2++) {
                if (endswith(*itr_2, "\"")) {
                    found_str = true; 
                    std::string s;
                    for (; itr != itr_2 + 1; itr++) {
                        s += *itr; 
                        s += ' ';
                    }
                    itr = itr - 1;
                    ret.arguments.emplace_back(s.substr(1, s.size() - 3));
                    break;
                }
            }
        }
        if (found_str) continue;
        decompose_argument(ret, *itr);
    }
    return ret;
}

std::tuple<std::string, std::vector<std::string>> qb::parse::get_time(std::vector<std::string> to_search)
{
    /**
     * Looking at this function now (13/02/21)
     * it seems like this, match(2).1, and match(2).2 are all
     * tools for parsing time expressions out of strings.
     * However, none of them seem particularly useful.
     * It seems like get_time is being used, though, so I suppose
     * the best approach is to write some unit tests for it and
     * then improve it for future use.
     */
    std::vector<std::string> not_matches;
    std::string found{""};
    bool was_found{false};

    auto is_time = [](char c) {
        return (isalpha(c) && (tolower(c) == 'm' || tolower(c) == 's' || tolower(c) == 'h'));
    };

    for (auto&& item : to_search)
    {
        if (!was_found && item.size() > 1)
        {
            size_t pos = 0;
            while (true)
            {
                if (!isdigit(item[pos])) break;
                // consume numbers
                while (pos < item.size() && isdigit(item[pos]))
                    pos++;
                // consume a time
                if (pos == item.size() || !is_time(item[pos])) break;
                pos++;
                if (pos == item.size())
                {
                    was_found = true;
                    found     = std::move(item);
                    goto cont;
                }
            }
        }
        not_matches.emplace_back(std::move(item));
    cont:;
    }

    return {found, not_matches};
}

std::tuple<std::string, std::vector<std::string>> qb::parse::match(std::string expr,
                                                                   std::vector<std::string> to_search)
{
    // This is my extremely quick and dirty expression matcher
    // Our goal is to find a single instance that matches expr within to_search
    // Then return that, plus everything else from to_search that wasn't found
    // This is not particularly efficient, but this function is being implemented with the
    // express purpose of not being run very frequently, and it's more fun to do it this way
    // than to just use a library implementation, so we're doing that.
    // Alternatively a full expression parser could be here but I think that's probably overkill.
    std::vector<std::string> not_matches;
    std::string found{""};
    bool was_found{false};

    for (auto&& item : to_search)
    {
        if (!was_found && match(expr, item))
        {
            was_found = true;
            found     = std::move(item);
            continue;
        }
        not_matches.emplace_back(std::move(item));
    }

    return {found, not_matches};
}

bool qb::parse::match(std::string expr, std::string to_match)
{
    // This doesn't really work or do anything but I don't have the heart to delete it
    const size_t expr_end = expr.size();
    size_t pos            = 0;
    auto is_time          = [](char c) {
        return (isalpha(c) && (tolower(c) == 'm' || tolower(c) == 's' || tolower(c) == 'h'));
    };
    auto is_char = [&](char expr_c, char act_c) -> bool {
        if (expr_c == 'N') return isdigit(act_c);
        if (expr_c == 'C') return isalpha(act_c);
        if (expr_c == 'T') return is_time(act_c);
        return false;
    };
    std::optional<std::string> group;
    while (pos < expr_end)
    {
        if (expr[pos] == '[') // start a group
        {
            assert(!group);
            group.emplace();
            pos++;
            // now consume group expression
            while (expr[pos] != ']')
            {
                (*group) += expr[pos];
                pos++;
                assert(pos != expr_end);
            }
            pos++;
            bool repeat = (pos != expr_end && expr[pos] == '+');
            do
            {
                // consume the thing!
                size_t g_pos{0};
                for (size_t i = 0; i < to_match.size(); i++)
                {
                    if (is_char((*group)[g_pos], to_match[i]))
                    // we matched a character, nice
                    {
                        // see if that repeats and we can consume more
                        if (g_pos != group->size() && (*group)[g_pos] == '+')
                        {
                        }
                    }
                }
            } while (repeat);
        }
    }
    return false;
}

bool qb::parse::is_command(std::string str)
{
    return startswith(trim_leading_ignored(str), config::cmd_start());
}

std::string qb::parse::get_command(std::string str)
{
    // Assumes cmd_start() has not already been removed.
    auto cmd_seed = remove_non_cmd(str);
    assert(startswith(cmd_seed, config::cmd_start()));
    return cmd_seed.substr(config::cmd_start().size());
}

std::string qb::parse::get_command_name(std::string str)
{
    // Assumes cmd_start() has already been removed.
    auto s = std::find_if_not(str.begin(), str.end(), isspace);
    return {s, std::find_if(s, str.end(), isspace)};
}

bool qb::parse::compare_emotes(const std::string& s, const qb::api::Emoji& e)
{
    if (e.id) return compare_emotes(s, *e.id);
    return compare_emotes(s, *e.name);
}
