#include "parse.hpp"

#include "components/config.hpp"
#include "api/emoji.hpp"
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
                for (int i = 0; i < to_match.size(); i++)
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

bool qb::parse::compare_emotes(const std::string& s, const qb::api::Emoji& e) {
    if (e.id) return compare_emotes(s, *e.id);
    return compare_emotes(s, *e.name);
}
